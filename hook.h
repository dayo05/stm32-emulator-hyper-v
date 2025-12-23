#ifndef HOOK_H
#define HOOK_H
// hooks.h

// Hook Callback Function Pointer
// address: The virtual address accessed
// page_ptr: Pointer to the actual physical memory page (4KB buffer)
// is_write: 1 if operation was a write, 0 if read
// Return: 1 to handled, 0 to crash
typedef int (*HookCallback)(uint32 address, void* page_ptr, int is_write);

struct HookEntry {
    uint32 virtual_address; // The address to hook (Must be 4KB aligned)
    HookCallback callback;  // The function to call
    int active;             // Is this hook used?
};

// Global State
#define MAX_HOOKS 10
struct HookEntry hooks[MAX_HOOKS];
struct HookEntry* current_pending_write_hook = 0; // Context for the Debug Handler
#endif