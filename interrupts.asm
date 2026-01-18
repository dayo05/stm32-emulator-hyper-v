; interrupts.asm
[bits 32]
section .text

[extern page_fault_handler]
[extern debug_handler]
[extern timer_handler]
[extern syscall_handler]
[extern keyboard_handler]
global isr1_wrapper
global isr14_wrapper
global isr_timer_wrapper
global isr_keyboard_wrapper
global isr80
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

isr_timer_wrapper:
    pushad              ; Save all general purpose registers
    cld                 ; Clear direction flag (C code expects this)

    call timer_handler  ; Call the C function

    ; Send EOI (End of Interrupt) to Master PIC
    ; If we don't do this, the PIC won't send any more interrupts!
    mov al, 0x20
    out 0x20, al

    popad               ; Restore registers
    iretd               ; Return from Interrupt

isr_keyboard_wrapper:
    pushad
    cld
    
    call keyboard_handler

    mov al, 0x20
    out 0x20, al
    popad
    iretd

isr80:
    cli             ; Disable interrupts
    pusha           ; Save all registers (EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX)
    push ds
    push es
    push fs
    push gs

    ; Load kernel data segment if necessary (depends on your GDT setup)
    mov ax, 0x10    ; Example kernel data segment selector
    mov ds, ax
    mov es, ax

    ; Pass the stack pointer to the C function so we can access pushed registers
    push esp
    call syscall_handler
    add esp, 4      ; Clean up stack argument

    pop gs
    pop fs
    pop es
    pop ds
    popa            ; Restore registers (EAX contains return value if you modify it)
    iret
