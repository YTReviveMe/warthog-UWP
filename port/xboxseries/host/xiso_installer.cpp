#include "xiso_installer.h"
#include "host.h"
#include "setup_ui.h"

#include <SDL3/SDL.h>
#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <libuwp.h>
#include <memory>
#include <string>
#include <system_error>
#include <vector>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.AccessCache.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.Profile.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/base.h>

namespace {
constexpr std::uint64_t sector_size = 2048;
constexpr std::uint64_t descriptor_offset = 0x10000;
constexpr std::size_t entry_header_size = 14;
constexpr unsigned char directory_attribute = 0x10;
constexpr std::uint32_t maximum_directory_size = 4u << 20;
constexpr std::size_t maximum_files = 256;
constexpr std::size_t copy_buffer_size = 1u << 20;
constexpr std::uint32_t gl_color_buffer_bit = 0x00004000u;
constexpr std::array<std::uint64_t, 4> partition_offsets = {0, 0x0FD90000ull, 0x02080000ull, 0x18300000ull};
constexpr char volume_magic[] = "MICROSOFT*XBOX*MEDIA";
constexpr wchar_t data_access_token[] = L"opence-game-data";
constexpr wchar_t selection_record[] = L"selected-data-root.txt";

using winrt::Windows::Storage::StorageFolder;
using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
using winrt::Windows::UI::Core::CoreProcessEventsOption;
using winrt::Windows::UI::Core::CoreWindow;

struct image_candidate {
    std::filesystem::path image;
    std::filesystem::path destination;
};

struct xiso_file {
    std::string name;
    std::uint32_t sector = 0;
    std::uint32_t size = 0;
};

bool has_maps(const std::filesystem::path &root) {
    std::error_code error;
    return std::filesystem::is_regular_file(root / L"maps" / L"ui.map", error);
}

void save_selection(const std::filesystem::path &local_root, const std::filesystem::path &installed_root) {
    std::error_code error;
    std::filesystem::create_directories(local_root, error);
    std::ofstream record(local_root / selection_record, std::ios::trunc);
    record << winrt::to_string(installed_root.wstring());
}

struct restore_state {
    std::atomic<bool> finished{};
    StorageFolder destination{nullptr};
};

winrt::fire_and_forget restore_external_folder(std::shared_ptr<restore_state> state) {
    try {
        auto access = StorageApplicationPermissions::FutureAccessList();
        if (access.ContainsItem(data_access_token))
            state->destination = co_await access.GetFolderAsync(data_access_token);
    } catch (const winrt::hresult_error &error) {
        host_logf(HOST_LOG_WARN, "installer: saved external access failed: %s",
                  winrt::to_string(error.message()).c_str());
    }
    state->finished.store(true, std::memory_order_release);
}

StorageFolder restore_external_folder_sync() {
    const auto window = CoreWindow::GetForCurrentThread();
    if (!window)
        return nullptr;
    auto state = std::make_shared<restore_state>();
    restore_external_folder(state);
    const auto dispatcher = window.Dispatcher();
    while (!state->finished.load(std::memory_order_acquire)) {
        dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
        Sleep(16);
    }
    return state->destination;
}

std::uint32_t read_u32(const unsigned char *bytes) {
    return std::uint32_t(bytes[0]) | std::uint32_t(bytes[1]) << 8 | std::uint32_t(bytes[2]) << 16 |
           std::uint32_t(bytes[3]) << 24;
}

bool names_match(std::string a, std::string b) {
    auto lower = [](unsigned char c) { return char(std::tolower(c)); };
    std::transform(a.begin(), a.end(), a.begin(), lower);
    std::transform(b.begin(), b.end(), b.begin(), lower);
    return a == b;
}

bool read_at(std::ifstream &image, std::uint64_t offset, void *buffer, std::size_t size) {
    image.clear();
    image.seekg(static_cast<std::streamoff>(offset));
    image.read(static_cast<char *>(buffer), static_cast<std::streamsize>(size));
    return image.good();
}

struct directory_walk {
    const std::vector<unsigned char> &table;
    std::vector<xiso_file> &entries;
    bool directories;
    unsigned visited = 0;

    void walk(std::uint32_t offset_units, unsigned depth) {
        const std::size_t offset = std::size_t(offset_units) * 4;
        if (depth > 64 || ++visited > 4096 || offset + entry_header_size > table.size())
            return;
        const unsigned char *entry = table.data() + offset;
        const std::uint32_t left = std::uint32_t(entry[0]) | std::uint32_t(entry[1]) << 8;
        const std::uint32_t right = std::uint32_t(entry[2]) | std::uint32_t(entry[3]) << 8;
        if (left == 0xffff)
            return;
        if (left)
            walk(left, depth + 1);
        const std::size_t name_size = entry[13];
        const bool is_directory = (entry[12] & directory_attribute) != 0;
        if (offset + entry_header_size + name_size <= table.size() && name_size && is_directory == directories &&
            entries.size() < maximum_files) {
            std::string name(reinterpret_cast<const char *>(entry + entry_header_size), name_size);
            if (name != "." && name != ".." && name.find('/') == std::string::npos &&
                name.find('\\') == std::string::npos) {
                entries.push_back({name, read_u32(entry + 4), read_u32(entry + 8)});
            }
        }
        if (right)
            walk(right, depth + 1);
    }
};

bool read_directory(std::ifstream &image, std::uint64_t partition, std::uint32_t sector, std::uint32_t size,
                    std::vector<unsigned char> &table) {
    if (!size || size > maximum_directory_size)
        return false;
    table.resize(size);
    return read_at(image, partition + std::uint64_t(sector) * sector_size, table.data(), table.size());
}

class progress_window {
  public:
    progress_window() {
        const auto family = winrt::Windows::System::Profile::AnalyticsInfo::VersionInfo().DeviceFamily();
        if (family == L"Windows.Xbox")
            uwp_GetActualSize(&screen_width_, &screen_height_);
        if (screen_width_ <= 0 || screen_height_ <= 0) {
            screen_width_ = 1920;
            screen_height_ = 1080;
        }
        ui_scale_ = std::clamp(float(screen_height_) / 1080.0f, 1.0f, 2.0f);
        uwp_SetScreenSize(screen_width_, screen_height_);
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            host_logf(HOST_LOG_ERROR, "installer: SDL initialization failed: %s", SDL_GetError());
            return;
        }
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        window_ = SDL_CreateWindow("Warthog game data installer", screen_width_, screen_height_, SDL_WINDOW_OPENGL);
        if (!window_) {
            host_logf(HOST_LOG_ERROR, "installer: progress window failed: %s", SDL_GetError());
            return;
        }
        context_ = SDL_GL_CreateContext(window_);
        if (!context_ || !SDL_GL_MakeCurrent(window_, context_)) {
            host_logf(HOST_LOG_ERROR, "installer: progress context failed: %s", SDL_GetError());
            return;
        }
        SDL_GL_SetSwapInterval(1);
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        int logical_width = 0, logical_height = 0;
        SDL_GetWindowSize(window_, &logical_width, &logical_height);
        ui_scale_ = std::clamp(float(logical_height) / 1080.0f, 0.5f, 2.0f);
        io.FontGlobalScale = ui_scale_;
        ImGui::StyleColorsDark();
        auto &style = ImGui::GetStyle();
        style.WindowRounding = 10.0f;
        style.FrameRounding = 7.0f;
        style.WindowPadding = ImVec2(28.0f, 24.0f);
        style.ItemSpacing = ImVec2(12.0f, 16.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.025f, 0.075f, 0.032f, 1.0f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.12f, 0.58f, 0.16f, 0.85f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.08f, 0.68f, 0.12f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.07f, 0.14f, 0.08f, 1.0f);
        style.ScaleAllSizes(ui_scale_);
        imgui_ready_ = ImGui_ImplSDL3_InitForOpenGL(window_, context_) && ImGui_ImplOpenGL3_Init("#version 120");
        clear_color_ = reinterpret_cast<clear_color_proc>(SDL_GL_GetProcAddress("glClearColor"));
        clear_ = reinterpret_cast<clear_proc>(SDL_GL_GetProcAddress("glClear"));
        viewport_ = reinterpret_cast<viewport_proc>(SDL_GL_GetProcAddress("glViewport"));
        draw(0, 1);
    }

    ~progress_window() {
        if (imgui_ready_) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
        }
        if (ImGui::GetCurrentContext())
            ImGui::DestroyContext();
        if (context_)
            SDL_GL_DestroyContext(context_);
        if (window_)
            SDL_DestroyWindow(window_);
    }

    bool draw(std::uint64_t done, std::uint64_t total) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (imgui_ready_)
                ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                return false;
        }
        if (!context_ || !clear_color_ || !clear_ || !viewport_ || !imgui_ready_)
            return true;
        if (auto core = CoreWindow::GetForCurrentThread())
            core.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
        const auto percent = total ? std::min(done, total) * 100 / total : 0;
        char title[96];
        std::snprintf(title, sizeof(title), "Warthog game data installer - %llu%%",
                      static_cast<unsigned long long>(percent));
        SDL_SetWindowTitle(window_, title);
        viewport_(0, 0, screen_width_, screen_height_);
        clear_color_(4.0f / 255.0f, 12.0f / 255.0f, 5.0f / 255.0f, 1.0f);
        clear_(gl_color_buffer_bit);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        int logical_width = 0, logical_height = 0;
        SDL_GetWindowSize(window_, &logical_width, &logical_height);
        auto &io = ImGui::GetIO();
        if (logical_width > 0 && logical_height > 0) {
            io.DisplaySize = ImVec2(float(logical_width), float(logical_height));
            io.DisplayFramebufferScale = ImVec2(float(screen_width_) / logical_width,
                                                float(screen_height_) / logical_height);
        }
        ImGui::NewFrame();
        const ImVec2 card_size(io.DisplaySize.x * 0.80f, 176.0f * ui_scale_);
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.93f), ImGuiCond_Always,
                                ImVec2(0.5f, 1.0f));
        ImGui::SetNextWindowSize(card_size);
        ImGui::Begin("Installing Warthog game data", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextColored(ImVec4(0.22f, 0.82f, 0.26f, 1.0f), "Warthog");
        ImGui::SameLine();
        ImGui::TextUnformatted("  Installing Halo game data");
        ImGui::TextWrapped("Please keep Warthog open while the required map files are extracted.");
        char overlay[24];
        std::snprintf(overlay, sizeof(overlay), "%llu%%", static_cast<unsigned long long>(percent));
        ImGui::ProgressBar(total ? float(double(std::min(done, total)) / double(total)) : 0.0f,
                           ImVec2(-1.0f, 46.0f * ui_scale_), overlay);
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window_);
        return true;
    }

  private:
    using clear_color_proc = void (*)(float, float, float, float);
    using clear_proc = void (*)(unsigned int);
    using viewport_proc = void (*)(int, int, int, int);
    SDL_Window *window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    clear_color_proc clear_color_ = nullptr;
    clear_proc clear_ = nullptr;
    viewport_proc viewport_ = nullptr;
    bool imgui_ready_ = false;
    int screen_width_ = 1920;
    int screen_height_ = 1080;
    float ui_scale_ = 1.0f;
};

bool extract_maps(const image_candidate &candidate, std::string &error) {
    std::ifstream image(candidate.image, std::ios::binary);
    if (!image) {
        error = "The selected disc image could not be opened.";
        return false;
    }

    std::uint64_t partition = 0;
    std::uint32_t root_sector = 0, root_size = 0;
    bool found_volume = false;
    std::array<unsigned char, sector_size> descriptor{};
    for (const auto offset : partition_offsets) {
        if (read_at(image, offset + descriptor_offset, descriptor.data(), descriptor.size()) &&
            !std::memcmp(descriptor.data(), volume_magic, 20) &&
            !std::memcmp(descriptor.data() + 0x7ec, volume_magic, 20)) {
            partition = offset;
            root_sector = read_u32(descriptor.data() + 20);
            root_size = read_u32(descriptor.data() + 24);
            found_volume = true;
            break;
        }
    }
    if (!found_volume) {
        error = "This is not a supported Xbox disc image.";
        return false;
    }

    std::vector<unsigned char> table;
    if (!read_directory(image, partition, root_sector, root_size, table)) {
        error = "The disc image's root directory is damaged.";
        return false;
    }
    std::vector<xiso_file> directories;
    directory_walk root_walk{table, directories, true};
    root_walk.walk(0, 0);
    auto maps = std::find_if(directories.begin(), directories.end(),
                             [](const xiso_file &entry) { return names_match(entry.name, "maps"); });
    if (maps == directories.end() || !read_directory(image, partition, maps->sector, maps->size, table)) {
        error = "This disc image does not contain a readable Halo maps folder.";
        return false;
    }

    std::vector<xiso_file> files;
    directory_walk maps_walk{table, files, false};
    maps_walk.walk(0, 0);
    const bool has_ui = std::any_of(files.begin(), files.end(),
                                    [](const xiso_file &entry) { return names_match(entry.name, "ui.map"); });
    if (!has_ui) {
        error = "The image has no maps/ui.map and is not a usable Halo disc image.";
        return false;
    }
    std::uint64_t total = 0;
    for (const auto &file : files)
        total += file.size;
    std::error_code filesystem_error;
    const auto available = std::filesystem::space(candidate.destination, filesystem_error).available;
    if (!filesystem_error && available < total + (64ull << 20)) {
        error = "There is not enough free space to extract the Halo maps (about 2 GB required).";
        return false;
    }

    const auto partial = candidate.destination / L"maps.installing";
    const auto final = candidate.destination / L"maps";
    std::filesystem::create_directories(partial, filesystem_error);
    if (filesystem_error) {
        error = "The maps installation folder could not be created.";
        return false;
    }
    progress_window progress;
    std::vector<unsigned char> buffer(copy_buffer_size);
    std::uint64_t done = 0;
    for (const auto &file : files) {
        const auto output_path = partial / winrt::to_hstring(file.name).c_str();
        std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
        if (!output) {
            error = "A map file could not be created. Check the destination and free space.";
            return false;
        }
        std::uint64_t offset = partition + std::uint64_t(file.sector) * sector_size;
        std::uint32_t remaining = file.size;
        host_logf(HOST_LOG_INFO, "installer: extracting maps/%s (%u bytes)", file.name.c_str(), file.size);
        while (remaining) {
            const std::size_t count = std::min<std::size_t>(remaining, buffer.size());
            if (!read_at(image, offset, buffer.data(), count)) {
                error = "The disc image ended while reading a map file.";
                return false;
            }
            output.write(reinterpret_cast<const char *>(buffer.data()), static_cast<std::streamsize>(count));
            if (!output) {
                error = "A map file could not be written. Check the destination and free space.";
                return false;
            }
            offset += count;
            remaining -= static_cast<std::uint32_t>(count);
            done += count;
            if (!progress.draw(done, total)) {
                error = "Installation was cancelled.";
                return false;
            }
        }
    }
    std::filesystem::rename(partial, final, filesystem_error);
    if (filesystem_error) {
        error = "The completed maps folder could not be activated.";
        return false;
    }
    std::ofstream record(candidate.destination / L"installed-from.txt", std::ios::trunc);
    record << winrt::to_string(candidate.image.wstring());
    host_logf(HOST_LOG_INFO, "installer: extracted %.1f MiB to %s", double(total) / (1024.0 * 1024.0),
              winrt::to_string(final.wstring()).c_str());
    return true;
}

} // namespace

bool xbox_install_game_data(const std::filesystem::path &local_root, std::filesystem::path &installed_root) {
    image_candidate candidate;
    std::string error;
    if (!xbox_show_setup_ui(local_root, candidate.image, candidate.destination, error)) {
        if (!error.empty())
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Warthog setup could not start", error.c_str(), nullptr);
        return false;
    }
    uwp_SetScreenSize(1920, 1080);
    host_sdl_set_backbuffer_size(1920, 1080);
    host_logf(HOST_LOG_INFO, "installer: selected %s", winrt::to_string(candidate.image.wstring()).c_str());
    if (!extract_maps(candidate, error)) {
        host_logf(HOST_LOG_ERROR, "installer: %s", error.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Warthog installation failed", error.c_str(), nullptr);
        return false;
    }
    installed_root = candidate.destination;
    save_selection(local_root, installed_root);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Warthog installation complete",
                             "The Halo maps were installed successfully. Warthog will start now.", nullptr);
    return true;
}

bool xbox_restore_game_data(const std::filesystem::path &local_root, std::filesystem::path &installed_root) {
    std::ifstream record(local_root / selection_record);
    std::string saved;
    std::getline(record, saved);
    if (!saved.empty()) {
        auto root = std::filesystem::path(winrt::to_hstring(saved).c_str());
        if (has_maps(root)) {
            installed_root = std::move(root);
            host_logf(HOST_LOG_INFO, "installer: restored remembered data root %s", saved.c_str());
            return true;
        }
        host_logf(HOST_LOG_WARN, "installer: remembered data root is unavailable: %s", saved.c_str());
    }
    const auto external = restore_external_folder_sync();
    if (external) {
        auto root = std::filesystem::path(external.Path().c_str());
        if (has_maps(root)) {
            installed_root = std::move(root);
            save_selection(local_root, installed_root);
            host_logf(HOST_LOG_INFO, "installer: restored external data access %s",
                      winrt::to_string(installed_root.wstring()).c_str());
            return true;
        }
    }
    return false;
}

void xbox_remember_game_data(const std::filesystem::path &local_root, const std::filesystem::path &installed_root) {
    save_selection(local_root, installed_root);
}
