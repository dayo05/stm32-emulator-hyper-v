; boot.asm

[org 0x7c00]
    xor ax, ax           ; Set AX = 0
    mov ds, ax           ; Set Data Segment to 0
    mov es, ax           ; Set Extra Segment to 0 (Critical for int 0x13 buffer)
    mov ss, ax           ; Set Stack Segment to 0

    mov sp, 0x9000       ; Set Stack Pointer
    mov [BOOT_DRIVE], dl ; BIOS stores our boot drive in DL, save it

    mov bp, 0x9000       ; Set the stack
    mov sp, bp

    call load_kernel     ; 1. Read C kernel from disk
; DEBUG: Print 'P' (Protected Mode switch coming)
    mov ah, 0x0E
    mov al, 'P'
    int 0x10

    call switch_to_pm    ; 2. Switch to 32-bit mode

    jmp $

%include "gdt.asm"       ; (We will create this file below)

[bits 16]
load_kernel:
    mov ah, 0x00         ; Function: Reset Disk System
    mov dl, [BOOT_DRIVE]
    int 0x13             ; If this fails, the next read will likely fail too, but we proceed.
    jc disk_error        ; Jump if error (Carry Flag set)
    ; Target Address: 0x10000
    ; In Real Mode, we use Segment:Offset (ES:BX)
    ; 0x1000 * 16 + 0x0000 = 0x10000
    mov ax, 0x1000
    mov es, ax       ; Set Extra Segment to 0x1000
    xor bx, bx       ; Set Offset (BX) to 0

; --- THE FIX ---
    ; Read all 31 sectors in ONE go.
    ; This avoids the loop logic where CL/CX registers conflict.
    mov ah, 0x02         ; BIOS Read Sector
    mov al, 31           ; Size of your kernel (sectors)
    mov ch, 0x00         ; Cylinder 0
    mov dh, 0x00         ; Head 0
    mov cl, 0x02         ; Start at Sector 2 (Sector 1 is bootloader)
    mov dl, [BOOT_DRIVE] ; Drive ID
    int 0x13             ; Call BIOS
    jc disk_error

    mov ah, 0x0E
    mov al, 'K'
    int 0x10
    ret


disk_error:
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    ; Simple error handler (loops forever)
    jmp $

[bits 16]
switch_to_pm:
    cli                  ; 1. Disable interrupts
    lgdt [gdt_descriptor]; 2. Load the GDT descriptor
    mov eax, cr0
    or eax, 0x1          ; 3. Set 32-bit mode bit in CR0
    mov cr0, eax
    jmp CODE_SEG:init_pm ; 4. Far jump to clear pipeline

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000     ; Stack at 0x90000 is still safe
    mov esp, ebp

    call 0x10000         ; Jump to new Kernel location

    jmp $

BOOT_DRIVE db 0

; Padding
times 510-($-$$) db 0
dw 0xaa55