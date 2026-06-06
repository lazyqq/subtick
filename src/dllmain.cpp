#include <Windows.h>
#include <iostream>
#include <thread>

#include "cheat.h"
#include "hooks.h"
#include "menu.h"

DWORD WINAPI MainThread(HMODULE hModule) {
    // Console for logs + fallback "menu"
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    SetConsoleTitleA("CS2 Subtick Bhop - Debug");

    std::cout << "=== CS2 Fast Subtick Bhop + Autostrafe (Internal DLL) ===\n";
    std::cout << "Injected successfully.\n\n";

    // Init MinHook (even if we don't hook much yet - ready for cmd hooks)
    Hooks::Initialize();

    // Start the main cheat logic thread (polls + applies movement)
    std::thread cheatThread([]() {
        Cheat::Run();
    });
    cheatThread.detach();

    // Start ImGui overlay menu (nice toggle window)
    std::thread menuThread([]() {
        Menu::RenderLoop();
    });
    menuThread.detach();

    // Keep the DLL alive
    while (true) {
        if (GetAsyncKeyState(VK_END) & 1) {
            std::cout << "[Main] END pressed - unloading.\n";
            break;
        }
        Sleep(50);
    }

    Hooks::Shutdown();
    Cheat::Shutdown();
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE h = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
        if (h) CloseHandle(h);
    }
    return TRUE;
}