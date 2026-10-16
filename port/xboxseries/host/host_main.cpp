#include "host.h"
#include "xiso_installer.h"
#include <SDL3/SDL.h>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <libuwp.h>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <vector>
#include <windows.h>
#include <winrt/Windows.Storage.h>
#include <zlib.h>

static std::mutex log_mutex;
static FILE *log_file;
static std::string data_root, save_root;

static bool same_path(const std::filesystem::path &left, const std::filesystem::path &right) {
    auto a = left.lexically_normal().wstring();
    auto b = right.lexically_normal().wstring();
    while (a.size() > 3 && (a.back() == L'\\' || a.back() == L'/'))
        a.pop_back();
    while (b.size() > 3 && (b.back() == L'\\' || b.back() == L'/'))
        b.pop_back();
    return _wcsicmp(a.c_str(), b.c_str()) == 0;
}

static void copy_missing_tree(const std::filesystem::path &source, const std::filesystem::path &destination) {
    std::error_code error;
    if (!std::filesystem::is_directory(source, error))
        return;
    std::filesystem::create_directories(destination, error);
    error.clear();
    for (std::filesystem::recursive_directory_iterator item(source, error), end; item != end && !error;
         item.increment(error)) {
        const auto relative = item->path().lexically_relative(source);
        const auto target = destination / relative;
        const auto filename = item->path().filename().wstring();
        if (filename.size() >= 9 && !_wcsnicmp(filename.c_str(), L"cache", 5) &&
            !_wcsicmp(item->path().extension().c_str(), L".map"))
            continue;
        if (item->is_directory(error)) {
            std::filesystem::create_directories(target, error);
        } else if (item->is_regular_file(error) && !std::filesystem::exists(target, error)) {
            std::filesystem::create_directories(target.parent_path(), error);
            error.clear();
            std::filesystem::copy_file(item->path(), target, std::filesystem::copy_options::skip_existing, error);
        }
        if (error) {
            host_logf(HOST_LOG_WARN, "portable data migration skipped %s: %s",
                      winrt::to_string(item->path().wstring()).c_str(), error.message().c_str());
            error.clear();
        }
    }
}

static void prepare_portable_storage(const std::filesystem::path &internal, const std::filesystem::path &selected) {
    if (same_path(internal, selected))
        return;
    std::error_code error;
    const auto migration_marker = selected / L".portable-storage-v1";
    std::filesystem::create_directories(selected / L"save", error);
    if (std::filesystem::exists(migration_marker, error))
        return;
    copy_missing_tree(internal / L"save", selected / L"save");
    const auto internal_config = internal / L"config.toml";
    const auto portable_config = selected / L"config.toml";
    if (std::filesystem::is_regular_file(internal_config, error) && !std::filesystem::exists(portable_config, error)) {
        error.clear();
        std::filesystem::copy_file(internal_config, portable_config, std::filesystem::copy_options::skip_existing,
                                   error);
        if (error)
            host_logf(HOST_LOG_WARN, "portable config migration failed: %s", error.message().c_str());
    }
    std::ofstream marker(migration_marker, std::ios::trunc);
    marker << "Warthog portable storage initialized\n";
    host_logf(HOST_LOG_INFO, "portable storage root: %s", winrt::to_string(selected.wstring()).c_str());
}

static void open_log_at(const std::filesystem::path &root) {
    std::lock_guard<std::mutex> guard(log_mutex);
    std::error_code error;
    std::filesystem::create_directories(root, error);
    FILE *replacement = nullptr;
    const auto path = root / L"warthog.log";
    const int result = _wfopen_s(&replacement, path.c_str(), L"a");
    if (result || !replacement) {
        if (log_file) {
            fprintf(log_file, "Log relocation failed errno=%d path=%ls\n", result, path.c_str());
            fflush(log_file);
        }
        OutputDebugStringA("Warthog: could not open log; retaining previous destination\n");
        return;
    }
    setvbuf(replacement, nullptr, _IONBF, 0);
    if (log_file) {
        fflush(log_file);
        fclose(log_file);
        log_file = nullptr;
    }
    log_file = replacement;
    fprintf(log_file, "\n--- Warthog log opened; tick=%llu ---\n", GetTickCount64());
}

extern "C" int GUEST_ABI host_decompress_map(const char *source_path, const char *destination_path,
                                             uint32_t decompressed_size, uint32_t destination_size) {
    typedef int(__cdecl * inflate_init_proc)(z_streamp, const char *, int);
    typedef int(__cdecl * inflate_proc)(z_streamp, int);
    typedef int(__cdecl * inflate_end_proc)(z_streamp);
    constexpr size_t map_header_size = 0x800;
    constexpr size_t input_buffer_size = 1024 * 1024;

    if (!source_path || !destination_path || decompressed_size <= map_header_size ||
        decompressed_size > destination_size) {
        host_logf(HOST_LOG_ERROR, "native cache: invalid request size=%u capacity=%u", decompressed_size,
                  destination_size);
        return 0;
    }

    std::ifstream source(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t *>(source_path))),
                         std::ios::binary | std::ios::ate);
    if (!source) {
        host_logf(HOST_LOG_ERROR, "native cache: cannot open source %s", source_path);
        return 0;
    }
    const std::streamoff source_size = source.tellg();
    if (source_size <= (std::streamoff)map_header_size ||
        source_size - (std::streamoff)map_header_size > 0xffffffffLL) {
        host_logf(HOST_LOG_ERROR, "native cache: invalid compressed size %lld", (long long)source_size);
        return 0;
    }

    std::array<unsigned char, map_header_size> header{};
    std::unique_ptr<unsigned char[]> compressed(new (std::nothrow) unsigned char[input_buffer_size]);
    if (!compressed) {
        host_logf(HOST_LOG_ERROR, "native cache: cannot allocate %zu-byte input buffer", input_buffer_size);
        return 0;
    }
    source.seekg(0);
    source.read((char *)header.data(), (std::streamsize)header.size());
    if (!source) {
        host_logf(HOST_LOG_ERROR, "native cache: failed reading header from %s", source_path);
        return 0;
    }

    HMODULE zlib = LoadPackagedLibrary(L"z-1.dll", 0);
    auto inflate_init = zlib ? reinterpret_cast<inflate_init_proc>(GetProcAddress(zlib, "inflateInit_")) : nullptr;
    auto inflate_data = zlib ? reinterpret_cast<inflate_proc>(GetProcAddress(zlib, "inflate")) : nullptr;
    auto inflate_end = zlib ? reinterpret_cast<inflate_end_proc>(GetProcAddress(zlib, "inflateEnd")) : nullptr;
    if (!inflate_init || !inflate_data || !inflate_end) {
        host_logf(HOST_LOG_ERROR, "native cache: zlib unavailable error=%lu", GetLastError());
        if (zlib)
            FreeLibrary(zlib);
        return 0;
    }
    const auto destination_file_path =
        std::filesystem::path(std::u8string(reinterpret_cast<const char8_t *>(destination_path)));
    CREATEFILE2_EXTENDED_PARAMETERS file_options{};
    file_options.dwSize = sizeof(file_options);
    file_options.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    HANDLE destination =
        CreateFile2(destination_file_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, OPEN_EXISTING, &file_options);
    if (destination == INVALID_HANDLE_VALUE) {
        host_logf(HOST_LOG_ERROR, "native cache: cannot open destination %s", destination_path);
        FreeLibrary(zlib);
        return 0;
    }
    HANDLE mapping = CreateFileMappingW(destination, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    unsigned char *destination_data =
        mapping ? static_cast<unsigned char *>(MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, decompressed_size))
                : nullptr;
    if (!destination_data) {
        host_logf(HOST_LOG_ERROR, "native cache: cannot map destination %s error=%lu", destination_path,
                  GetLastError());
        if (mapping)
            CloseHandle(mapping);
        CloseHandle(destination);
        FreeLibrary(zlib);
        return 0;
    }
    memcpy(destination_data, header.data(), header.size());
    z_stream stream{};
    stream.next_out = destination_data + map_header_size;
    stream.avail_out = decompressed_size - (uInt)map_header_size;
    int zlib_result = inflate_init(&stream, ZLIB_VERSION, sizeof(stream));
    while (zlib_result == Z_OK) {
        source.read((char *)compressed.get(), (std::streamsize)input_buffer_size);
        const auto received = source.gcount();
        if (received <= 0)
            break;
        stream.next_in = compressed.get();
        stream.avail_in = (uInt)received;
        while (stream.avail_in && zlib_result == Z_OK)
            zlib_result = inflate_data(&stream, Z_NO_FLUSH);
    }
    const auto expanded_size = stream.total_out;
    inflate_end(&stream);
    source.close();
    const bool expanded = zlib_result == Z_STREAM_END && expanded_size == decompressed_size - map_header_size &&
                          FlushViewOfFile(destination_data, decompressed_size) != FALSE;
    UnmapViewOfFile(destination_data);
    CloseHandle(mapping);
    CloseHandle(destination);
    FreeLibrary(zlib);
    if (!expanded) {
        host_logf(HOST_LOG_ERROR, "native cache: decompression failed result=%d actual=%llu expected=%zu error=%lu",
                  zlib_result, (unsigned long long)expanded_size, decompressed_size - map_header_size, GetLastError());
        return 0;
    }
    return 1;
}

static LONG WINAPI log_unhandled_exception(EXCEPTION_POINTERS *details) {
    if (details && details->ExceptionRecord && details->ContextRecord) {
        const EXCEPTION_RECORD *record = details->ExceptionRecord;
        const CONTEXT *context = details->ContextRecord;
        host_logf(HOST_LOG_ERROR,
                  "unhandled exception code=%08lx address=%p rip=%016llx rsp=%016llx "
                  "rax=%016llx rbx=%016llx rcx=%016llx rdx=%016llx",
                  record->ExceptionCode, record->ExceptionAddress, context->Rip, context->Rsp, context->Rax,
                  context->Rbx, context->Rcx, context->Rdx);
    } else {
        host_logf(HOST_LOG_ERROR, "unhandled exception without context");
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

extern "C" void host_logf(int priority, const char *format, ...) {
    char message[2048];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    std::lock_guard<std::mutex> guard(log_mutex);
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
    if (log_file) {
        fprintf(log_file, "[%d] %s\n", priority, message);
        fflush(log_file);
    }
}

extern "C" void GUEST_ABI host_log(int priority, const char *text) { host_logf(priority, "%s", text); }
extern "C" void GUEST_ABI host_abort(const char *reason) { host_fatal("guest abort: %s", reason); }
extern "C" void GUEST_ABI host_exit(int code) {
    host_logf(HOST_LOG_INFO, "game exited (%d)", code);
    ExitProcess((UINT)code);
}
extern "C" int GUEST_ABI host_errno(void) { return errno; }
extern "C" void host_fatal(const char *format, ...) {
    char message[2048];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    host_logf(HOST_LOG_ERROR, "%s", message);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Warthog", message, NULL);
    ExitProcess(1);
}

extern "C" void GUEST_ABI host_android_path(int which, char *buffer, uint32_t size) {
    if (size) {
        strncpy_s(buffer, size, (which ? save_root : data_root).c_str(), _TRUNCATE);
    }
}

static bool has_maps(const std::filesystem::path &root) {
    std::error_code error;
    return std::filesystem::is_regular_file(root / "maps" / "ui.map", error);
}

static std::filesystem::path package_directory() {
    wchar_t module[MAX_PATH];
    GetModuleFileNameW(NULL, module, MAX_PATH);
    return std::filesystem::path(module).parent_path();
}

struct environment {
    std::vector<std::string> entries;
};
static uint32_t make_boot(const environment &env) {
    const size_t capacity = 0x10000;
    char *memory = (char *)host_low_map(capacity, 3);
    auto *boot = (halo_guest_boot *)memory;
    uint32_t *argv = (uint32_t *)(memory + sizeof(*boot));
    uint32_t *environment_list = argv + 2;
    char *text = (char *)(environment_list + env.entries.size() + 1);
    if (!memory)
        host_fatal("cannot allocate guest environment");
    strcpy_s(text, capacity - (text - memory), "halo");
    argv[0] = (uint32_t)(uintptr_t)text;
    argv[1] = 0;
    text += 5;
    for (size_t i = 0; i < env.entries.size(); ++i) {
        size_t length = env.entries[i].size() + 1;
        memcpy(text, env.entries[i].c_str(), length);
        environment_list[i] = (uint32_t)(uintptr_t)text;
        text += length;
    }
    environment_list[env.entries.size()] = 0;
    boot->argc = 1;
    boot->argv = (uint32_t)(uintptr_t)argv;
    boot->environment = (uint32_t)(uintptr_t)environment_list;
    boot->page_size = 4096;
    return (uint32_t)(uintptr_t)boot;
}

static std::vector<unsigned char> read_package_file(const wchar_t *name) {
    std::filesystem::path path = package_directory() / name;
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input)
        return {};
    auto size = input.tellg();
    input.seekg(0);
    std::vector<unsigned char> data((size_t)size);
    input.read((char *)data.data(), size);
    return data;
}

extern "C" int SDL_main(int, char **) {
    auto local_w = winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path();
    auto local_cache_w = winrt::Windows::Storage::ApplicationData::Current().LocalCacheFolder().Path();
    std::filesystem::path local(local_w.c_str());
    std::filesystem::path local_cache(local_cache_w.c_str());
    std::filesystem::path internal = local / L"Warthog";
    open_log_at(internal);
    std::error_code internal_maps_error;
    std::filesystem::create_directories(internal / L"custom_maps", internal_maps_error);
    if (internal_maps_error)
        host_logf(HOST_LOG_WARN, "could not create internal custom_maps: %s", internal_maps_error.message().c_str());
    SetUnhandledExceptionFilter(log_unhandled_exception);
    host_logf(HOST_LOG_INFO, "Warthog UWP 0.7.5 starting");
    SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "1");
    SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "0");
    int physical_width = 1920, physical_height = 1080;
    uwp_SetScreenSize(physical_width, physical_height);
    host_sdl_set_backbuffer_size(physical_width, physical_height);
    for (const wchar_t *library : {L"libuwp.dll", L"libgallium_wgl.dll", L"opengl32.dll"}) {
        SetLastError(ERROR_SUCCESS);
        HMODULE module = LoadPackagedLibrary(library, 0);
        if (!module)
            host_logf(HOST_LOG_ERROR, "LoadPackagedLibrary(%ls) failed: %lu", library, GetLastError());
    }

    std::vector<std::filesystem::path> data_roots{internal};
    for (wchar_t drive = L'D'; drive <= L'Z'; ++drive) {
        std::wstring path{drive, L':', L'\\'};
        std::filesystem::path root = std::filesystem::path(path) / L"Warthog";
        std::error_code error;
        if (std::filesystem::is_directory(root, error))
            data_roots.push_back(std::move(root));
    }
    std::filesystem::path selected_data;
    xbox_restore_game_data(internal, selected_data);
    if (selected_data.empty()) {
        for (const auto &root : data_roots) {
            if (has_maps(root)) {
                selected_data = root;
                xbox_remember_game_data(internal, selected_data);
                break;
            }
        }
    }
    if (selected_data.empty()) {
        const bool installed = xbox_install_game_data(internal, selected_data);
        uwp_SetScreenSize(physical_width, physical_height);
        host_sdl_set_backbuffer_size(physical_width, physical_height);
        if (!installed)
            return 2;
    }
    if (selected_data.empty() || !has_maps(selected_data)) {
        host_logf(HOST_LOG_ERROR, "no maps or usable disc image in internal/removable Warthog folders");
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, "Warthog game data needed",
            "Game setup was not completed. Restart Warthog and choose your legally owned Halo .iso/.xiso, or "
            "provide an existing maps folder in LocalState\\Warthog or USB:\\Warthog.",
            NULL);
        return 2;
    }
    for (const wchar_t *folder : {L"custom_maps", L"maps_ce", L"maps_md", L"maps_pc"}) {
        std::error_code folder_error;
        std::filesystem::create_directories(selected_data / folder, folder_error);
        if (folder_error)
            host_logf(HOST_LOG_WARN, "could not create %ls at %s: %s", folder,
                      winrt::to_string(selected_data.wstring()).c_str(), folder_error.message().c_str());
    }
    prepare_portable_storage(internal, selected_data);
    const bool portable = !same_path(internal, selected_data);
    const auto writable_root = portable ? selected_data : internal;
    if (portable) {
        host_logf(HOST_LOG_INFO, "continuing log at %s", winrt::to_string(writable_root.wstring()).c_str());
        open_log_at(writable_root);
    }
    std::error_code save_error;
    std::filesystem::create_directories(writable_root / L"save", save_error);
    const auto map_cache_root = writable_root / L"map_cache";
    const auto shader_cache_root = local_cache / L"Warthog" / L"shader_cache";
    std::error_code map_cache_error;
    std::error_code shader_cache_error;
    std::filesystem::create_directories(map_cache_root, map_cache_error);
    std::filesystem::create_directories(shader_cache_root, shader_cache_error);
    data_root = winrt::to_string(selected_data.wstring());
    save_root = winrt::to_string((writable_root / L"save").wstring());
    environment env;
    env.entries.push_back("HOME=" + save_root);
    env.entries.push_back("HALO_DATA_ROOT=" + data_root);
    env.entries.push_back("HALO_SAVE_ROOT=" + save_root);
    env.entries.push_back("HALO_CONFIG_ROOT=" + winrt::to_string(writable_root.wstring()));
    if (!map_cache_error) {
        env.entries.push_back("HALO_MAP_CACHE_ROOT=" + winrt::to_string(map_cache_root.wstring()));
        host_logf(HOST_LOG_INFO, "map cache root: %s", winrt::to_string(map_cache_root.wstring()).c_str());
    } else {
        host_logf(HOST_LOG_WARN, "map cache unavailable: %s", map_cache_error.message().c_str());
    }
    if (!shader_cache_error) {
        env.entries.push_back("HALO_SHADER_CACHE_ROOT=" + winrt::to_string(shader_cache_root.wstring()));
        host_logf(HOST_LOG_INFO, "shader cache root: %s", winrt::to_string(shader_cache_root.wstring()).c_str());
    } else {
        host_logf(HOST_LOG_WARN, "internal shader cache unavailable: %s", shader_cache_error.message().c_str());
    }
    env.entries.push_back("HALO_UPDATE_AUTO=false");
    env.entries.push_back(
        "HALO_NET_BROKERS_FILE=" +
        winrt::to_string((package_directory() / L"Warthog" / L"network" / L"brokers.txt").wstring()));
    int game_width = (480 * physical_width + physical_height / 2) / physical_height;
    if (game_width < 640)
        game_width = 640;
    if (game_width > 1600)
        game_width = 1600;
    game_width &= ~1;
    env.entries.push_back("HALO_DISPLAY_WIDTH=" + std::to_string(game_width));
    env.entries.push_back("TZ=UTC0");

    auto image = read_package_file(L"halo_guest.elf");
    if (image.empty())
        host_fatal("halo_guest.elf is missing from the installed package");
    if (host_load_image(image.data(), image.size()))
        host_fatal("could not load the x32 game image; see Warthog\\warthog.log");
    uint32_t boot = make_boot(env);
    host_logf(HOST_LOG_INFO, "data=%s saves=%s config=%s log=%s", data_root.c_str(), save_root.c_str(),
              winrt::to_string(writable_root.wstring()).c_str(),
              winrt::to_string((writable_root / L"warthog.log").wstring()).c_str());
    host_run_guest_main(boot);
    return 0;
}
