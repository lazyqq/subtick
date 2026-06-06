#include "menu.h"
#include "cheat.h"
#include "../sdk/cs2_sdk.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <d3d11.h>
#include <dxgi.h>
#include <tchar.h>
#include <thread>
#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static HWND g_hWnd = nullptr;
static HWND g_gameHwnd = nullptr;
static WNDCLASSEXW g_wc{};

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* pBackBuffer = nullptr;
            g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            if (pBackBuffer) {
                g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
                pBackBuffer->Release();
            }
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0; // disable alt menu
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
    return true;
}

static void CleanupDeviceD3D() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static void UpdateOverlayPosition() {
    if (!g_gameHwnd || !g_hWnd) return;

    RECT rect;
    if (GetClientRect(g_gameHwnd, &rect) && GetWindowRect(g_gameHwnd, &rect)) {
        // Match game client area
        SetWindowPos(g_hWnd, HWND_TOPMOST,
            rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

namespace Menu {
    bool InitializeOverlay() {
        g_gameHwnd = FindWindowW(L"SDL_app", nullptr); // CS2 uses SDL
        if (!g_gameHwnd) {
            g_gameHwnd = FindWindowW(nullptr, L"Counter-Strike 2");
        }
        if (!g_gameHwnd) {
            std::cout << "[Menu] Could not find CS2 window. Overlay will still run but may be mispositioned.\n";
            g_gameHwnd = GetForegroundWindow();
        }

        ImGui_ImplWin32_EnableDpiAwareness();

        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"CS2BhopOverlay", nullptr };
        ::RegisterClassExW(&wc);

        g_hWnd = ::CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
            wc.lpszClassName, L"CS2 Bhop",
            WS_POPUP,
            0, 0, 100, 100,
            nullptr, nullptr, wc.hInstance, nullptr);

        // Make the window click-through when menu is closed
        SetLayeredWindowAttributes(g_hWnd, 0, 255, LWA_ALPHA);

        if (!CreateDeviceD3D(g_hWnd)) {
            CleanupDeviceD3D();
            ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return false;
        }

        ::ShowWindow(g_hWnd, SW_SHOWDEFAULT);
        ::UpdateWindow(g_hWnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(g_hWnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        std::cout << "[Menu] ImGui + DX11 overlay initialized.\n";
        return true;
    }

    void ShutdownOverlay() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        if (g_hWnd) {
            DestroyWindow(g_hWnd);
            g_hWnd = nullptr;
        }
    }

    void RenderLoop() {
        if (!InitializeOverlay()) {
            std::cout << "[Menu] Overlay failed to start. Falling back to hotkeys only.\n";
            return;
        }

        bool done = false;
        while (!done) {
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done) break;

            // Keep overlay on top of game
            UpdateOverlayPosition();

            // Toggle click-through based on menu visibility
            LONG exStyle = GetWindowLong(g_hWnd, GWL_EXSTYLE);
            if (showMenu.load()) {
                SetWindowLong(g_hWnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
            } else {
                SetWindowLong(g_hWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
            }

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (showMenu.load()) {
                ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);
                ImGui::Begin("CS2 Fast Subtick Bhop + Autostrafe", nullptr, ImGuiWindowFlags_NoCollapse);

                ImGui::Text("Build: client.dll 14165 (dump 2026-06-06)");
                ImGui::Separator();

                bool bhop = Cheat::bhopEnabled.load();
                if (ImGui::Checkbox("Bunny Hop (Subtick)", &bhop)) {
                    Cheat::bhopEnabled.store(bhop);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(F1)");

                bool strafe = Cheat::autostrafeEnabled.load();
                if (ImGui::Checkbox("Autostrafe (Air)", &strafe)) {
                    Cheat::autostrafeEnabled.store(strafe);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(F2)");

                ImGui::Separator();
                ImGui::Text("Instructions:");
                ImGui::BulletText("Hold SPACE for bhop (the cheat forces perfect timing via subtick button state).");
                ImGui::BulletText("In air: move mouse left/right or hold A/D for assisted circling (autostrafe).");
                ImGui::BulletText("END key unloads the DLL completely.");
                ImGui::BulletText("INSERT toggles this menu.");

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "Status: %s | %s",
                    Cheat::bhopEnabled.load() ? "BHOP ON" : "BHOP OFF",
                    Cheat::autostrafeEnabled.load() ? "STRAFE ON" : "STRAFE OFF");

                if (ImGui::Button("Unload DLL")) {
                    Cheat::Shutdown();
                    FreeLibraryAndExitThread(GetModuleHandle(nullptr), 0);
                }

                ImGui::End();
            }

            // Small always-visible indicator when menu closed
            if (!showMenu.load()) {
                ImGui::SetNextWindowBgAlpha(0.35f);
                ImGui::Begin("##status", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
                ImGui::Text("Bhop+Strafe | F1/F2/INS/END");
                ImGui::End();
            }

            ImGui::Render();
            const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            g_pSwapChain->Present(1, 0); // vsync on for stability
        }

        ShutdownOverlay();
    }

    void Toggle() {
        bool v = !showMenu.load();
        showMenu.store(v);
    }
}