; kernel_entry.asm

[bits 32]
[extern kern_main] ; Define calling point. Must match function name in C file

mov eax, kern_main
call eax
jmp $         ; Hang if main returns