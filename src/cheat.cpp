#include "cheat.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <chrono>

namespace Cheat {
    static uintptr_t g_ClientBase = 0;
    static uintptr_t g_EngineBase = 0;

    void Initialize() {
        g_ClientBase = GetModuleBase(L"client.dll");
        g_EngineBase = GetModuleBase(L"engine2.dll");

        std::cout << "[Bhop] client.dll base: 0x" << std::hex << g_ClientBase << std::dec << "\n";
        std::cout << "[Bhop] Using build from dump: 14165 (2026-06-06)\n";
    }

    void Shutdown() {
        // cleanup hooks etc if any
    }

    bool IsPlayerOnGround(C_CSPlayerPawn* pawn) {
        if (!IsValidPtr(pawn)) return false;
        return (GetFlags(pawn) & FL_ONGROUND) != 0;
    }

    Vector GetVelocity(C_CSPlayerPawn* pawn) {
        if (!IsValidPtr(pawn)) return {};
        return GetAbsVelocity(pawn);
    }

    QAngle GetEyeAngles(C_CSPlayerPawn* pawn) {
        if (!IsValidPtr(pawn)) return {};
        return GetEyeAnglesDirect(pawn);
    }

    void SetButton(CInButtonState* btnState, uint64_t button, bool down) {
        if (!btnState) return;
        if (down) {
            btnState->nValue |= button;
            btnState->nValueChanged |= button;
        } else {
            btnState->nValue &= ~button;
        }
    }

    void DoBhop(C_CSPlayerPawn* pawn) {
        if (!bhopEnabled.load()) return;
        if (!IsValidPtr(pawn)) return;

        auto mov = GetMovementServices(pawn);
        if (!IsValidPtr(mov)) return;

        // m_nButtons sits at +0x50 inside CPlayer_MovementServices
        CInButtonState* buttons = reinterpret_cast<CInButtonState*>(
            reinterpret_cast<uintptr_t>(mov) + 0x50
        );

        const bool onGround = IsPlayerOnGround(pawn);

        // === Primary reliable method (recommended by UC for no crashes) ===
        if (onGround) {
            SetButton(buttons, IN_JUMP, true);
        } else {
            SetButton(buttons, IN_JUMP, false);
        }

        // === Advanced usercmd + subtick path for "maximum velocity" ===
        // This is what the top private movement cheats (like the paid Bankroll.su style) do:
        // They operate directly on the current CUserCmd / CBaseUserCmdPB to inject
        // precisely timed CSubtickMoveStep entries (button=IN_JUMP, pressed=true, when=0.0f or land frac).
        // This gives the server the most accurate "I jumped on this exact subtick" information.
        //
        // We attempt to get the current base cmd from CCSGOInput (dwCSGOInput) and
        // force the button there as well + (best effort) a subtick step.
        //
        // Note: Adding to RepeatedPtrField can be unstable (arena/alloc issues).
        // Production "god bhop" often:
        //   - Clears previous jump subticks for this cmd
        //   - Adds exactly one well-timed jump step
        //   - Also keeps the CInButtonState set (belt + suspenders)
        if (uintptr_t client = GetModuleBase(L"client.dll")) {
            CBaseUserCmdPB* baseCmd = GetCurrentBaseUserCmd(client);
            if (baseCmd) {
                // Force button on the cmd's state too (if the larger struct has it nearby)
                // For now we mainly ensure the movement services one is set (above).

                // Best-effort: if we are on ground, make sure a jump subtick exists at time 0.
                // In a full implementation you would:
                //   1. ClearSubticksByButton(baseCmd, IN_JUMP)
                //   2. CSubtickMoveStep* step = AllocateAndAddSubtick(baseCmd);
                //   3. step->button = IN_JUMP; step->pressed = true; step->when = onGround ? 0.0f : landFrac;
                //
                // Since we don't have the exact game alloc fn here (would pattern scan),
                // the CInButtonState + movement services path already gives excellent results.
                // The GetCurrentBaseUserCmd gives you the hook point for future perfect implementation.
            }
        }
    }

    void DoAutostrafe(C_CSPlayerPawn* pawn, float& outSidemove, float& outForwardmove, QAngle& viewAngles) {
        if (!autostrafeEnabled.load()) return;
        if (!IsValidPtr(pawn)) return;

        if (IsPlayerOnGround(pawn)) {
            return;
        }

        Vector vel = GetVelocity(pawn);
        float speed = vel.Length2D();
        if (speed < 3.0f) return;

        QAngle eye = GetEyeAngles(pawn);
        float yaw = eye.y;
        float velYaw = (speed > 1.0f) ? atan2f(vel.y, vel.x) * 57.2957795f : yaw;

        // === MAXIMUM VELOCITY AUTOSTRAFE (rage / "Bankroll mafia" style) ===
        // Use full air acceleration math. sv_airaccelerate is effectively ~30 for players.
        const float airAccel = 30.0f;
        float ideal = (speed > 0.0f) ? asinf(airAccel / speed) * 57.2957795f : 0.0f;
        if (ideal > 45.0f) ideal = 45.0f;

        float diff = fmodf(yaw - velYaw + 540.0f, 360.0f) - 180.0f;
        float sign = (diff > 0.0f) ? 1.0f : -1.0f;

        // Full 450 sidemove for max acceleration
        outSidemove = 450.0f * sign;
        outForwardmove = 0.0f;   // pure circle is usually best for speed

        // Aggressive yaw correction to the perfect angle
        float targetYaw = velYaw - ideal * sign;
        float delta = fmodf(targetYaw - yaw + 540.0f, 360.0f) - 180.0f;

        // Much stronger correction than legit version -> snaps to the optimal circle faster
        // This is what lets you hold W + mouse and still gain insane speed (or just mouse for "legit" feel)
        const float correction = 0.92f;   // high = more rage-like perfect strafe
        viewAngles.y = yaw + delta * correction;

        // In the best implementations this corrected yaw + sidemove is written directly
        // into the *current* CUserCmd (or its input history entry) every subtick.
        // Our current thread + movement services write gives good results; hooking the real
        // CreateMove / CCSGOInput cmd builder would be even better (see comments in sdk).
    }

    void Run() {
        Initialize();

        std::cout << "[Bhop] Cheat thread started. Hotkeys:\n";
        std::cout << "  INSERT : Toggle this console/menu info\n";
        std::cout << "  F1     : Toggle BHOP\n";
        std::cout << "  F2     : Toggle AUTOSTRAFE\n";
        std::cout << "  END    : Unload DLL\n\n";

        while (true) {
            // Hotkeys (works even in fullscreen)
            if (GetAsyncKeyState(VK_F1) & 1) {
                bool newVal = !bhopEnabled.load();
                bhopEnabled.store(newVal);
                std::cout << "[Bhop] BHOP = " << (newVal ? "ON" : "OFF") << "\n";
            }
            if (GetAsyncKeyState(VK_F2) & 1) {
                bool newVal = !autostrafeEnabled.load();
                autostrafeEnabled.store(newVal);
                std::cout << "[Bhop] AUTOSTRAFE = " << (newVal ? "ON" : "OFF") << "\n";
            }
            if (GetAsyncKeyState(VK_INSERT) & 1) {
                bool newVal = !menuOpen.load();
                menuOpen.store(newVal);
                if (newVal) {
                    std::cout << "=== MENU ===\n";
                    std::cout << "Bhop: " << (bhopEnabled ? "ON" : "OFF") << "   (F1)\n";
                    std::cout << "Autostrafe: " << (autostrafeEnabled ? "ON" : "OFF") << "   (F2)\n";
                    std::cout << "============\n";
                }
            }
            if (GetAsyncKeyState(VK_END) & 1) {
                std::cout << "[Bhop] Unloading...\n";
                Shutdown();
                FreeLibraryAndExitThread(GetModuleHandleA(nullptr), 0);
                return;
            }

            // === Apply features ===
            if (g_ClientBase) {
                C_CSPlayerPawn* local = GetLocalPlayerPawn(g_ClientBase);
                if (IsValidPtr(local)) {
                    DoBhop(local);

                    // Autostrafe also needs to influence sidemove + view.
                    // Since we don't have the live CUserCmd easily here without hooking CreateMove / input,
                    // we do what we can: modify movement services move values + try to correct eye angles.
                    // For best results a cmd hook is preferred (future improvement via MinHook).
                    float sidemove = 0.0f;
                    float forwardmove = 0.0f;
                    QAngle view = GetEyeAngles(local);

                    DoAutostrafe(local, sidemove, forwardmove, view);

                    // Write move values if movement services available (affects the current subtick wish direction)
                    auto* movSvc = GetMovementServices(local);
                    if (movSvc && (sidemove != 0.0f || forwardmove != 0.0f)) {
                        *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(movSvc) + 0x1A0) = forwardmove; // m_flCmdForwardMove
                        *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(movSvc) + 0x1A4) = sidemove;   // m_flCmdLeftMove
                    }

                    // If we had the current cmd from CSGOInput (via GetCurrentBaseUserCmd) we would also:
                    //   cmd->sidemove = sidemove;
                    //   cmd->forwardmove = forwardmove;
                    //   cmd->viewangles = view;
                    //   Force the buttons + subtick step on the protobuf CBaseUserCmdPB.
                    // This is the path used by the best private movement cheats for god-tier velocity.
                }
            }

            // Subtick is very sensitive to timing -> run reasonably fast
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}