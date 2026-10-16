#include "setup_ui.h"
#include "host.h"

#include <SDL3/SDL.h>
#include <Windows.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <libuwp.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.AccessCache.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.Profile.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/base.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {
using namespace winrt;
using namespace Windows::Storage;
using namespace Windows::Storage::AccessCache;
using namespace Windows::UI::Core;

constexpr wchar_t data_access_token[] = L"opence-game-data";

struct file_entry {
    std::string name;
    std::string path;
    bool folder = false;
};

enum class browse_action { internal, external, child, parent };

struct launcher_state {
    StorageFolder folder{nullptr};
    std::vector<file_entry> entries;
    std::string current_folder_path;
    std::string selected_image_path;
    std::string destination_path;
    std::string error_message;
    bool busy = false;
    bool install = false;
    bool focus_internal = true;
    bool focus_install = false;
};

bool path_is_within(std::filesystem::path child, std::filesystem::path parent) {
    auto child_text = child.lexically_normal().wstring();
    auto parent_text = parent.lexically_normal().wstring();
    std::transform(child_text.begin(), child_text.end(), child_text.begin(), ::towlower);
    std::transform(parent_text.begin(), parent_text.end(), parent_text.begin(), ::towlower);
    if (!parent_text.empty() && parent_text.back() != L'\\')
        parent_text.push_back(L'\\');
    return child_text.size() >= parent_text.size() && child_text.compare(0, parent_text.size(), parent_text) == 0;
}

bool is_image_name(const hstring &name) {
    auto extension = std::filesystem::path(name.c_str()).extension().wstring();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    return extension == L".iso" || extension == L".xiso";
}

Windows::Foundation::IAsyncAction list_folder(const std::shared_ptr<launcher_state> &state, StorageFolder folder) {
    const auto items = co_await folder.GetItemsAsync();
    std::vector<file_entry> entries;
    for (const auto &item : items) {
        const bool is_folder = item.IsOfType(StorageItemTypes::Folder);
        if (is_folder || is_image_name(item.Name()))
            entries.push_back({to_string(item.Name()), to_string(item.Path()), is_folder});
    }
    std::sort(entries.begin(), entries.end(), [](const file_entry &left, const file_entry &right) {
        if (left.folder != right.folder)
            return left.folder > right.folder;
        return left.name < right.name;
    });
    state->folder = folder;
    state->current_folder_path = to_string(folder.Path());
    state->entries = std::move(entries);
}

Windows::Foundation::IAsyncAction select_file(const std::shared_ptr<launcher_state> &state, StorageFile file) {
    const auto local_folder = ApplicationData::Current().LocalFolder();
    const auto file_path = std::filesystem::path(file.Path().c_str());
    StorageFolder destination{nullptr};
    if (path_is_within(file_path, std::filesystem::path(local_folder.Path().c_str()))) {
        destination = co_await local_folder.CreateFolderAsync(L"Warthog", CreationCollisionOption::OpenIfExists);
    } else {
        const auto parent = co_await file.GetParentAsync();
        if (!parent)
            throw hresult_error(E_FAIL, L"The selected image's folder is unavailable.");
        destination = _wcsicmp(parent.Name().c_str(), L"Warthog") == 0
                          ? parent
                          : co_await parent.CreateFolderAsync(L"Warthog", CreationCollisionOption::OpenIfExists);
        StorageApplicationPermissions::FutureAccessList().AddOrReplace(data_access_token, destination);
    }
    co_await destination.CreateFolderAsync(L"custom_maps", CreationCollisionOption::OpenIfExists);
    state->selected_image_path = to_string(file.Path());
    state->destination_path = to_string(destination.Path());
    state->focus_install = true;
}

fire_and_forget browse(const std::shared_ptr<launcher_state> &state, browse_action action, std::string name = {}) {
    state->busy = true;
    state->error_message.clear();
    try {
        StorageFolder folder{nullptr};
        if (action == browse_action::internal) {
            folder = co_await ApplicationData::Current().LocalFolder().CreateFolderAsync(
                L"Warthog", CreationCollisionOption::OpenIfExists);
        } else if (action == browse_action::external) {
            const auto drives = KnownFolders::RemovableDevices();
            const auto roots = co_await drives.GetFoldersAsync();
            for (const auto &root : roots) {
                if (_wcsicmp(root.Path().c_str(), L"E:\\") == 0) {
                    folder = root;
                    break;
                }
            }
            if (!folder && roots.Size() == 1)
                folder = roots.GetAt(0);
            if (!folder && roots.Size() > 1)
                folder = drives;
            if (!folder) {
                state->error_message = "No USB storage was found. Connect a media-formatted drive and try again.";
            }
        } else if (action == browse_action::child) {
            folder = co_await state->folder.GetFolderAsync(to_hstring(name));
        } else if (action == browse_action::parent) {
            folder = co_await state->folder.GetParentAsync();
        }
        if (folder)
            co_await list_folder(state, folder);
    } catch (const hresult_error &error) {
        state->error_message = to_string(error.message());
    } catch (const std::exception &error) {
        state->error_message = error.what();
    }
    state->busy = false;
}

fire_and_forget choose_entry(const std::shared_ptr<launcher_state> &state, file_entry entry) {
    state->busy = true;
    state->error_message.clear();
    try {
        const auto file = co_await StorageFile::GetFileFromPathAsync(to_hstring(entry.path));
        co_await select_file(state, file);
    } catch (const hresult_error &error) {
        state->error_message = to_string(error.message());
    } catch (const std::exception &error) {
        state->error_message = error.what();
    }
    state->busy = false;
}

void style_launcher(float scale) {
    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(22.0f, 18.0f);
    style.ItemSpacing = ImVec2(12.0f, 10.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.018f, 0.035f, 0.020f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.030f, 0.070f, 0.035f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.12f, 0.58f, 0.16f, 0.70f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.055f, 0.32f, 0.075f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.08f, 0.52f, 0.11f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.68f, 0.16f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.07f, 0.38f, 0.09f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.09f, 0.55f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.12f, 0.68f, 0.16f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.82f, 0.24f, 1.0f);
    style.ScaleAllSizes(scale);
}
} // namespace

bool xbox_show_setup_ui(const std::filesystem::path &local_root, std::filesystem::path &image,
                        std::filesystem::path &destination, std::string &error) {
    error.clear();
    (void)local_root;
    int screen_width = 1920;
    int screen_height = 1080;
    const auto device_family = Windows::System::Profile::AnalyticsInfo::VersionInfo().DeviceFamily();
    if (device_family == L"Windows.Xbox")
        uwp_GetActualSize(&screen_width, &screen_height);
    if (screen_width <= 0 || screen_height <= 0) {
        screen_width = 1920;
        screen_height = 1080;
    }
    uwp_SetScreenSize(screen_width, screen_height);
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD)) {
        error = std::string("SDL initialization failed: ") + SDL_GetError();
        host_logf(HOST_LOG_ERROR, "setup UI: SDL initialization failed: %s", SDL_GetError());
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Window *window = SDL_CreateWindow("Warthog Setup", screen_width, screen_height, SDL_WINDOW_OPENGL);
    SDL_GLContext context = window ? SDL_GL_CreateContext(window) : nullptr;
    if (!window || !context || !SDL_GL_MakeCurrent(window, context)) {
        error = std::string("OpenGL initialization failed: ") + SDL_GetError();
        host_logf(HOST_LOG_ERROR, "setup UI: OpenGL window failed: %s", SDL_GetError());
        if (context)
            SDL_GL_DestroyContext(context);
        if (window)
            SDL_DestroyWindow(window);
        return false;
    }
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigNavCursorVisibleAlways = true;
    io.IniFilename = nullptr;
    int window_width = 0, window_height = 0;
    int drawable_width = 0, drawable_height = 0;
    SDL_GetWindowSize(window, &window_width, &window_height);
    SDL_GetWindowSizeInPixels(window, &drawable_width, &drawable_height);
    const float ui_scale = std::clamp(float(window_height) / 1080.0f, 0.5f, 2.0f);
    io.FontGlobalScale = ui_scale;
    style_launcher(ui_scale);
    if (!ImGui_ImplSDL3_InitForOpenGL(window, context) || !ImGui_ImplOpenGL3_Init("#version 120")) {
        error = "ImGui initialization failed.";
        host_logf(HOST_LOG_ERROR, "setup UI: ImGui initialization failed");
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        return false;
    }

    using viewport_proc = void (*)(int, int, int, int);
    using clear_color_proc = void (*)(float, float, float, float);
    using clear_proc = void (*)(unsigned int);
    const auto viewport = reinterpret_cast<viewport_proc>(SDL_GL_GetProcAddress("glViewport"));
    const auto clear_color = reinterpret_cast<clear_color_proc>(SDL_GL_GetProcAddress("glClearColor"));
    const auto clear = reinterpret_cast<clear_proc>(SDL_GL_GetProcAddress("glClear"));
    if (!viewport || !clear_color || !clear) {
        error = "Required OpenGL functions are unavailable.";
        host_logf(HOST_LOG_ERROR, "setup UI: required OpenGL functions are unavailable");
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        return false;
    }

    auto state = std::make_shared<launcher_state>();
    browse(state, browse_action::internal);
    bool quit = false;
    while (!quit && !state->install) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                quit = true;
        }
        if (auto core = CoreWindow::GetForCurrentThread())
            core.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        SDL_GetWindowSize(window, &window_width, &window_height);
        if (window_width > 0 && window_height > 0) {
            io.DisplaySize = ImVec2(float(window_width), float(window_height));
            io.DisplayFramebufferScale = ImVec2(float(screen_width) / window_width,
                                                float(screen_height) / window_height);
        }
        ImGui::NewFrame();

        const ImVec2 safe_pos(io.DisplaySize.x * 0.06f, io.DisplaySize.y * 0.06f);
        const ImVec2 safe_size(io.DisplaySize.x * 0.88f, io.DisplaySize.y * 0.88f);
        ImGui::SetNextWindowPos(safe_pos);
        ImGui::SetNextWindowSize(safe_size);
        ImGui::Begin("Warthog Setup", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::TextColored(ImVec4(0.22f, 0.82f, 0.26f, 1.0f), "Warthog");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.75f, 1.0f), "     UWP Port by: ReviveMe");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 6.0f * ui_scale));
        ImGui::BeginDisabled(state->busy);

        const float footer_height = 76.0f * ui_scale;
        const float content_height = std::max(1.0f, ImGui::GetContentRegionAvail().y - footer_height);
        ImGui::BeginChild("setup-page", ImVec2(0, content_height), ImGuiChildFlags_NavFlattened,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (ImGui::BeginTable("setup-layout", 2, ImGuiTableFlags_SizingStretchProp, ImGui::GetContentRegionAvail())) {
            ImGui::TableSetupColumn("Storage", ImGuiTableColumnFlags_WidthStretch, 1.45f);
            ImGui::TableSetupColumn("Install", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableNextColumn();
            ImGui::BeginChild("storage-card", ImVec2(0, 0),
                              ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened);
            ImGui::TextColored(ImVec4(0.22f, 0.82f, 0.26f, 1.0f), "GAME STORAGE");
            ImGui::TextColored(ImVec4(0.62f, 0.70f, 0.80f, 1.0f), "Halo: Combat Evolved Xbox - ISO/XISO");
            const float width = ImGui::GetContentRegionAvail().x;
            const float half = (width - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
            const bool internal_clicked = ImGui::Button("Internal storage", ImVec2(half, 38.0f * ui_scale));
            if (state->focus_internal && !state->busy) {
                ImGui::SetKeyboardFocusHere(-1);
                state->focus_internal = false;
            }
            if (internal_clicked)
                browse(state, browse_action::internal);
            ImGui::SameLine();
            if (ImGui::Button("External Storage", ImVec2(half, 38.0f * ui_scale)))
                browse(state, browse_action::external);
            ImGui::BeginDisabled(!state->folder);
            if (ImGui::Button("Up one folder", ImVec2(-1.0f, 34.0f * ui_scale)))
                browse(state, browse_action::parent);
            ImGui::EndDisabled();
            ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.75f, 1.0f), "%s",
                               state->current_folder_path.empty() ? "Choose a storage location"
                                                                  : state->current_folder_path.c_str());
            ImGui::BeginChild("files", ImVec2(0, 0),
                              ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened,
                              ImGuiWindowFlags_AlwaysVerticalScrollbar);
            for (const auto &entry : state->entries) {
                const auto label = std::string(entry.folder ? "[Folder]  " : "[Game]  ") + entry.name;
                if (ImGui::Selectable(label.c_str(), entry.path == state->selected_image_path)) {
                    if (entry.folder)
                        browse(state, browse_action::child, entry.name);
                    else
                        choose_entry(state, entry);
                    break;
                }
            }
            if (state->entries.empty())
                ImGui::TextWrapped("No ISO/XISO files found here. Choose another storage location or folder.");
            ImGui::EndChild();
            ImGui::EndChild();

            ImGui::TableNextColumn();
            ImGui::BeginChild("install-card", ImVec2(0, 0), true);
            ImGui::TextColored(ImVec4(0.22f, 0.82f, 0.26f, 1.0f), "FIRST-TIME SETUP");
            ImGui::TextWrapped("Select your legally owned Halo Xbox game image. Warthog extracts only the required maps "
                               "and remembers their location.");
            ImGui::Dummy(ImVec2(0.0f, 10.0f * ui_scale));
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.22f, 0.82f, 0.26f, 1.0f), "INSTALL LOCATION");
            if (!state->selected_image_path.empty()) {
                ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f), "Game image selected");
                const auto filename = std::filesystem::path(to_hstring(state->selected_image_path).c_str()).filename();
                ImGui::TextWrapped("%s", to_string(filename.wstring()).c_str());
                ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.75f, 1.0f), "%s", state->destination_path.c_str());
                ImGui::TextWrapped(
                    "About 2 GB of free space is required. The original image may be removed after installation.");
            } else {
                ImGui::TextColored(ImVec4(0.85f, 0.70f, 0.30f, 1.0f), "Select an .ISO or .XISO file to continue");
            }
            if (state->busy)
                ImGui::TextUnformatted("Reading storage...");
            if (!state->error_message.empty())
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.38f, 1.0f), "%s", state->error_message.c_str());
            ImGui::EndChild();
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::Separator();
        ImGui::BeginDisabled(state->selected_image_path.empty());
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.06f, 0.52f, 0.09f, 1.0f));
        if (state->focus_install && !state->busy)
            ImGui::SetKeyboardFocusHere();
        const float install_width = 300.0f * ui_scale;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - install_width);
        if (ImGui::Button("INSTALL GAME", ImVec2(install_width, 46.0f * ui_scale)))
            state->install = true;
        if (!state->busy)
            state->focus_install = false;
        ImGui::PopStyleColor();
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        ImGui::End();

        ImGui::Render();
        viewport(0, 0, screen_width, screen_height);
        clear_color(4.0f / 255.0f, 12.0f / 255.0f, 5.0f / 255.0f, 1.0f);
        clear(0x00004000u);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    if (state->install) {
        image = std::filesystem::path(to_hstring(state->selected_image_path).c_str());
        destination = std::filesystem::path(to_hstring(state->destination_path).c_str());
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return state->install;
}
