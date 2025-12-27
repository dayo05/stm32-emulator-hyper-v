Write-Host "Start building user application"
md -Force build > $null

# Compile to object file
gcc -m32 -ffreestanding -c user_app.c -o build/user_app.o
gcc -m32 -ffreestanding -c ../syscall/register_timer_handler.c -o build/timer_handler_syscall.o
gcc -m32 -ffreestanding -c ../syscall/print.c -o build/print_syscall.o


# Link to raw binary, forcing entry at 0x20000
# 1. Link as a Windows PE file (i386pe), but force the base address to 0.
#    This makes the linker calculate 0x20000 as an absolute address.
ld -m i386pe -o build/user_app.tmp -T app_link.ld --image-base 0 build/user_app.o build/timer_handler_syscall.o build/print_syscall.o

# 2. Extract ONLY the code section to a raw binary.
#    This throws away the Windows PE headers and creates your flat file.
objcopy -O binary -j .text -j .data -j .rdata build/user_app.tmp build/user_app.bin
