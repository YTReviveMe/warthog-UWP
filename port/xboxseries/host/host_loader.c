#include "host.h"
#include <string.h>
#include <windows.h>

#define EI_CLASS 4
#define ELFCLASS32 1
#define ET_EXEC 2
#define EM_X86_64 62
#define PT_LOAD 1
#define PF_X 1

#pragma pack(push, 1)
struct elf32_header {
    unsigned char ident[16];
    uint16_t type, machine;
    uint32_t version, entry, phoff, shoff, flags;
    uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
};
struct elf32_program {
    uint32_t type, offset, vaddr, paddr, filesz, memsz, flags, align;
};
#pragma pack(pop)

struct host_guest_image host_image;

static void GUEST_ABI missing_import(void) { host_fatal("the guest called an unavailable host import"); }

int host_load_image(const void *file, size_t size) {
    const struct elf32_header *elf = (const struct elf32_header *)file;
    const struct elf32_program *segments;
    uint64_t low = ~0ULL, high = 0;
    uint32_t i, count;
    const struct halo_guest_header *header;
    uint64_t *table;
    const char *name;
    if (size < sizeof(*elf) || memcmp(elf->ident, "\177ELF", 4) || elf->ident[EI_CLASS] != ELFCLASS32 ||
        elf->machine != EM_X86_64 || elf->type != ET_EXEC) {
        host_logf(HOST_LOG_ERROR, "guest image is not x86-64 x32 ELF");
        return -1;
    }
    if ((uint64_t)elf->phoff + (uint64_t)elf->phnum * elf->phentsize > size)
        return -1;
    segments = (const struct elf32_program *)((const char *)file + elf->phoff);
    for (i = 0; i < elf->phnum; ++i)
        if (segments[i].type == PT_LOAD) {
            if (segments[i].vaddr < low)
                low = segments[i].vaddr;
            if ((uint64_t)segments[i].vaddr + segments[i].memsz > high)
                high = (uint64_t)segments[i].vaddr + segments[i].memsz;
        }
    low &= ~0xfffULL;
    high = (high + 0xfff) & ~0xfffULL;
    if (low != HALO_GUEST_IMAGE_BASE || high > 0x100000000ULL)
        return -1;
    if (host_memory_initialize((uint32_t)low, (uint32_t)(high - low)))
        return -1;
    for (i = 0; i < elf->phnum; ++i)
        if (segments[i].type == PT_LOAD) {
            uintptr_t start = segments[i].vaddr & ~(uintptr_t)0xfff;
            uintptr_t end = (segments[i].vaddr + segments[i].memsz + 0xfff) & ~(uintptr_t)0xfff;
            if ((uint64_t)segments[i].offset + segments[i].filesz > size ||
                !VirtualAllocFromApp((void *)start, end - start, MEM_COMMIT, PAGE_READWRITE))
                return -1;
            memcpy((void *)(uintptr_t)segments[i].vaddr, (const char *)file + segments[i].offset, segments[i].filesz);
            if (segments[i].memsz > segments[i].filesz)
                memset((void *)(uintptr_t)(segments[i].vaddr + segments[i].filesz), 0,
                       segments[i].memsz - segments[i].filesz);
        }
    header = (const struct halo_guest_header *)(uintptr_t)low;
    if (header->magic != HALO_GUEST_MAGIC || header->abi_version != HALO_GUEST_ABI_VERSION)
        return -1;
    host_image.header = header;
    host_image.base = (uint32_t)low;
    host_image.end = (uint32_t)high;
    table = (uint64_t *)(uintptr_t)header->import_table;
    name = (const char *)(uintptr_t)header->import_names;
    count = *(const uint32_t *)(uintptr_t)header->import_count;
    for (i = 0; i < count; ++i) {
        void *function = host_resolve_import(name);
        if (!function && !strncmp(name, "hostgl_", 7))
            function = host_gl_resolve(name + 7);
        if (!function) {
            host_logf(HOST_LOG_WARN, "missing import: %s", name);
            function = (void *)missing_import;
        }
        table[i] = (uint64_t)(uintptr_t)function;
        name += strlen(name) + 1;
    }
    table[count] = (uint64_t)(uintptr_t)host_native_stack_get;
    for (i = 0; i < elf->phnum; ++i)
        if (segments[i].type == PT_LOAD && (segments[i].flags & PF_X)) {
            uintptr_t start = segments[i].vaddr & ~(uintptr_t)0xfff;
            uintptr_t end = (segments[i].vaddr + segments[i].memsz + 0xfff) & ~(uintptr_t)0xfff;
            ULONG old;
            if (!VirtualProtectFromApp((void *)start, end - start, PAGE_EXECUTE_READ, &old))
                return -1;
            FlushInstructionCache(GetCurrentProcess(), (void *)start, end - start);
        }
    host_logf(HOST_LOG_INFO, "loaded guest %08x-%08x with %u imports", (unsigned)low, (unsigned)high, count);
    return 0;
}
