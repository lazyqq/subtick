#include "utils.h"
#include <Psapi.h>
#include <iostream>

uintptr_t PatternScan(uintptr_t base, size_t size, const char* pattern, const char* mask) {
    size_t patternLen = strlen(mask);
    for (size_t i = 0; i < size - patternLen; i++) {
        bool found = true;
        for (size_t j = 0; j < patternLen; j++) {
            if (mask[j] != '?' && pattern[j] != *(char*)(base + i + j)) {
                found = false;
                break;
            }
        }
        if (found)
            return base + i;
    }
    return 0;
}

uintptr_t GetInterface(uintptr_t moduleBase, const char* interfaceName) {
    // Standard Source2 interface reg: CreateInterface export
    using CreateInterfaceFn = void* (__cdecl*)(const char* name, int* returnCode);
    auto CreateInterface = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(reinterpret_cast<HMODULE>(moduleBase), "CreateInterface"));
    if (!CreateInterface) return 0;

    // Try exact + versioned variants common in CS2
    void* iface = CreateInterface(interfaceName, nullptr);
    if (iface) return reinterpret_cast<uintptr_t>(iface);

    // Some are like Source2Client002 etc, already in dumper, user can pass full.
    return 0;
}

bool IsValidPtr(void* ptr) {
    if (!ptr) return false;
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(ptr, &mbi, sizeof(mbi))) {
        DWORD p = mbi.Protect;
        return (mbi.State == MEM_COMMIT) &&
               (p & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READWRITE | PAGE_READONLY)) &&
               !(p & PAGE_GUARD);
    }
    return false;
}