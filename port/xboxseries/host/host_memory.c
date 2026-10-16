#include "host.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define PAGE_SIZE_XBOX 0x1000ULL
#define LOW_LIMIT 0x100000000ULL
#define POOL_SIZE (128ULL * 1024 * 1024)
#define POOL_PAGES (POOL_SIZE / PAGE_SIZE_XBOX)
#define MAX_POOLS 20
#define CUSTOM_EDITION_CACHE_BASE 0x40440000ULL
#define CUSTOM_EDITION_CACHE_SIZE 0x01700000ULL

struct pool {
    uintptr_t base;
    uint32_t free_pages;
    unsigned char used[POOL_PAGES];
};
static struct pool *pools[MAX_POOLS];
static int pool_count;
static SRWLOCK memory_lock = SRWLOCK_INIT;
static uintptr_t window_base, window_end, image_base, image_end;
static uintptr_t custom_edition_base, custom_edition_end;

static size_t round_page(size_t value) { return (value + 0xfff) & ~(size_t)0xfff; }
static int in_range(uintptr_t a, size_t n, uintptr_t lo, uintptr_t hi) { return a >= lo && a + n >= a && a + n <= hi; }

static DWORD page_protection(int protection) {
    switch (protection & 7) {
    case 0:
        return PAGE_NOACCESS;
    case 1:
        return PAGE_READONLY;
    case 4:
        return PAGE_EXECUTE;
    case 5:
        return PAGE_EXECUTE_READ;
    default:
        return PAGE_READWRITE;
    }
}

static int reserve_fixed(uintptr_t address, size_t size) {
    return VirtualAllocFromApp((void *)address, size, MEM_RESERVE, PAGE_NOACCESS) == (void *)address ? 0 : -1;
}

int host_memory_initialize(uint32_t base, uint32_t size) {
    window_base = HALO_GUEST_WINDOW_BASE;
    window_end = window_base + HALO_GUEST_WINDOW_SIZE;
    image_base = base;
    image_end = image_base + round_page(size);
    if (VirtualAllocFromApp((void *)window_base, HALO_GUEST_WINDOW_SIZE, MEM_RESERVE | MEM_WRITE_WATCH,
                            PAGE_READWRITE) != (void *)window_base) {
        host_logf(HOST_LOG_ERROR, "cannot reserve Xbox RAM at %08x (error %lu)", (unsigned)window_base, GetLastError());
        return -1;
    }
    if (reserve_fixed(image_base, image_end - image_base)) {
        host_logf(HOST_LOG_ERROR, "cannot reserve guest image at %08x (error %lu)", (unsigned)image_base,
                  GetLastError());
        return -1;
    }
    return 0;
}

static struct pool *new_pool(void) {
    uintptr_t address;
    struct pool *pool;
    if (pool_count >= MAX_POOLS)
        return NULL;
    for (address = 0x10000000; address + POOL_SIZE <= 0x80000000; address += 0x01000000) {
        if (VirtualAllocFromApp((void *)address, POOL_SIZE, MEM_RESERVE, PAGE_NOACCESS) == (void *)address)
            break;
    }
    if (address + POOL_SIZE > 0x80000000)
        return NULL;
    pool = (struct pool *)calloc(1, sizeof(*pool));
    if (!pool) {
        VirtualFree((void *)address, 0, MEM_RELEASE);
        return NULL;
    }
    pool->base = address;
    pool->free_pages = (uint32_t)POOL_PAGES;
    pools[pool_count++] = pool;
    host_logf(HOST_LOG_INFO, "guest memory pool at %08x", (unsigned)address);
    return pool;
}

static void *take_pages(struct pool *pool, size_t pages) {
    size_t page, run = 0;
    if (pages > pool->free_pages)
        return NULL;
    for (page = 0; page < POOL_PAGES; ++page) {
        if (pool->used[page]) {
            run = 0;
            continue;
        }
        if (++run == pages) {
            size_t first = page + 1 - pages;
            memset(pool->used + first, 1, pages);
            pool->free_pages -= (uint32_t)pages;
            return (void *)(pool->base + first * PAGE_SIZE_XBOX);
        }
    }
    return NULL;
}

void *host_low_map(size_t size, int protection) {
    size_t pages = round_page(size) / PAGE_SIZE_XBOX;
    void *result = NULL;
    int i;
    if (!pages || pages > POOL_PAGES)
        return NULL;
    AcquireSRWLockExclusive(&memory_lock);
    for (i = 0; i < pool_count && !result; ++i)
        result = take_pages(pools[i], pages);
    if (!result) {
        struct pool *pool = new_pool();
        if (pool)
            result = take_pages(pool, pages);
    }
    ReleaseSRWLockExclusive(&memory_lock);
    if (result && !VirtualAllocFromApp(result, pages * PAGE_SIZE_XBOX, MEM_COMMIT, page_protection(protection))) {
        host_low_unmap(result, pages * PAGE_SIZE_XBOX);
        result = NULL;
    }
    return result;
}

static struct pool *find_pool(uintptr_t address, size_t size) {
    int i;
    for (i = 0; i < pool_count; ++i)
        if (in_range(address, size, pools[i]->base, pools[i]->base + POOL_SIZE))
            return pools[i];
    return NULL;
}

void host_low_unmap(void *pointer, size_t size) {
    uintptr_t address = (uintptr_t)pointer & ~(uintptr_t)0xfff;
    size_t length = round_page((uintptr_t)pointer + size - address);
    struct pool *pool;
    AcquireSRWLockExclusive(&memory_lock);
    pool = find_pool(address, length);
    if (pool) {
        size_t first = (address - pool->base) / PAGE_SIZE_XBOX, pages = length / PAGE_SIZE_XBOX, i;
        VirtualFree((void *)address, length, MEM_DECOMMIT);
        for (i = first; i < first + pages; ++i)
            if (pool->used[i]) {
                pool->used[i] = 0;
                ++pool->free_pages;
            }
    }
    ReleaseSRWLockExclusive(&memory_lock);
}

int host_low_owns(uintptr_t address, size_t size) {
    int result;
    if (in_range(address, size, window_base, window_end) || in_range(address, size, image_base, image_end) ||
        in_range(address, size, custom_edition_base, custom_edition_end))
        return 1;
    AcquireSRWLockShared(&memory_lock);
    result = find_pool(address, size) != NULL;
    ReleaseSRWLockShared(&memory_lock);
    return result;
}

long GUEST_ABI host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset) {
    void *result;
    size_t length = round_page((size_t)size);
    (void)fd;
    (void)offset;
    if (!length)
        return -22;
    if (flags & 0x100000) {
        if (address == window_base && length == window_end - window_base) {
            host_logf(HOST_LOG_INFO, "guest reserved contiguous window %08x-%08x", (unsigned)window_base,
                      (unsigned)window_end);
            return (long)address;
        }
        if (address == CUSTOM_EDITION_CACHE_BASE && length == CUSTOM_EDITION_CACHE_SIZE) {
            SetLastError(ERROR_SUCCESS);
            result = VirtualAllocFromApp((void *)(uintptr_t)address, length, MEM_RESERVE | MEM_COMMIT,
                                         page_protection(protection));
            if (result == (void *)(uintptr_t)address) {
                custom_edition_base = (uintptr_t)address;
                custom_edition_end = custom_edition_base + length;
                host_logf(HOST_LOG_INFO, "guest reserved Custom Edition cache %08x-%08x", (unsigned)custom_edition_base,
                          (unsigned)custom_edition_end);
                return (long)address;
            }
            host_logf(HOST_LOG_WARN, "Custom Edition cache reservation failed at %08x error=%lu", (unsigned)address,
                      GetLastError());
            return -12;
        }
        host_logf(HOST_LOG_WARN, "MAP_FIXED_NOREPLACE rejected address=%08x size=%08x flags=%08x", (unsigned)address,
                  (unsigned)length, (unsigned)flags);
        return -17;
    }
    if (flags & 0x10) {
        if (address + length > LOW_LIMIT || !host_low_owns((uintptr_t)address, length)) {
            host_logf(HOST_LOG_WARN, "fixed mmap outside guest reservation address=%08x size=%08x flags=%08x",
                      (unsigned)address, (unsigned)length, (unsigned)flags);
            return -12;
        }
        VirtualFree((void *)(uintptr_t)address, length, MEM_DECOMMIT);
        SetLastError(ERROR_SUCCESS);
        result = VirtualAllocFromApp((void *)(uintptr_t)address, length, MEM_COMMIT, page_protection(protection));
    } else {
        result = host_low_map(length, protection);
    }
    if (!result)
        host_logf(HOST_LOG_WARN, "mmap failed address=%08x size=%08x protection=%x flags=%08x error=%lu",
                  (unsigned)address, (unsigned)length, protection, (unsigned)flags, GetLastError());
    return result ? (long)(uintptr_t)result : -12;
}

long GUEST_ABI host_guest_munmap(uint64_t address, uint64_t size) {
    if (address + size > LOW_LIMIT || !host_low_owns((uintptr_t)address, (size_t)size))
        return -22;
    if (in_range((uintptr_t)address, (size_t)size, image_base, image_end))
        return -22;
    if (address == custom_edition_base && round_page((size_t)size) == custom_edition_end - custom_edition_base) {
        if (!VirtualFree((void *)(uintptr_t)address, 0, MEM_RELEASE))
            return -22;
        custom_edition_base = custom_edition_end = 0;
        return 0;
    }
    if (find_pool((uintptr_t)address, (size_t)size))
        host_low_unmap((void *)(uintptr_t)address, (size_t)size);
    else
        VirtualFree((void *)(uintptr_t)address, round_page((size_t)size), MEM_DECOMMIT);
    return 0;
}

long GUEST_ABI host_guest_mprotect(uint64_t address, uint64_t size, int protection) {
    ULONG old;
    if (address + size > LOW_LIMIT ||
        !VirtualProtectFromApp((void *)(uintptr_t)address, round_page((size_t)size), page_protection(protection), &old))
        return -22;
    return 0;
}

#define WATCH_PAGE_COUNT (HALO_GUEST_WINDOW_SIZE / PAGE_SIZE_XBOX)

static volatile LONG watch_generation[WATCH_PAGE_COUNT];
static volatile LONG watch_serial = 1;
static PVOID watch_written[WATCH_PAGE_COUNT];
static SRWLOCK watch_lock = SRWLOCK_INIT;
static ULONGLONG watch_last_poll;
static int watch_active;

static size_t watch_page_index(uintptr_t address) { return (address - window_base) / PAGE_SIZE_XBOX; }

static void watch_refresh(void) {
    ULONGLONG now;
    ULONG_PTR count = WATCH_PAGE_COUNT;
    DWORD granularity = 0;
    ULONG_PTR index;

    if (!watch_active)
        return;
    now = GetTickCount64();
    if (now == watch_last_poll || !TryAcquireSRWLockExclusive(&watch_lock))
        return;
    if (now != watch_last_poll) {
        if (GetWriteWatch(WRITE_WATCH_FLAG_RESET, (void *)window_base, HALO_GUEST_WINDOW_SIZE, watch_written, &count,
                          &granularity) == 0) {
            for (index = 0; index < count; ++index) {
                size_t page = watch_page_index((uintptr_t)watch_written[index]);
                if (page < WATCH_PAGE_COUNT)
                    watch_generation[page] = InterlockedIncrement(&watch_serial);
            }
        }
        watch_last_poll = now;
    }
    ReleaseSRWLockExclusive(&watch_lock);
}

void GUEST_ABI host_memory_watch_initialize(void) {
    watch_active = 1;
    watch_last_poll = 0;
    watch_refresh();
}

void GUEST_ABI host_memory_watch_begin_frame(void) { watch_refresh(); }

void GUEST_ABI host_memory_watch_protect(uint32_t address, uint32_t size) {
    (void)address;
    (void)size;
    watch_refresh();
}

uint32_t GUEST_ABI host_memory_watch_generation(uint32_t address, uint32_t size) {
    uintptr_t start = address;
    size_t first, last, page;
    LONG newest = 0;

    watch_refresh();
    if (!size || start < window_base || start >= window_end)
        return 0;
    first = watch_page_index(start);
    last = watch_page_index(start + size - 1);
    if (last >= WATCH_PAGE_COUNT)
        last = WATCH_PAGE_COUNT - 1;
    for (page = first; page <= last; ++page) {
        LONG generation = watch_generation[page];
        if (generation > newest)
            newest = generation;
    }
    return (uint32_t)newest;
}

uint32_t GUEST_ABI host_memory_watch_serial(void) {
    watch_refresh();
    return (uint32_t)watch_serial;
}

void GUEST_ABI host_memory_watch_prepare_write(uint32_t address, uint32_t size) {
    (void)address;
    (void)size;
}

void GUEST_ABI host_memory_watch_forget(uint32_t address, uint32_t size) {
    uintptr_t start = address;
    uintptr_t end;
    size_t first, last, page;

    if (!size)
        return;
    end = start + size;
    if (end < start || end <= window_base || start >= window_end)
        return;
    watch_refresh();
    if (start < window_base)
        start = window_base;
    if (end > window_end)
        end = window_end;
    first = watch_page_index(start);
    last = watch_page_index(end - 1);
    for (page = first; page <= last; ++page)
        watch_generation[page] = InterlockedIncrement(&watch_serial);
    ResetWriteWatch((void *)start, end - start);
}
