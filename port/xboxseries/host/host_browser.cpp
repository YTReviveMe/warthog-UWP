#include <windows.h>
#include <algorithm>
#include <bcrypt.h>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Web.Http.h>
extern "C" {
#include "browser_http.h"
}

using namespace winrt;
using namespace Windows::Web::Http;
using namespace Windows::Storage::Streams;

static SRWLOCK download_lock = SRWLOCK_INIT;
static HttpClient download_client = nullptr;
static HttpResponseMessage download_response = nullptr;
static IInputStream download_stream = nullptr;

static int request(const char *url, const char *body, size_t length, const char *type, const char *agent,
                   const char *headers, char *response, int capacity, char *error, int error_capacity) {
    if (!url || !response || capacity <= 0)
        return 0;
    response[0] = 0;
    if (error && error_capacity > 0)
        error[0] = 0;
    try {
        HttpClient client;
        HttpRequestMessage message(body ? HttpMethod::Post() : HttpMethod::Get(),
                                   Windows::Foundation::Uri(to_hstring(url)));
        message.Headers().TryAppendWithoutValidation(L"User-Agent", to_hstring(agent ? agent : "warthog-uwp"));
        if (body) {
            DataWriter writer;
            writer.WriteBytes(array_view<const uint8_t>(reinterpret_cast<const uint8_t *>(body),
                                                        reinterpret_cast<const uint8_t *>(body) + length));
            HttpBufferContent content(writer.DetachBuffer());
            content.Headers().TryAppendWithoutValidation(L"Content-Type",
                                                         to_hstring(type ? type : "application/x-www-form-urlencoded"));
            message.Content(content);
        }
        if (headers) {
            std::string lines(headers);
            size_t start = 0;
            while (start < lines.size()) {
                size_t end = lines.find("\r\n", start);
                if (end == std::string::npos)
                    end = lines.size();
                size_t colon = lines.find(':', start);
                if (colon < end)
                    message.Headers().TryAppendWithoutValidation(to_hstring(lines.substr(start, colon - start)),
                                                                 to_hstring(lines.substr(colon + 1, end - colon - 1)));
                start = end + 2;
            }
        }
        auto operation = client.SendRequestAsync(message);
        if (operation.wait_for(std::chrono::seconds(10)) != Windows::Foundation::AsyncStatus::Completed) {
            operation.Cancel();
            if (error && error_capacity > 0)
                snprintf(error, error_capacity, "HTTP request timed out");
            return 0;
        }
        auto reply = operation.GetResults();
        auto read = reply.Content().ReadAsBufferAsync();
        if (read.wait_for(std::chrono::seconds(10)) != Windows::Foundation::AsyncStatus::Completed) {
            read.Cancel();
            if (error && error_capacity > 0)
                snprintf(error, error_capacity, "HTTP response timed out");
            return 0;
        }
        auto buffer = read.GetResults();
        size_t count = (std::min)(static_cast<size_t>(buffer.Length()), static_cast<size_t>(capacity - 1));
        memcpy(response, buffer.data(), count);
        response[count] = 0;
        return static_cast<int>(reply.StatusCode());
    } catch (const hresult_error &failure) {
        if (error && error_capacity > 0)
            snprintf(error, error_capacity, "HTTP failure %08x", static_cast<unsigned>(failure.code().value));
        return 0;
    }
}

extern "C" int posix_browser_request(const char *url, const char *body, const char *type, char *response, int capacity,
                                     char *error, int error_capacity) {
    return request(url, body, body ? strlen(body) : 0, type, nullptr, nullptr, response, capacity, error,
                   error_capacity);
}

extern "C" int posix_browser_request_as(const char *url, const char *body, const char *type, const char *agent,
                                        char *response, int capacity, char *error, int error_capacity) {
    return request(url, body, body ? strlen(body) : 0, type, agent, nullptr, response, capacity, error, error_capacity);
}

extern "C" int posix_browser_send(const char *url, const char *body, size_t length, const char *type,
                                  const char *headers, char *response, int capacity, char *error, int error_capacity) {
    return request(url, body, length, type, nullptr, headers, response, capacity, error, error_capacity);
}

extern "C" int posix_browser_download_open(const char *url, const char *agent, int *status, int *length, char *error,
                                            int error_capacity) {
    if (!url || !status || !length)
        return 0;
    *status = 0;
    *length = -1;
    if (error && error_capacity > 0)
        error[0] = 0;
    AcquireSRWLockExclusive(&download_lock);
    if (download_stream) {
        if (error && error_capacity > 0)
            snprintf(error, error_capacity, "a download is open already");
        ReleaseSRWLockExclusive(&download_lock);
        return 0;
    }
    try {
        HttpClient client;
        HttpRequestMessage message(HttpMethod::Get(), Windows::Foundation::Uri(to_hstring(url)));
        message.Headers().TryAppendWithoutValidation(L"User-Agent", to_hstring(agent ? agent : "warthog-uwp"));
        auto operation = client.SendRequestAsync(message, HttpCompletionOption::ResponseHeadersRead);
        if (operation.wait_for(std::chrono::seconds(10)) != Windows::Foundation::AsyncStatus::Completed) {
            operation.Cancel();
            if (error && error_capacity > 0)
                snprintf(error, error_capacity, "HTTP request timed out");
            ReleaseSRWLockExclusive(&download_lock);
            return 0;
        }
        auto reply = operation.GetResults();
        *status = static_cast<int>(reply.StatusCode());
        if (*status != 200) {
            if (error && error_capacity > 0)
                snprintf(error, error_capacity, "the server answered %d", *status);
            ReleaseSRWLockExclusive(&download_lock);
            return 0;
        }
        auto size = reply.Content().Headers().ContentLength();
        if (size && size.Value() <= INT_MAX)
            *length = static_cast<int>(size.Value());
        auto read = reply.Content().ReadAsInputStreamAsync();
        if (read.wait_for(std::chrono::seconds(10)) != Windows::Foundation::AsyncStatus::Completed) {
            read.Cancel();
            if (error && error_capacity > 0)
                snprintf(error, error_capacity, "HTTP response timed out");
            ReleaseSRWLockExclusive(&download_lock);
            return 0;
        }
        download_stream = read.GetResults();
        download_response = reply;
        download_client = client;
        ReleaseSRWLockExclusive(&download_lock);
        return 1;
    } catch (const hresult_error &failure) {
        if (error && error_capacity > 0)
            snprintf(error, error_capacity, "HTTP failure %08x", static_cast<unsigned>(failure.code().value));
        ReleaseSRWLockExclusive(&download_lock);
        return 0;
    }
}

extern "C" int posix_browser_download_read(int download, void *buffer, int size) {
    if (download != 1 || !buffer || size <= 0)
        return -1;
    AcquireSRWLockExclusive(&download_lock);
    if (!download_stream) {
        ReleaseSRWLockExclusive(&download_lock);
        return -1;
    }
    try {
        Buffer chunk(static_cast<uint32_t>(size));
        auto operation = download_stream.ReadAsync(chunk, static_cast<uint32_t>(size), InputStreamOptions::Partial);
        if (operation.wait_for(std::chrono::seconds(10)) != Windows::Foundation::AsyncStatus::Completed) {
            operation.Cancel();
            ReleaseSRWLockExclusive(&download_lock);
            return -1;
        }
        auto result = operation.GetResults();
        int count = static_cast<int>(result.Length());
        if (count > 0)
            memcpy(buffer, result.data(), static_cast<size_t>(count));
        ReleaseSRWLockExclusive(&download_lock);
        return count;
    } catch (const hresult_error &) {
        ReleaseSRWLockExclusive(&download_lock);
        return -1;
    }
}

extern "C" void posix_browser_download_close(int download) {
    if (download != 1)
        return;
    AcquireSRWLockExclusive(&download_lock);
    download_stream = nullptr;
    download_response = nullptr;
    download_client = nullptr;
    ReleaseSRWLockExclusive(&download_lock);
}

extern "C" void posix_browser_lock(void) {}

extern "C" void posix_browser_unlock(void) {}

extern "C" int posix_browser_private_key(const char *path, unsigned char *key, int size) {
    if (!path || !key || size <= 0)
        return 0;
    auto filename = std::filesystem::path(to_hstring(path).c_str());
    std::ifstream existing(filename, std::ios::binary);
    if (existing)
        return static_cast<bool>(existing.read(reinterpret_cast<char *>(key), size));
    if (BCryptGenRandom(nullptr, key, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
        return 0;
    HANDLE file = CreateFile2(filename.c_str(), GENERIC_WRITE, 0, CREATE_NEW, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return 0;
    DWORD written = 0;
    bool saved = WriteFile(file, key, size, &written, nullptr) && written == static_cast<DWORD>(size);
    CloseHandle(file);
    return saved;
}

extern "C" int posix_browser_replace_key(const char *path, const unsigned char *key, int size) {
    if (!path || !key || size <= 0)
        return 0;
    auto filename = std::filesystem::path(to_hstring(path).c_str());
    auto temporary = filename;
    temporary += L".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output.write(reinterpret_cast<const char *>(key), size))
        return 0;
    output.close();
    return MoveFileExW(temporary.c_str(), filename.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) !=
           FALSE;
}
