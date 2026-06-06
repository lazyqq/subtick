#pragma once
#include <MinHook.h>

namespace Hooks {
    bool Initialize();
    void Shutdown();

    // Example: we can add a hook on input or frame function for perfect cmd access
    // void* GetCSGOInput();
}