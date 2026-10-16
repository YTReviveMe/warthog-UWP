; Enter x32 System V guest code from a Windows x64 host. The fifth and sixth
; parameters are the fourth guest argument and the top of a low (<4 GB) stack.
PUBLIC host_enter_guest
.code
host_enter_guest PROC
    mov eax, DWORD PTR [rsp+40]
    mov r10, QWORD PTR [rsp+48]
    push rbx
    push rbp
    push rdi
    push rsi
    push r12
    push r13
    push r14
    push r15
    ; Windows x64 preserves XMM6-XMM15; the x32 SysV guest may clobber them.
    sub rsp, 160
    movdqu XMMWORD PTR [rsp+0], xmm6
    movdqu XMMWORD PTR [rsp+16], xmm7
    movdqu XMMWORD PTR [rsp+32], xmm8
    movdqu XMMWORD PTR [rsp+48], xmm9
    movdqu XMMWORD PTR [rsp+64], xmm10
    movdqu XMMWORD PTR [rsp+80], xmm11
    movdqu XMMWORD PTR [rsp+96], xmm12
    movdqu XMMWORD PTR [rsp+112], xmm13
    movdqu XMMWORD PTR [rsp+128], xmm14
    movdqu XMMWORD PTR [rsp+144], xmm15
    mov r11, rsp
    and r10, -16
    sub r10, 16
    mov QWORD PTR [r10+8], r11
    mov r11, rcx
    mov edi, edx
    mov esi, r8d
    mov edx, r9d
    mov ecx, eax
    mov rsp, r10
    call r11
    mov rsp, QWORD PTR [rsp+8]
    movdqu xmm6, XMMWORD PTR [rsp+0]
    movdqu xmm7, XMMWORD PTR [rsp+16]
    movdqu xmm8, XMMWORD PTR [rsp+32]
    movdqu xmm9, XMMWORD PTR [rsp+48]
    movdqu xmm10, XMMWORD PTR [rsp+64]
    movdqu xmm11, XMMWORD PTR [rsp+80]
    movdqu xmm12, XMMWORD PTR [rsp+96]
    movdqu xmm13, XMMWORD PTR [rsp+112]
    movdqu xmm14, XMMWORD PTR [rsp+128]
    movdqu xmm15, XMMWORD PTR [rsp+144]
    add rsp, 160
    pop r15
    pop r14
    pop r13
    pop r12
    pop rsi
    pop rdi
    pop rbp
    pop rbx
    ret
host_enter_guest ENDP
END
