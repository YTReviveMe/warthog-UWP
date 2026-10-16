#include "host.h"
#include <intrin.h>
#include <stdlib.h>
#include <windows.h>

#define GUEST_STACK_GUARD 0x10000
#define GUEST_STACK_ARGUMENT_HEADROOM 0x1000

static __declspec(thread) uint32_t guest_tp;
static __declspec(thread) uintptr_t native_stack;
static __declspec(thread) void *guest_stack_mapping;
static __declspec(thread) size_t guest_stack_mapping_size;

extern uint32_t host_enter_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d, void *stack_top);

uint32_t GUEST_ABI host_get_tp(void) { return guest_tp; }
void GUEST_ABI host_set_tp(uint32_t value) { guest_tp = value; }
uint64_t GUEST_ABI host_native_stack_get(void) { return native_stack; }

static int ensure_guest_stack(size_t requested) {
    if (guest_stack_mapping)
        return 1;
    if (requested < 1024 * 1024)
        requested = 1024 * 1024;
    requested = (requested + 0xffff) & ~(size_t)0xffff;
    guest_stack_mapping_size = requested + GUEST_STACK_GUARD;
    guest_stack_mapping = host_low_map(guest_stack_mapping_size, 3);
    return guest_stack_mapping != NULL;
}

uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    if (!ensure_guest_stack(2 * 1024 * 1024))
        host_fatal("could not allocate a low guest stack");
    if (!guest_tp && function != host_image.header->thread_attach && function != host_image.header->thread_start)
        host_call_guest(host_image.header->thread_attach, 0, 0, 0, 0);
    native_stack = ((uintptr_t)_AddressOfReturnAddress() - 0x1000) & ~(uintptr_t)15;
    return host_enter_guest(function, a, b, c, d,
                            (char *)guest_stack_mapping + guest_stack_mapping_size - GUEST_STACK_ARGUMENT_HEADROOM);
}

void host_run_guest_main(uint32_t boot) {
    if (!ensure_guest_stack(2 * 1024 * 1024))
        host_fatal("could not allocate the main guest stack");
    native_stack = ((uintptr_t)_AddressOfReturnAddress() - 0x1000) & ~(uintptr_t)15;
    host_enter_guest(host_image.header->start, boot, 0, 0, 0,
                     (char *)guest_stack_mapping + guest_stack_mapping_size - GUEST_STACK_ARGUMENT_HEADROOM);
    host_fatal("the guest returned from its main entry point");
}

struct thread_start {
    void *(*function)(void *);
    void *argument;
    size_t stack_size;
};
static DWORD WINAPI thread_main(void *opaque) {
    struct thread_start start = *(struct thread_start *)opaque;
    free(opaque);
    if (!ensure_guest_stack(start.stack_size))
        return 1;
    start.function(start.argument);
    return 0;
}

int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size) {
    struct thread_start *start = (struct thread_start *)calloc(1, sizeof(*start));
    HANDLE thread;
    if (!start)
        return 12;
    start->function = function;
    start->argument = argument;
    start->stack_size = stack_size;
    thread = CreateThread(NULL, 0, thread_main, start, 0, NULL);
    if (!thread) {
        free(start);
        return 11;
    }
    CloseHandle(thread);
    return 0;
}

static void *guest_thread_main(void *thread) {
    host_call_guest(host_image.header->thread_start, (uint32_t)(uintptr_t)thread, 0, 0, 0);
    return NULL;
}

int GUEST_ABI host_thread_create(uint32_t thread, uint32_t stack_size) {
    return host_native_thread_create(guest_thread_main, (void *)(uintptr_t)thread, stack_size);
}
