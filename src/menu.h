#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <atomic>

namespace Menu {
    inline std::atomic<bool> showMenu{ true };

    bool InitializeOverlay();
    void ShutdownOverlay();
    void RenderLoop();   // runs its own thread
    void Toggle();
}