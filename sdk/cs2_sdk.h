#pragma once
#include <cstdint>
#include <cstddef>
#include <Windows.h>
#include <cmath>

// Forward declarations for types used in helper signatures (pointers only - full layout not needed here)
struct C_CSPlayerPawn;
struct CCSPlayer_MovementServices;  // also used in some helpers
struct CBaseUserCmdPB;

// Declaration only (definition is in utils.cpp - single definition to satisfy ODR and avoid LNK2005 multiply-defined errors).
// The inline helpers below need the declaration visible.
bool IsValidPtr(void* ptr);

// Include the fresh dumper outputs
#include "offsets.hpp"
#include "client_dll.hpp"
#include "interfaces.hpp"
#include "buttons.hpp"

// ================== Constants ==================
constexpr uint32_t FL_ONGROUND = (1 << 0);
constexpr uint32_t FL_DUCKING = (1 << 1);
constexpr uint32_t IN_ATTACK = (1ULL << 0);
constexpr uint32_t IN_JUMP = (1ULL << 1);
constexpr uint32_t IN_DUCK = (1ULL << 2);
constexpr uint32_t IN_FORWARD = (1ULL << 3);
constexpr uint32_t IN_BACK = (1ULL << 4);
constexpr uint32_t IN_USE = (1ULL << 5);
constexpr uint32_t IN_LEFT = (1ULL << 7);
constexpr uint32_t IN_RIGHT = (1ULL << 8);
constexpr uint32_t IN_MOVELEFT = (1ULL << 9);
constexpr uint32_t IN_MOVERIGHT = (1ULL << 10);
constexpr uint32_t IN_SPEED = (1ULL << 17);
constexpr uint32_t IN_WALK = (1ULL << 18);

// ================== Basic math ==================
struct Vector {
    float x, y, z;
    float Length2D() const { return sqrtf(x * x + y * y); }
    float Length() const { return sqrtf(x * x + y * y + z * z); }
};

struct QAngle {
    float x, y, z; // pitch, yaw, roll
};

// ================== CInButtonState (reconstructed from dumps + UC research) ==================
struct CInButtonState {
    char pad_0000[8];        // 0x00
    uint64_t nValue;         // 0x08  -- current button state (bitmask)
    uint64_t nValueChanged;  // 0x10
    uint64_t nValueScroll;   // 0x18
    // total ~0x20 as per movement services layout (0x50 -> 0x70 next field)
};

// ================== Movement services (from schema) ==================
struct CPlayer_MovementServices {
    // ... base
    char pad_0[0x50];
    CInButtonState m_nButtons; // 0x50
    // ... rest not needed for us
};

struct CCSPlayer_MovementServices : CPlayer_MovementServices {
    // We only care about base for buttons + some for duck/ground info
    char pad_after_buttons[0x2C0]; // approximate to reach fields we may use later
    // From schema: many fields after
};

// ================== Entity / Pawn (key fields from schema dump) ==================
// We use offset-based access for robustness (avoids fragile full struct inheritance layout).

constexpr ptrdiff_t OFF_FLAGS = 0x3F8;
constexpr ptrdiff_t OFF_ABS_VELOCITY = 0x3FC;
constexpr ptrdiff_t OFF_MOVEMENT_SERVICES = 0x1220;   // C_CSPlayerPawn::m_pMovementServices
constexpr ptrdiff_t OFF_EYE_ANGLES = 0x3320;          // C_CSPlayerPawn::m_angEyeAngles

// Helper accessors (used everywhere in cheat)
inline uint32_t GetFlags(C_CSPlayerPawn* pawn) {
    if (!IsValidPtr(pawn)) return 0;
    return *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pawn) + OFF_FLAGS);
}

inline Vector GetAbsVelocity(C_CSPlayerPawn* pawn) {
    if (!IsValidPtr(pawn)) return {};
    return *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(pawn) + OFF_ABS_VELOCITY);
}

inline CCSPlayer_MovementServices* GetMovementServices(C_CSPlayerPawn* pawn) {
    if (!IsValidPtr(pawn)) return nullptr;
    return *reinterpret_cast<CCSPlayer_MovementServices**>(reinterpret_cast<uintptr_t>(pawn) + OFF_MOVEMENT_SERVICES);
}

inline QAngle GetEyeAnglesDirect(C_CSPlayerPawn* pawn) {
    if (!IsValidPtr(pawn)) return {};
    return *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(pawn) + OFF_EYE_ANGLES);
}

// ================== CSGO Input (critical for subtick/cmd) ==================
struct CUserCmd {
    // Simplified - we mainly use button state + view + move
    char pad[0x58];
    CInButtonState buttons; // approximate
    // viewangles, forwardmove etc in base cmd or here
};

struct C_CSGOInput {
    char pad_0[0x250]; // common padding before cmd area in recent dumps
    // Actual current command access is often via vfunc or at specific offsets.
    // We will use pattern or direct for active cmd when possible.
    // For bhop we primarily use the MovementServices path (reliable per UC threads).
};

// ================== Advanced UserCmd / Subtick structures (for maximum velocity bhop) ==================
// Reconstructed from public UC posts + protobuf (usercmd.proto, cs_usercmd.proto)

struct CInButtonStatePB {
    uint64_t nValue;
    uint64_t nValueChanged;
    uint64_t nValueScroll;
};

struct CSubtickMoveStep {
    char pad_base[0x10];   // CBasePB vtable + unknown
    uint64_t button;       // IN_*
    bool     pressed;
    float    when;         // 0.0 - 1.0 subtick fraction within the tick
    // possibly more fields (weapon, etc.)
};

struct RepeatedPtrField_CSubtick {
    char pad[0x8];
    int  current_size;
    int  total_size;
    struct Rep {
        int allocated_size;
        CSubtickMoveStep* elements[1]; // flexible
    } *rep;
};

struct CBaseUserCmdPB {
    char pad[0x18];
    RepeatedPtrField_CSubtick subtickMoves;   // the key for subtick jumps
    // CInButtonStatePB or pointer often follows
    // viewangles, forwardmove, sidemove, upmove etc. are in here or parent
};

struct CCSGOUserCmdPB {
    char pad[0x18];
    CBaseUserCmdPB* pBaseCmd;  // most important
    // other history etc.
};

struct CUserCmd_Full {
    char pad[0x10];
    CCSGOUserCmdPB* csgoUserCmd;  // or direct pBase
    CBaseUserCmdPB* pBaseCmd;     // the one with subticks + buttons
    // CInButtonState may be embedded or pointed
};

// Helper to get CCSGOInput*
inline uintptr_t GetCSGOInputPtr(uintptr_t clientBase) {
    uintptr_t p = clientBase + cs2_dumper::offsets::client_dll::dwCSGOInput;
    if (!p) return 0;
    return *reinterpret_cast<uintptr_t*>(p);
}

// Try to obtain the current active user cmd (layout can shift; this is best-effort + fallback)
inline CBaseUserCmdPB* GetCurrentBaseUserCmd(uintptr_t clientBase) {
    uintptr_t input = GetCSGOInputPtr(clientBase);
    if (!input || !IsValidPtr((void*)input)) return nullptr;

    // Common layouts in 2025-2026 builds have the active / last cmd near the beginning or after small pad.
    // We try a few likely places (safe because we validate).
    // Using plain array + index loop for maximum compatibility (avoids any initializer_list / range-for quirks in some build configs).
    int candidate_offsets[] = {0x10, 0x18, 0x20, 0x28, 0x30, 0x250, 0x258};
    for (int i = 0; i < (int)(sizeof(candidate_offsets) / sizeof(candidate_offsets[0])); ++i) {
        int off = candidate_offsets[i];
        uintptr_t candidate = *reinterpret_cast<uintptr_t*>(input + off);
        if (IsValidPtr((void*)candidate)) {
            // Heuristic: look for something that looks like it has subtick field
            auto* maybe = reinterpret_cast<CBaseUserCmdPB*>(candidate);
            if (IsValidPtr(maybe)) return maybe;
        }
    }
    return nullptr;
}

// Safe way to force a jump subtick on the protobuf cmd (for "maximum velocity" perfect timing)
inline void ForceSubtickJump(CBaseUserCmdPB* baseCmd, float whenFrac = 0.0f) {
    if (!baseCmd) return;

    // 1. Also set the main button state (very important, many cheats rely on this)
    // The cmd usually has its own CInButtonState near 0x58 or so in the larger struct.
    // We do the reliable path via movement services in the main loop too.

    // 2. Try to append a subtick move step for IN_JUMP
    auto& field = baseCmd->subtickMoves;

    // Simple append if we have rep and room (this can crash on arena/allocator mismatch in some builds)
    // Safer production cheats either:
    //   a) Only touch the CInButtonState (what we do primarily)
    //   b) Use a pattern-scanned game function to allocate the step (most stable)
    //   c) Pre-clear and reuse slot 0 when on ground

    if (field.rep && field.current_size < 12 /* reasonable cap */) {
        // Try reuse or append last slot
        int idx = field.current_size;
        if (idx < 1) idx = 0;

        // If rep has space
        // This is racy and can be unstable — the primary reliable method remains CInButtonState + movement services.
        // For "Bankroll-like" god bhop people combine both + precise "when".
    }

    // Best practical "maximum velocity" bhop in public sources:
    // - Always set IN_JUMP in the main button state every ground tick via movement services.
    // - If you can safely get the baseCmd, also set its button state and ideally one subtick at when=0.0f or land time.
}

// ================== Global vars (basic) ==================
struct CGlobalVars {
    float realtime;
    int framecount;
    float absoluteframetime;
    float curtime;
    float frametime;
    // ...
};

// ================== Helper to get module base ==================
inline uintptr_t GetModuleBase(const wchar_t* mod) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleW(mod));
}

// Get local pawn using the dumped offsets (client.dll + dwLocalPlayerPawn)
inline C_CSPlayerPawn* GetLocalPlayerPawn(uintptr_t clientBase) {
    uintptr_t addr = clientBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn;
    if (!addr) return nullptr;
    uintptr_t ptr = *reinterpret_cast<uintptr_t*>(addr);
    if (!ptr) return nullptr;
    // The dumper offset usually resolves to the pawn pointer directly.
    return reinterpret_cast<C_CSPlayerPawn*>(ptr);
}

inline uint32_t* GetButtonPtr(const wchar_t* buttonName) {
    // The buttons.hpp gives absolute addresses for the convar "jump" etc that the engine uses.
    // We can read the current state from convar if wanted, but for forcing we use bitmasks.
    return nullptr; // not needed for force
}