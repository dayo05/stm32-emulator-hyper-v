; kernel_entry.asm

[bits 32]
[extern kern_main] ; Define calling point. Must match function name in C file
mov edi, 0xB8000
    mov byte [edi], 'X'      ; Char
    mov byte [edi+1], 0x2F   ; Green background, White text
mov eax, kern_main
    call eax
jmp $         ; Hang if main returns