#include "host.h"
#include <windows.h>
#include <bcrypt.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define NR(n) (0x40000000LL + (n))
#define GUEST(type, value) ((type)(uintptr_t)(uint32_t)(value))

struct guest_timespec {
    int64_t seconds;
    int32_t nanoseconds;
    int32_t pad;
};
struct guest_iovec {
    uint32_t base, length;
};
struct guest_stat {
    uint64_t dev, ino, nlink;
    uint32_t mode, uid, gid, pad0;
    uint64_t rdev;
    int64_t size;
    int32_t blksize, pad1;
    int64_t blocks;
    struct guest_timespec atime, mtime, ctime;
    int64_t unused[3];
};

static long failure(void) { return errno ? -errno : -5; }
static int open_flags(int flags) {
    int result = flags & 3;
    if (flags & 0100)
        result |= _O_CREAT;
    if (flags & 0200)
        result |= _O_EXCL;
    if (flags & 01000)
        result |= _O_TRUNC;
    if (flags & 02000)
        result |= _O_APPEND;
    return result | _O_BINARY;
}
static void fill_stat(struct guest_stat *out, const struct _stat64 *in) {
    memset(out, 0, sizeof(*out));
    out->dev = in->st_dev;
    out->ino = in->st_ino;
    out->nlink = in->st_nlink;
    out->mode = in->st_mode;
    out->uid = in->st_uid;
    out->gid = in->st_gid;
    out->rdev = in->st_rdev;
    out->size = in->st_size;
    out->blksize = 4096;
    out->blocks = (in->st_size + 511) / 512;
    out->atime.seconds = in->st_atime;
    out->mtime.seconds = in->st_mtime;
    out->ctime.seconds = in->st_ctime;
}
static void now_time(struct guest_timespec *out, int monotonic) {
    if (monotonic) {
        LARGE_INTEGER q, f;
        QueryPerformanceCounter(&q);
        QueryPerformanceFrequency(&f);
        out->seconds = q.QuadPart / f.QuadPart;
        out->nanoseconds = (int32_t)((q.QuadPart % f.QuadPart) * 1000000000LL / f.QuadPart);
    } else {
        FILETIME ft;
        ULARGE_INTEGER value;
        GetSystemTimePreciseAsFileTime(&ft);
        value.LowPart = ft.dwLowDateTime;
        value.HighPart = ft.dwHighDateTime;
        value.QuadPart -= 116444736000000000ULL;
        out->seconds = value.QuadPart / 10000000ULL;
        out->nanoseconds = (int32_t)(value.QuadPart % 10000000ULL) * 100;
    }
    out->pad = 0;
}

static long sleep_timespec(const struct guest_timespec *request, int absolute, int monotonic) {
    int64_t seconds;
    int64_t nanoseconds;
    uint64_t milliseconds;

    if (!request || request->seconds < 0 || request->nanoseconds < 0 || request->nanoseconds >= 1000000000)
        return -22;
    seconds = request->seconds;
    nanoseconds = request->nanoseconds;
    if (absolute) {
        struct guest_timespec now;
        now_time(&now, monotonic);
        seconds -= now.seconds;
        nanoseconds -= now.nanoseconds;
        if (nanoseconds < 0) {
            nanoseconds += 1000000000;
            seconds--;
        }
        if (seconds < 0)
            return 0;
    }
    milliseconds = (uint64_t)seconds * 1000 + ((uint64_t)nanoseconds + 999999) / 1000000;
    if (milliseconds > 0xfffffffeULL)
        milliseconds = 0xfffffffeULL;
    Sleep((DWORD)milliseconds);
    return 0;
}

long long GUEST_ABI host_syscall(long long number, long long a, long long b, long long c, long long d, long long e,
                                 long long f) {
    (void)e;
    (void)f;
    switch (number) {
    case NR(0): {
        int r = _read((int)a, GUEST(void *, b), (unsigned)c);
        return r < 0 ? failure() : r;
    }
    case NR(1): {
        if (a == 1 || a == 2) {
            char text[1025];
            unsigned n = (unsigned)c > 1024 ? 1024 : (unsigned)c;
            memcpy(text, GUEST(void *, b), n);
            text[n] = 0;
            host_logf(a == 2 ? HOST_LOG_WARN : HOST_LOG_INFO, "%s", text);
            return c;
        }
        {
            int r = _write((int)a, GUEST(const void *, b), (unsigned)c);
            return r < 0 ? failure() : r;
        }
    }
    case NR(2): {
        const char *path = GUEST(const char *, a);
        int r = _open(path, open_flags((int)b), (int)c);
        if (r < 0)
            host_logf(HOST_LOG_WARN, "open failed errno=%d path=%s", errno, path);
        return r < 0 ? failure() : r;
    }
    case NR(3):
        return _close((int)a) ? failure() : 0;
    case NR(5): {
        struct _stat64 s;
        if (_fstat64((int)a, &s))
            return failure();
        fill_stat(GUEST(struct guest_stat *, b), &s);
        return 0;
    }
    case NR(8): {
        __int64 r = _lseeki64((int)a, b, (int)c);
        return r < 0 ? failure() : r;
    }
    case NR(9):
        return host_guest_mmap(a, b, (int)c, (int)d, (int)e, f);
    case NR(10):
        return host_guest_mprotect(a, b, (int)c);
    case NR(11):
        return host_guest_munmap(a, b);
    case NR(12):
    case NR(25):
        return -12;
    case NR(17): {
        __int64 old = _telli64((int)a);
        if (old < 0 || _lseeki64((int)a, d, SEEK_SET) < 0)
            return failure();
        int r = _read((int)a, GUEST(void *, b), (unsigned)c);
        _lseeki64((int)a, old, SEEK_SET);
        return r < 0 ? failure() : r;
    }
    case NR(18): {
        __int64 old = _telli64((int)a);
        if (old < 0 || _lseeki64((int)a, d, SEEK_SET) < 0)
            return failure();
        int r = _write((int)a, GUEST(void *, b), (unsigned)c);
        _lseeki64((int)a, old, SEEK_SET);
        return r < 0 ? failure() : r;
    }
    case NR(24):
        SwitchToThread();
        return 0;
    case NR(35):
        return sleep_timespec(GUEST(const struct guest_timespec *, a), 0, 1);
    case NR(39):
        return GetCurrentProcessId();
    case NR(60):
    case NR(231):
        host_exit((int)a);
    case NR(72):
        return 0;
    case NR(74):
    case NR(75):
        return _commit((int)a) ? failure() : 0;
    case NR(77):
        return _chsize_s((int)a, b) ? failure() : 0;
    case NR(79):
        return _getcwd(GUEST(char *, a), (int)b) ? a : failure();
    case NR(80):
        return _chdir(GUEST(const char *, a)) ? failure() : 0;
    case NR(82):
        return rename(GUEST(const char *, a), GUEST(const char *, b)) ? failure() : 0;
    case NR(87):
        return remove(GUEST(const char *, a)) ? failure() : 0;
    case NR(89): {
        const char *path = GUEST(const char *, a);
        if (strcmp(path, "/proc/self/exe"))
            return -22;
        DWORD n = GetModuleFileNameA(NULL, GUEST(char *, b), (DWORD)c);
        return n ? (long long)n : -5;
    }
    case NR(96): {
        struct guest_timespec *t = GUEST(struct guest_timespec *, a);
        now_time(t, 0);
        t->nanoseconds /= 1000;
        return 0;
    }
    case NR(99):
        return -38;
    case NR(102):
    case NR(104):
    case NR(107):
    case NR(108):
        return 0;
    case NR(110):
        return 0;
    case NR(158):
        return -38;
    case NR(186):
        return GetCurrentThreadId();
    case NR(202): {
        volatile LONG *address = GUEST(volatile LONG *, a);
        int operation = (int)b & 127;
        if (operation == 0) {
            LONG expected = (LONG)c;
            DWORD timeout = INFINITE;
            if (d) {
                const struct guest_timespec *t = GUEST(const struct guest_timespec *, d);
                timeout = (DWORD)(t->seconds * 1000 + (t->nanoseconds + 999999) / 1000000);
            }
            if (*address != expected)
                return -11;
            return WaitOnAddress((volatile VOID *)address, &expected, 4, timeout) ? 0 : -110;
        }
        if (operation == 1) {
            if (c == 1)
                WakeByAddressSingle((PVOID)address);
            else
                WakeByAddressAll((PVOID)address);
            return 0;
        }
        return -38;
    }
    case NR(218):
        return GetCurrentThreadId();
    case NR(228):
        now_time(GUEST(struct guest_timespec *, b), a == 1);
        return 0;
    case NR(230):
        return sleep_timespec(GUEST(const struct guest_timespec *, c), (int)b & 1, a == 1);
    case NR(257): {
        const char *path = GUEST(const char *, b);
        int r = _open(path, open_flags((int)c), (int)d);
        if (r < 0)
            host_logf(HOST_LOG_WARN, "openat failed errno=%d path=%s", errno, path);
        return r < 0 ? failure() : r;
    }
    case NR(258):
        return _mkdir(GUEST(const char *, b)) ? failure() : 0;
    case NR(262): {
        struct _stat64 s;
        if (_stat64(GUEST(const char *, b), &s))
            return failure();
        fill_stat(GUEST(struct guest_stat *, c), &s);
        return 0;
    }
    case NR(263):
        return remove(GUEST(const char *, b)) ? failure() : 0;
    case NR(264):
        return rename(GUEST(const char *, b), GUEST(const char *, d)) ? failure() : 0;
    case NR(269):
        return _access(GUEST(const char *, b), (int)c) ? failure() : 0;
    case NR(318):
        return BCryptGenRandom(NULL, GUEST(PUCHAR, a), (ULONG)b, BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0 ? b : -5;
    case NR(514):
        return -25;
    case NR(515): {
        const struct guest_iovec *iov = GUEST(const struct guest_iovec *, b);
        long long total = 0;
        int index;
        for (index = 0; index < (int)c; index++) {
            int got = _read((int)a, GUEST(void *, iov[index].base), iov[index].length);
            if (got < 0)
                return total ? total : failure();
            total += got;
            if ((unsigned)got < iov[index].length)
                break;
        }
        return total;
    }
    case NR(516): {
        const struct guest_iovec *iov = GUEST(const struct guest_iovec *, b);
        long long total = 0;
        int index;
        for (index = 0; index < (int)c; index++) {
            const void *base = GUEST(const void *, iov[index].base);
            unsigned length = iov[index].length;
            if (a == 1 || a == 2) {
                unsigned done = 0;
                while (done < length) {
                    char text[1025];
                    unsigned n = length - done > 1024 ? 1024 : length - done;
                    memcpy(text, (const char *)base + done, n);
                    text[n] = 0;
                    host_logf(a == 2 ? HOST_LOG_WARN : HOST_LOG_INFO, "%s", text);
                    done += n;
                }
            } else {
                int written = _write((int)a, base, length);
                if (written < 0)
                    return total ? total : failure();
                if ((unsigned)written < length)
                    return total + written;
            }
            total += length;
        }
        return total;
    }
    default:
        host_logf(HOST_LOG_WARN, "unsupported x32 syscall %lld", number - 0x40000000LL);
        return -38;
    }
}
