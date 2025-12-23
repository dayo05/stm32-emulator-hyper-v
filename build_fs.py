# build_fs.py

import struct
import os
import time

# --- Configuration ---
# ENSURE THESE PATHS ARE CORRECT FOR YOUR PROJECT
BOOTLOADER = "build/boot.bin"      # The raw 512-byte bootloader
KERNEL     = "build/kernel.bin"    # The raw compiled C kernel (NO bootloader attached)
OUTPUT_DISK = "build/os_with_fs.vhd"       # The final file we will boot

# Files to put inside the OS
# Format: (Virtual Filename, Real Path)
FILES = [
    ("app.bin", "app/build/user_app.bin")
]

# Disk Geometry (10MB)
CYLINDERS = 20
HEADS = 16
SECTORS = 63
DISK_SIZE = CYLINDERS * HEADS * SECTORS * 512
# This must match 'mov al, 63' in boot.asm
KERNEL_SECTORS = 63

def create_vhd_footer(size):
    footer = bytearray(512)

    # 1. Prepare Fields
    cookie      = b'conectix'
    features    = 0x00000002
    version     = 0x00010000
    data_offset = 0xFFFFFFFFFFFFFFFF
    timestamp   = int(time.time()) - 946684800
    creator_app = b'py  '
    creator_ver = 0x00010000
    creator_os  = b'Wi2k'
    orig_size   = size
    curr_size   = size
    geometry    = (CYLINDERS << 16) | (HEADS << 8) | SECTORS
    disk_type   = 2 # Fixed Disk
    checksum    = 0 # Placeholder
    uuid        = b'\x00' * 16

    # 2. Pack Data (Correct Format: 14 items)
    # Format: > 8s I I Q I 4s I 4s Q Q I I I 16s
    struct.pack_into('>8sIIQI4sI4sQQIII16s', footer, 0,
                     cookie,       # 8s
                     features,     # I
                     version,      # I
                     data_offset,  # Q
                     timestamp,    # I
                     creator_app,  # 4s
                     creator_ver,  # I
                     creator_os,   # 4s
                     orig_size,    # Q
                     curr_size,    # Q
                     geometry,     # I
                     disk_type,    # I
                     checksum,     # I
                     uuid          # 16s
                     )

    # 3. Calculate Checksum
    # The VHD checksum is the one's complement of the sum of all bytes in the footer
    current_sum = sum(footer)
    final_checksum = (~current_sum) & 0xFFFFFFFF

    # 4. Write Checksum back to offset 64
    struct.pack_into('>I', footer, 64, final_checksum)

    return footer

def build():
    print(f"Building {OUTPUT_DISK}...")

    # 1. Load Bootloader
    with open(BOOTLOADER, 'rb') as f:
        boot_data = f.read()
        if len(boot_data) != 512:
            print("ERROR: Bootloader must be exactly 512 bytes.")
            return

    # 2. Load Kernel
    with open(KERNEL, 'rb') as f:
        kernel_data = f.read()

    # 3. Construct the Image
    # Sector 1: Bootloader
    data = boot_data

    KERNEL_AREA_SIZE = KERNEL_SECTORS * 512
    if len(kernel_data) > KERNEL_AREA_SIZE:
        print(f"ERROR: Kernel too large! ({len(kernel_data)} > {KERNEL_AREA_SIZE})")
        return

    data += kernel_data
    data += b'\x00' * (KERNEL_AREA_SIZE - len(kernel_data))

    # Sector 64: Filesystem Start
    print(f"Kernel ends at offset {len(data)}. Adding Filesystem...")

    # FS Header: Magic 'FS' + File Count (2 bytes)
    fs_header = b'FS' + struct.pack('<H', len(FILES))
    fs_body = b''
    current_offset = 0

    for vname, real_path in FILES:
        try:
            with open(real_path, 'rb') as f:
                file_content = f.read()

            # Entry: Name(16) + Size(4) + Offset(4)
            entry_name = vname.encode('ascii')[:16].ljust(16, b'\x00')
            fs_header += entry_name
            fs_header += struct.pack('<I', len(file_content))
            fs_header += struct.pack('<I', current_offset)

            fs_body += file_content
            current_offset += len(file_content)
            print(f" + Added {vname}")
        except FileNotFoundError:
            print(f" ! Warning: {real_path} not found. Skipping.")

    data += fs_header
    data += fs_body

    # 4. Pad to Disk Size
    if len(data) > DISK_SIZE:
        print("ERROR: Image exceeds disk size.")
        return
    data += b'\x00' * (DISK_SIZE - len(data))

    # 5. Append VHD Footer
    data += create_vhd_footer(DISK_SIZE)

    with open(OUTPUT_DISK, 'wb') as f:
        f.write(data)

    print("Done.")

if __name__ == "__main__":
    build()