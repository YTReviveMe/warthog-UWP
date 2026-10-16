#pragma once

#include "halo_android_abi.h"
#include <stddef.h>
#include <stdint.h>

#if defined(__clang__) && defined(_M_X64)
#define GUEST_ABI __attribute__((sysv_abi))
#else
#define GUEST_ABI
#endif

#define HOST_LOG_INFO 4
#define HOST_LOG_WARN 5
#define HOST_LOG_ERROR 6

#ifdef __cplusplus
extern "C" {
#endif

struct host_guest_image {
    const struct halo_guest_header *header;
    uint32_t base, end;
};

extern struct host_guest_image host_image;

void host_logf(int priority, const char *format, ...);
void GUEST_ABI host_log(int priority, const char *text);
void GUEST_ABI host_abort(const char *reason);
void GUEST_ABI host_exit(int code);
int GUEST_ABI host_errno(void);
int GUEST_ABI host_decompress_map(const char *source_path, const char *destination_path, uint32_t decompressed_size,
                                  uint32_t destination_size);
void host_fatal(const char *format, ...);

int host_load_image(const void *elf, size_t size);
int host_memory_initialize(uint32_t image_base, uint32_t image_size);
void *host_low_map(size_t size, int protection);
void host_low_unmap(void *address, size_t size);
int host_low_owns(uintptr_t address, size_t size);
long GUEST_ABI host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset);
long GUEST_ABI host_guest_munmap(uint64_t address, uint64_t size);
long GUEST_ABI host_guest_mprotect(uint64_t address, uint64_t size, int protection);

uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size);
void host_run_guest_main(uint32_t boot);
uint32_t GUEST_ABI host_get_tp(void);
void GUEST_ABI host_set_tp(uint32_t thread);
uint64_t GUEST_ABI host_native_stack_get(void);
int GUEST_ABI host_thread_create(uint32_t guest_thread, uint32_t stack_size);

void *host_resolve_import(const char *name);
void *host_gl_resolve(const char *name);
void host_sdl_set_backbuffer_size(int width, int height);

void GUEST_ABI host_memory_watch_initialize(void);
void GUEST_ABI host_memory_watch_protect(uint32_t address, uint32_t size);
uint32_t GUEST_ABI host_memory_watch_generation(uint32_t address, uint32_t size);
uint32_t GUEST_ABI host_memory_watch_serial(void);
void GUEST_ABI host_memory_watch_prepare_write(uint32_t address, uint32_t size);
void GUEST_ABI host_memory_watch_forget(uint32_t address, uint32_t size);

#ifdef __cplusplus
}
#endif
