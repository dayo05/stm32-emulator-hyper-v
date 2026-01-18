{
  description = "Custom OS Kernel & User App Build";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        
        cc = pkgs.pkgsi686Linux.gcc;
        binutils = pkgs.pkgsi686Linux.binutils;

        osPackage = pkgs.stdenv.mkDerivation {
          name = "stm32-emulator";
          src = ./.; 

          nativeBuildInputs = [
            pkgs.nasm
            pkgs.python3
            cc
            binutils
          ];

          hardeningDisable = [ "all" ];

          buildPhase = ''
            mkdir -p build
            mkdir -p app/build

            echo "=== 1. Compile Kernel (C) ==="
            gcc -ffreestanding -m32 -fno-pic -fno-leading-underscore -c kernel.c -o build/kernel.o
            gcc -ffreestanding -m32 -fno-pic -fno-leading-underscore -c print.c -o build/print.o
            gcc -ffreestanding -m32 -fno-pic -fno-leading-underscore -c ata.c -o build/ata.o
            gcc -ffreestanding -m32 -fno-pic -fno-leading-underscore -c syscalls.c -o build/syscalls.o
            gcc -ffreestanding -m32 -fno-pic -fno-leading-underscore -c acpi.c -o build/acpi.o

            echo "=== 2. Assemble Entry (ASM) ==="
            nasm -f elf32 kernel_entry.asm -o build/kernel_entry.o
            nasm -f elf32 interrupts.asm -o build/interrupts.o

            echo "=== 3. Link Kernel ==="
            ld -m elf_i386 -T link.ld --image-base 0 -o build/kernel.tmp \
              build/kernel_entry.o build/interrupts.o build/kernel.o \
              build/print.o build/ata.o build/syscalls.o build/acpi.o

            echo "=== 4. Extract Kernel Binary ==="
            objcopy -O binary build/kernel.tmp build/kernel.bin

            echo "=== 5. Assemble Boot Sector ==="
            nasm -f bin boot.asm -o build/boot.bin

            echo "=== 6. Build User App ==="
            # Compiling application C files (Note: Paths adjusted for root execution)
            gcc -m32 -ffreestanding -fno-pic -c app/user_app.c -o app/build/user_app.o
            gcc -m32 -ffreestanding -fno-pic -c syscall/register_timer_handler.c -o app/build/timer_handler_syscall.o
            gcc -m32 -ffreestanding -fno-pic -c syscall/print.c -o app/build/print_syscall.o

            # Link App (ELF format)
            # We use app/app_link.ld but force output to app/build/user_app.tmp
            ld -m elf_i386 -o app/build/user_app.tmp -T app/app_link.ld --image-base 0 \
               app/build/user_app.o app/build/timer_handler_syscall.o app/build/print_syscall.o

            # Extract App Binary
            # Safe default: dumps all allocatable sections (.text, .data, .rodata)
            objcopy -O binary app/build/user_app.tmp app/build/user_app.bin

            echo "=== 7. Build Filesystem ==="
            python3 build_fs.py
          '';

          installPhase = ''
            mkdir -p $out
            cp build/os_with_fs.vhd $out/
            
            # Optional: Copy debug symbols
            cp build/kernel.tmp $out/kernel.elf
          '';
        };
      in
      {
        packages.default = osPackage;

        apps.default = {
          type = "app";
          program = "${pkgs.writeShellScript "run-os" ''
            echo "Starting QEMU in text mode (curses)..."
            echo "Press 'Esc' then '2' to access the QEMU monitor if needed."
            
            ${pkgs.qemu}/bin/qemu-system-i386 \
              -drive file=${osPackage}/os_with_fs.vhd,format=vpc,index=0,media=disk,snapshot=on \
              -display curses
          ''}";
        };

        devShells.default = pkgs.mkShell {
          buildInputs = [ pkgs.nasm pkgs.python3 pkgs.qemu cc binutils ];
        };
      }
    );
}
