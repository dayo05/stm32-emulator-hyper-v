// app/user_app.c
// This is the code we will compile on Windows and inject into the VM

void start_app() {
    char* video_memory = (char*) 0xB8000;

    // Calculate position for 3rd line (160 bytes per line)
    // 0xB8000 + (2 * 160) = 0xB8140
    int offset = 320;

    const char* msg = "[User Mode] Hello! I was loaded from the filesystem!";

    int i = 0;
    while (msg[i] != 0) {
        video_memory[offset + (i * 2)] = msg[i];
        video_memory[offset + (i * 2) + 1] = 0x0A; // Light Green text
        i++;
    }

    volatile int* device = (int*)0x100000;
    int val = *device; // Triggers Page Fault -> Callback -> Injects 0xCAFEBABE

    // 4. Test WRITE
    *device = 0xFFFF;  // Triggers Page Fault -> Map -> Trap -> Write -> Debug -> Callback -> Print


    return; // Return control to the kernel
}