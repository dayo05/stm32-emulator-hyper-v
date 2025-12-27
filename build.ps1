md -Force -Path build > $null

# 1. Compile C
gcc -ffreestanding -m32 -fno-leading-underscore -c kernel.c -o build/kernel.o
gcc -ffreestanding -m32 -fno-leading-underscore -c print.c -o build/print.o
gcc -ffreestanding -m32 -fno-leading-underscore -c ata.c -o build/ata.o
gcc -ffreestanding -m32 -fno-leading-underscore -c syscalls.c -o build/syscalls.o

# 2. Assemble Entry
# Ensure kernel_entry.asm has: [extern kernel_main] and call kernel_main
# Ensure kernel.c has: void kernel_main()
nasm -f win32 kernel_entry.asm -o build/kernel_entry.o
nasm -f win32 interrupts.asm -o build/interrupts.o

# 3. Link with Script
# Note: We don't need -Ttext because the script handles it
ld -m i386pe -T link.ld --image-base 0 -o build/kernel.tmp build/kernel_entry.o build/interrupts.o build/kernel.o build/print.o build/ata.o build/syscalls.o
# 4. Extract Raw Binary
# REMOVED: -j .rdata (It is now inside .text)
objcopy -O binary -j .text -j .data build/kernel.tmp build/kernel.bin

# 5. Assemble Boot Sector
nasm -f bin boot.asm -o build/boot.bin

# ... Rest of your script ...
Push-Location "app"
& "./build_app.ps1"
Pop-Location

python build_fs.py