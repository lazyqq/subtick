#include "hooks.h"
#include <iostream>

namespace Hooks {
    bool Initialize() {
        MH_STATUS status = MH_Initialize();
        if (status != MH_OK) {
            std::cout << "[Hooks] MinHook init failed: " << MH_StatusToString(status) << "\n";
            return false;
        }
        std::cout << "[Hooks] MinHook initialized.\n";
        // Add CreateHook / EnableHook here for future (e.g. input cmd builder, Present for menu, etc.)
        return true;
    }

    void Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
}