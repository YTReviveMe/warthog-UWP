EXTERN host_enter_guest:PROC
PUBLIC abi_probe
PUBLIC guest_probe_begin
PUBLIC guest_probe_end
.code
abi_probe PROC
    sub rsp, 248
    mov QWORD PTR [rsp+208], r8
    mov QWORD PTR [rsp+40], rdx
    movdqu XMMWORD PTR [rsp+48], xmm6
    pcmpeqd xmm6, xmm6
    movdqu XMMWORD PTR [rsp+64], xmm7
    pcmpeqd xmm7, xmm7
    movdqu XMMWORD PTR [rsp+80], xmm8
    pcmpeqd xmm8, xmm8
    movdqu XMMWORD PTR [rsp+96], xmm9
    pcmpeqd xmm9, xmm9
    movdqu XMMWORD PTR [rsp+112], xmm10
    pcmpeqd xmm10, xmm10
    movdqu XMMWORD PTR [rsp+128], xmm11
    pcmpeqd xmm11, xmm11
    movdqu XMMWORD PTR [rsp+144], xmm12
    pcmpeqd xmm12, xmm12
    movdqu XMMWORD PTR [rsp+160], xmm13
    pcmpeqd xmm13, xmm13
    movdqu XMMWORD PTR [rsp+176], xmm14
    pcmpeqd xmm14, xmm14
    movdqu XMMWORD PTR [rsp+192], xmm15
    pcmpeqd xmm15, xmm15
    mov edx, 11
    mov r8d, 22
    mov r9d, 33
    mov DWORD PTR [rsp+32], 44
    call host_enter_guest
    mov r10, QWORD PTR [rsp+208]
    mov DWORD PTR [r10], eax
    xor r11d, r11d
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm0, xmm6
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    je preserved_6
    or r11d, 1
preserved_6:
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm0, xmm7
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    je preserved_7
    or r11d, 2
preserved_7:
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm0, xmm8
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    je preserved_8
    or r11d, 4
preserved_8:
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm0, xmm9
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    je preserved_9
    or r11d, 8
preserved_9:
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm0, xmm10
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    je preserved_10
    or r11d, 16
preserved_10:
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm0, xmm11
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    je preserved_11
    or r11d, 32
preserved_11:
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm0, xmm12
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    je preserved_12
    or r11d, 64
preserved_12:
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm0, xmm13
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    je preserved_13
    or r11d, 128
preserved_13:
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm0, xmm14
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    je preserved_14
    or r11d, 256
preserved_14:
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm0, xmm15
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    je preserved_15
    or r11d, 512
preserved_15:
    mov DWORD PTR [r10+4], r11d
    movdqu xmm6, XMMWORD PTR [rsp+48]
    movdqu xmm7, XMMWORD PTR [rsp+64]
    movdqu xmm8, XMMWORD PTR [rsp+80]
    movdqu xmm9, XMMWORD PTR [rsp+96]
    movdqu xmm10, XMMWORD PTR [rsp+112]
    movdqu xmm11, XMMWORD PTR [rsp+128]
    movdqu xmm12, XMMWORD PTR [rsp+144]
    movdqu xmm13, XMMWORD PTR [rsp+160]
    movdqu xmm14, XMMWORD PTR [rsp+176]
    movdqu xmm15, XMMWORD PTR [rsp+192]
    add rsp, 248
    ret
abi_probe ENDP

; Copied into executable memory below 4 GB. No relocations.
guest_probe_begin LABEL BYTE
    mov rax, rsp
    and eax, 15
    cmp eax, 8
    jne bad_stack
    pxor xmm6, xmm6
    pxor xmm7, xmm7
    pxor xmm8, xmm8
    pxor xmm9, xmm9
    pxor xmm10, xmm10
    pxor xmm11, xmm11
    pxor xmm12, xmm12
    pxor xmm13, xmm13
    pxor xmm14, xmm14
    pxor xmm15, xmm15
    mov eax, edi
    add eax, esi
    add eax, edx
    add eax, ecx
    ret
bad_stack:
    mov eax, 0deadbeefh
    ret
guest_probe_end LABEL BYTE
END
