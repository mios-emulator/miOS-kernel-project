[bits 32]
[extern kernel_main]
[extern __bss_start]
[extern __bss_end]
global _start

_start:
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    cld
    rep stosb
    call kernel_main
.colgar:
    cli
    hlt
    jmp .colgar
