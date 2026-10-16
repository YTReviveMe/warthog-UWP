#include <windows.h>
#include <algorithm>
#include <bcrypt.h>
#include <chrono>
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
