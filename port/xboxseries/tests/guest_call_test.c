#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern unsigned char guest_probe_begin[], guest_probe_end[];
extern void abi_probe(uint32_t function, void *stack_top, uint32_t output[2]);

int main(void) {
    unsigned char *memory = NULL;
    for (uintptr_t address = 0x10000000; address < 0x70000000; address += 0x1000000) {
        memory = VirtualAlloc((void *)address, 0x20000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (memory) break;
    }
    if (!memory) { fprintf(stderr, "Cannot allocate low test memory\n"); return 2; }
    size_t size = (size_t)((uintptr_t)guest_probe_end - (uintptr_t)guest_probe_begin);
    memcpy(memory, guest_probe_begin, size);
    DWORD old;
    if (!VirtualProtect(memory, 0x1000, PAGE_EXECUTE_READ, &old) ||
        !FlushInstructionCache(GetCurrentProcess(), memory, size)) return 2;
    uint32_t output[2] = {0};
    for (int i = 0; i < 1000; ++i) {
        abi_probe((uint32_t)(uintptr_t)memory, memory + 0x20000, output);
        if (output[0] != 110 || output[1]) {
            fprintf(stderr, "FAIL: result=%u (expected 110), XMM6-XMM15 corruption mask=0x%x\n",
                    output[0], output[1]);
            VirtualFree(memory, 0, MEM_RELEASE);
            return 1;
        }
    }
    VirtualFree(memory, 0, MEM_RELEASE);
    puts("PASS: 1000 guest calls preserve XMM6-XMM15, arguments, return value, and guest stack alignment");
    return 0;
}
