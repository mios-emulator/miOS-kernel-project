[org 0x7C00]
[bits 16]

KERNEL  equ 0x1000
SECTORS equ 20

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [drive], dl

    mov bx, KERNEL
    mov ah, 0x02
    mov al, SECTORS
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [drive]
    int 0x13
    jc fallo

    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:modo32

fallo:
    mov ax, 0x0E45
    int 0x10
    jmp $

[bits 32]
modo32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    jmp KERNEL

drive db 0

gdt:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_desc:
    dw $ - gdt - 1
    dd gdt

times 510 - ($ - $$) db 0
dw 0xAA55
