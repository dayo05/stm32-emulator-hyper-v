Write-Host "Start building user application"
md -Force build > $null

# Compile to object file
gcc -m32 -ffreestanding -c user_app.c -o build/user_app.o

# Link to raw binary, forcing entry at 0x20000
# 1. Link as a Windows PE file (i386pe), but force the base address to 0.
#    This makes the linker calculate 0x20000 as an absolute address.
ld -m i386pe -o build/user_app.tmp -Ttext 0x20000 --image-base 0 build/user_app.o

# 2. Extract ONLY the code section to a raw binary.
#    This throws away the Windows PE headers and creates your flat file.
objcopy -O binary -j .text -j .data -j .rdata build/user_app.tmp build/user_app.bin
