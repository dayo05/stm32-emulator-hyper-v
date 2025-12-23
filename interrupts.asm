; interrupts.asm
[bits 32]
section .text

[extern page_fault_handler]
[extern debug_handler]
global isr1_wrapper
global isr14_wrapper
global load_idt

isr1_wrapper:
    push 0          ; Dummy error code (Int 1 doesn't push one, but struct expects it)
    pusha           ; Pushes EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX

    push esp        ; Pass pointer to TrapFrame as argument
    call debug_handler
    add esp, 4      ; Clean up argument

    popa
    add esp, 4      ; Clean up dummy error code
    iret

isr14_wrapper:
    ; Int 14 pushes error code automatically.
    pusha

    push esp        ; Pass pointer to TrapFrame
    call page_fault_handler
    add esp, 4

    popa
    add esp, 4      ; Clean up error code
    iret

load_idt:
    mov edx, [esp + 4]
    mov cx, [esp + 8]
    sub esp, 6
    mov [esp], cx
    mov [esp+2], edx
    lidt [esp]
    add esp, 6
    ret