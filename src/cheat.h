#pragma once
#include <atomic>
#include <Windows.h>
#include "../sdk/cs2_sdk.h"

namespace Cheat {
    inline std::atomic<bool> bhopEnabled{ true };
    inline std::atomic<bool> autostrafeEnabled{ true };
    inline std::atomic<bool> menuOpen{ true }; // console style or overlay

    void Initialize();
    void Shutdown();

    // Main loop (runs in its own thread)
    void Run();

    // Core features
    void DoBhop(C_CSPlayerPawn* pawn);
    void DoAutostrafe(C_CSPlayerPawn* pawn, float& outSidemove, float& outForwardmove, QAngle& viewAngles);

    // Helpers
    bool IsPlayerOnGround(C_CSPlayerPawn* pawn);
    Vector GetVelocity(C_CSPlayerPawn* pawn);
    QAngle GetEyeAngles(C_CSPlayerPawn* pawn);
    void SetButton(CInButtonState* btnState, uint64_t button, bool down);
}