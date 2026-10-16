#include "host.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define HANDLE_COUNT 256
enum handle_type { HANDLE_FREE, HANDLE_WINDOW, HANDLE_CONTEXT, HANDLE_GAMEPAD, HANDLE_AUDIO };
struct handle { int type; void *object; };
static struct handle handles[HANDLE_COUNT];
static SRWLOCK handle_lock = SRWLOCK_INIT;
static int backbuffer_width = 1920, backbuffer_height = 1080;

void host_sdl_set_backbuffer_size(int width, int height) {
    if (width > 0 && height > 0) { backbuffer_width = width; backbuffer_height = height; }
}
static uint32_t handle_new(int type, void *object) {
    uint32_t i;
    if (!object) return 0;
    AcquireSRWLockExclusive(&handle_lock);
    for (i = 1; i < HANDLE_COUNT; ++i)
        if (handles[i].type == type && handles[i].object == object) break;
    if (i == HANDLE_COUNT)
        for (i = 1; i < HANDLE_COUNT; ++i)
            if (handles[i].type == HANDLE_FREE) { handles[i].type = type; handles[i].object = object; break; }
    ReleaseSRWLockExclusive(&handle_lock);
    if (i == HANDLE_COUNT) host_logf(HOST_LOG_ERROR, "out of SDL handles");
    return i < HANDLE_COUNT ? i : 0;
}
static void *handle_get(uint32_t value, int type) {
    return value && value < HANDLE_COUNT && handles[value].type == type ? handles[value].object : NULL;
}

int GUEST_ABI host_sdl_init(uint32_t flags) {
    int result = SDL_Init((SDL_InitFlags)flags);
    if (!result) host_logf(HOST_LOG_ERROR, "SDL_Init failed: %s", SDL_GetError());
    return result;
}
int GUEST_ABI host_sdl_set_hint(const char *name, const char *value) { return SDL_SetHint(name, value); }
void GUEST_ABI host_sdl_get_error(char *buffer, uint32_t size) {
    if (size) SDL_strlcpy(buffer, SDL_GetError(), size);
}
int64_t GUEST_ABI host_sdl_ticks(void) { return (int64_t)SDL_GetTicks(); }
int64_t GUEST_ABI host_sdl_thread_id(void) { return (int64_t)GetCurrentThreadId(); }
void GUEST_ABI host_sdl_scancode_name(int scancode, char *buffer, uint32_t size) {
    if (size) SDL_strlcpy(buffer, SDL_GetScancodeName((SDL_Scancode)scancode), size);
}
int GUEST_ABI host_sdl_scancode_from_name(const char *name) { return (int)SDL_GetScancodeFromName(name); }

uint32_t GUEST_ABI host_sdl_create_window(const char *title, int width, int height, int64_t flags) {
    uint32_t result = handle_new(HANDLE_WINDOW, SDL_CreateWindow(title, width, height, (SDL_WindowFlags)flags));
    if (!result) host_logf(HOST_LOG_ERROR, "SDL_CreateWindow failed: %s", SDL_GetError());
    return result;
}
void GUEST_ABI host_sdl_window_size_in_pixels(uint32_t window, int *width, int *height) {
    (void)window;
    *width = backbuffer_width; *height = backbuffer_height;
}
int GUEST_ABI host_sdl_set_relative_mouse(uint32_t window, int enabled) {
    SDL_Window *object = (SDL_Window *)handle_get(window, HANDLE_WINDOW);
    return object ? SDL_SetWindowRelativeMouseMode(object, enabled != 0) : 0;
}
int GUEST_ABI host_sdl_gl_set_attribute(int attribute, int value) {
    return SDL_GL_SetAttribute((SDL_GLAttr)attribute, value);
}
uint32_t GUEST_ABI host_sdl_gl_create_context(uint32_t window) {
    SDL_Window *object = (SDL_Window *)handle_get(window, HANDLE_WINDOW);
    uint32_t result = object ? handle_new(HANDLE_CONTEXT, SDL_GL_CreateContext(object)) : 0;
    if (!result) host_logf(HOST_LOG_ERROR, "SDL_GL_CreateContext failed: %s", SDL_GetError());
    return result;
}
int GUEST_ABI host_sdl_gl_make_current(uint32_t window, uint32_t context) {
    return SDL_GL_MakeCurrent((SDL_Window *)handle_get(window, HANDLE_WINDOW), handle_get(context, HANDLE_CONTEXT));
}
int GUEST_ABI host_sdl_gl_set_swap_interval(int interval) {
    int accepted = SDL_GL_SetSwapInterval(interval);
    if (!accepted) host_logf(HOST_LOG_WARN, "Xbox presentation: swap interval %d rejected: %s", interval, SDL_GetError());
    return accepted;
}
int GUEST_ABI host_sdl_gl_swap_window(uint32_t window) {
    SDL_Window *object = (SDL_Window *)handle_get(window, HANDLE_WINDOW);
    return object ? SDL_GL_SwapWindow(object) : 0;
}
int GUEST_ABI host_sdl_poll_event(void *output) {
    SDL_Event event;
    if (!SDL_PollEvent(&event)) return 0;
    memcpy(output, &event, sizeof(event));
    return 1;
}

int GUEST_ABI host_sdl_get_gamepads(uint32_t *ids, int capacity) {
    int count = 0, i;
    SDL_JoystickID *list = SDL_GetGamepads(&count);
    if (!list) return 0;
    if (count > capacity) count = capacity;
    for (i = 0; i < count; ++i) ids[i] = (uint32_t)list[i];
    SDL_free(list);
    return count;
}
static SDL_Gamepad *controller_from_instance(SDL_JoystickID id) {
    SDL_Gamepad *controller = SDL_GetGamepadFromID(id);
    return controller ? controller : SDL_OpenGamepad(id);
}
uint32_t GUEST_ABI host_sdl_open_gamepad(uint32_t id) {
    return handle_new(HANDLE_GAMEPAD, controller_from_instance((SDL_JoystickID)id));
}
uint32_t GUEST_ABI host_sdl_gamepad_from_id(uint32_t id) {
    return handle_new(HANDLE_GAMEPAD, controller_from_instance((SDL_JoystickID)id));
}
int GUEST_ABI host_sdl_gamepad_axis(uint32_t gamepad, int axis) {
    SDL_Gamepad *object = (SDL_Gamepad *)handle_get(gamepad, HANDLE_GAMEPAD);
    return object ? SDL_GetGamepadAxis(object, (SDL_GamepadAxis)axis) : 0;
}
int GUEST_ABI host_sdl_gamepad_button(uint32_t gamepad, int button) {
    SDL_Gamepad *object = (SDL_Gamepad *)handle_get(gamepad, HANDLE_GAMEPAD);
    return object ? SDL_GetGamepadButton(object, (SDL_GamepadButton)button) : 0;
}
int GUEST_ABI host_sdl_gamepad_type(uint32_t gamepad) {
    SDL_Gamepad *object = (SDL_Gamepad *)handle_get(gamepad, HANDLE_GAMEPAD);
    return object ? (int)SDL_GetGamepadType(object) : (int)SDL_GAMEPAD_TYPE_UNKNOWN;
}
int GUEST_ABI host_sdl_rumble_gamepad(uint32_t gamepad, uint32_t low, uint32_t high, uint32_t ms) {
    SDL_Gamepad *object = (SDL_Gamepad *)handle_get(gamepad, HANDLE_GAMEPAD);
    return object ? SDL_RumbleGamepad(object, (Uint16)low, (Uint16)high, ms) : 0;
}

void GUEST_ABI host_gesture_insets(int *insets) { memset(insets, 0, 4 * sizeof(*insets)); }
void GUEST_ABI host_touch_read(int *state) { memset(state, 0, 7 * sizeof(*state)); }
void GUEST_ABI host_touch_look_read(float *delta) { memset(delta, 0, 2 * sizeof(*delta)); }
void GUEST_ABI host_touch_rumble(unsigned int low, unsigned int high) { (void)low; (void)high; }
void GUEST_ABI host_touch_scene(int scene) { (void)scene; }
void GUEST_ABI host_touch_bindings(const int *controls) { (void)controls; }

struct guest_audio_spec { uint32_t format; int channels; int frequency; };
struct audio_binding {
    SDL_AudioStream *stream;
    uint32_t callback, userdata, handle;
    unsigned char *buffer;
    int buffer_length, buffer_capacity;
};
static __declspec(thread) struct audio_binding *audio_calling;

static int keep_callback_audio(struct audio_binding *binding, const void *data, int length) {
    if (length < 0) return 0;
    if (binding->buffer_length + length > binding->buffer_capacity) {
        int capacity = (binding->buffer_length + length) * 2;
        void *resized = realloc(binding->buffer, (size_t)capacity);
        if (!resized) return 0;
        binding->buffer = (unsigned char *)resized;
        binding->buffer_capacity = capacity;
    }
    memcpy(binding->buffer + binding->buffer_length, data, (size_t)length);
    binding->buffer_length += length;
    return 1;
}
static void SDLCALL audio_callback(void *userdata, SDL_AudioStream *stream, int additional, int total) {
    struct audio_binding *binding = (struct audio_binding *)userdata;
    audio_calling = binding;
    host_call_guest(binding->callback, binding->userdata, binding->handle, (uint32_t)additional, (uint32_t)total);
    audio_calling = NULL;
    if (binding->buffer_length) {
        SDL_PutAudioStreamData(stream, binding->buffer, binding->buffer_length);
        binding->buffer_length = 0;
    }
}
uint32_t GUEST_ABI host_sdl_open_audio_stream(uint32_t device, const void *spec_ptr, uint32_t callback,
                                              uint32_t userdata) {
    const struct guest_audio_spec *guest = (const struct guest_audio_spec *)spec_ptr;
    SDL_AudioSpec spec;
    struct audio_binding *binding = (struct audio_binding *)calloc(1, sizeof(*binding));
    if (!binding) return 0;
    SDL_zero(spec);
    spec.format = (SDL_AudioFormat)guest->format;
    spec.channels = guest->channels;
    spec.freq = guest->frequency;
    binding->callback = callback;
    binding->userdata = userdata;
    binding->stream = SDL_OpenAudioDeviceStream((SDL_AudioDeviceID)device, &spec,
                                                 callback ? audio_callback : NULL, binding);
    if (!binding->stream) {
        host_logf(HOST_LOG_ERROR, "Xbox audio: SDL3 stream open failed: %s", SDL_GetError());
        free(binding);
        return 0;
    }
    binding->handle = handle_new(HANDLE_AUDIO, binding);
    return binding->handle;
}
int GUEST_ABI host_sdl_put_audio_stream_data(uint32_t stream, const void *data, int length) {
    struct audio_binding *binding = (struct audio_binding *)handle_get(stream, HANDLE_AUDIO);
    if (!binding) return 0;
    if (binding == audio_calling) return keep_callback_audio(binding, data, length);
    return SDL_PutAudioStreamData(binding->stream, data, length);
}
int GUEST_ABI host_sdl_resume_audio_stream_device(uint32_t stream) {
    struct audio_binding *binding = (struct audio_binding *)handle_get(stream, HANDLE_AUDIO);
    return binding ? SDL_ResumeAudioStreamDevice(binding->stream) : 0;
}

int GUEST_ABI host_sdl_set_clipboard_text(const char *text) { return SDL_SetClipboardText(text); }
void GUEST_ABI host_sdl_get_clipboard_text(char *buffer, uint32_t size) {
    char *text = SDL_GetClipboardText();
    if (size) SDL_strlcpy(buffer, text ? text : "", size);
    SDL_free(text);
}
int GUEST_ABI host_sdl_show_toast(const char *message, int duration, int gravity, int x, int y) {
    (void)message; (void)duration; (void)gravity; (void)x; (void)y; return 0;
}
int GUEST_ABI host_sdl_show_simple_message_box(uint32_t flags, const char *title, const char *message) {
    return SDL_ShowSimpleMessageBox((SDL_MessageBoxFlags)flags, title, message, NULL);
}
int GUEST_ABI host_sdl_show_message_box(uint32_t flags, const char *title, const char *message, int count,
                                        const uint32_t *button_flags, const int32_t *button_ids,
                                        const uint32_t *button_texts) {
    SDL_MessageBoxButtonData buttons[8];
    SDL_MessageBoxData box;
    int i, answer = -1;
    if (count < 0 || count > 8) return -1;
    for (i = 0; i < count; ++i) {
        buttons[i].flags = (SDL_MessageBoxButtonFlags)button_flags[i];
        buttons[i].buttonID = button_ids[i];
        buttons[i].text = (const char *)(uintptr_t)button_texts[i];
    }
    SDL_zero(box);
    box.flags = (SDL_MessageBoxFlags)flags;
    box.title = title;
    box.message = message;
    box.numbuttons = count;
    box.buttons = buttons;
    return SDL_ShowMessageBox(&box, &answer) ? answer : -1;
}
int GUEST_ABI host_sdl_open_url(const char *url) { return SDL_OpenURL(url); }
