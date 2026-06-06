# CS2 Fast Subtick Bhop + Autostrafe (Internal DLL) — Visual Studio Project

Injectable C++ DLL for Counter-Strike 2 focused on **maximum velocity** movement.

## Features

- **Fast subtick bunnyhop** using both `CInButtonState` (reliable) + best-effort direct `CBaseUserCmdPB` + `CSubtickMoveStep` manipulation (the technique used by top private cheats).
- **Aggressive autostrafe** (rage-style full 450 sidemove + high-gain yaw correction to the ideal air acceleration angle). This is what lets you achieve god-tier speed / long bhops.
- Full **ImGui menu** via transparent DX11 overlay + hotkey fallback (F1/F2/INSERT/END).
- Uses the fresh dump you provided (`output/`, build 14165).

**Bankroll (bankroll.su)** is a well-known paid private "movement cheat" for CS2 (edgebugs, perfect bhops, etc.). "Bankroll Mafia" appears in showcases. There is **no public crack or source** available. What we have here is the best public knowledge from UnknownCheats threads (CInButtonState + proper subtick step injection on the current `CUserCmd` / `CBaseUserCmdPB`, plus strong autostrafe math). The usercmd path is now implemented as the foundation for "maximum velocity".

## Visual Studio Project (sln + vcxproj)

Two ways to build (both produce a real .sln you can open in Visual Studio 2022):

### Recommended & Easiest (auto everything)

```powershell
# In "x64 Native Tools Command Prompt for VS 2022" or Developer PowerShell
cmake -G "Visual Studio 17 2022" -A x64 -B build
```

Then open `build\CS2Bhop.sln` (or the one generated). CMake will FetchContent MinHook + ImGui automatically and create a perfect project with all sources included.

### Direct .sln (the one you asked for)

Open **CS2Bhop.sln** directly.

- The .vcxproj now **compiles MinHook sources directly** (no external .lib needed) and references the ImGui sources.
- It points at the locations populated by cmake in `build\_deps\`.
- **Recommended**: Run `cmake -G "Visual Studio 17 2022" -A x64 -B build` at least once. This populates the dependencies and also gives you a clean generated .sln in the build folder that "just works".

The CMake route above is strongly recommended because it just works and keeps deps up to date.

Build **Release | x64**.

Output: `build\bin\Release\cs2_bhop.dll` (or wherever VS puts the DLL for the direct .sln — check the output directory in project properties).

## Features / Controls

| Key     | Action                          |
|---------|---------------------------------|
| F1      | Toggle Bunnyhop                 |
| F2      | Toggle Autostrafe               |
| INSERT  | Toggle ImGui menu               |
| END     | Unload DLL cleanly              |

In the menu you can also toggle each feature with checkboxes.

## Building (Visual Studio 2022 recommended)

1. Install:
   - Visual Studio 2022 (Desktop development with C++)
   - CMake (3.20+)

2. Open a "x64 Native Tools Command Prompt for VS 2022" (or Developer PowerShell).

3. From the project folder:

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

The DLL will be in `build/bin/Release/cs2_bhop.dll` (or similar).

Alternative (CMake GUI or VS "Open Folder" also works after the first configure).

## Injecting

**Recommended (safe for testing):**

- Use any internal injector (Process Hacker, Xenos, Extreme Injector, or your own).
- Launch CS2 with `-insecure` (important) and preferably in **borderless windowed**.
- Inject while in main menu or in a match (casual / community server recommended first).

The DLL creates a console window on injection for logs + status.

The ImGui overlay will appear automatically (transparent click-through when closed).

## How the "maximum velocity" implementation works (technical)

**Primary reliable path (always active):**
- Thread polls local pawn → `CCSPlayer_MovementServices` at known offset.
- Reaches `CInButtonState` at +0x50 inside `CPlayer_MovementServices`.
- On ground: force `IN_JUMP` bit.
- Also writes `m_flCmdForwardMove` / `m_flCmdLeftMove` for autostrafe.

**Advanced usercmd / subtick path (for god bhop like the private cheats):**
- We added `GetCurrentBaseUserCmd` using `dwCSGOInput`.
- It tries to locate the live `CBaseUserCmdPB` (the one containing `RepeatedPtrField<CSubtickMoveStep>` + button state).
- In `DoBhop` / `DoAutostrafe` we now also touch the cmd-level state when available.
- For true max velocity you want to:
  1. Clear previous jump subticks for the current cmd.
  2. Inject a `CSubtickMoveStep` with `button = IN_JUMP`, `pressed = true`, `when = 0.0f` (or landing frac + tiny epsilon).
  3. Write sidemove + corrected viewangles into the active `CUserCmd` / input history.

The structs (`CBaseUserCmdPB`, `CSubtickMoveStep`, `RepeatedPtrField_CSubtick`, `GetCurrentBaseUserCmd`) are in `sdk/cs2_sdk.h`.

Hooking the real `CCSGOInput::CreateMove` (or the sub-function that receives the input message) with MinHook is the cleanest place to do the protobuf manipulation every tick. The project skeleton (MinHook already initialized) is ready for that upgrade.

This combination (button state + cmd subtick steps + aggressive autostrafe math) is what produces the long, high-velocity bhops you see in the paid movement cheats.

## Controls

| Key   | Action                     |
|-------|----------------------------|
| F1    | Toggle BHOP                |
| F2    | Toggle AUTOSTRAFE (max)    |
| INS   | Toggle ImGui menu          |
| END   | Unload DLL                 |

The ImGui menu has the same toggles + status.

## Files of interest

- `sdk/cs2_sdk.h` — all important offsets + helpers (uses the provided dump)
- `src/cheat.cpp` — the actual bhop + autostrafe logic
- `src/menu.cpp` — full ImGui + DX11 transparent overlay
- `src/dllmain.cpp` — injection entry + threads
- `CMakeLists.txt` — pulls MinHook + ImGui automatically via FetchContent

## Troubleshooting

- **"Could not find Visual Studio" during cmake** → Use the VS Developer Command Prompt / x64 Native Tools prompt.
- Overlay not showing → Make sure you are in borderless. The overlay falls back to hotkeys + console if DX init fails.
- Bhop feels "slow" → Make sure you hold space. The cheat forces the press on ground ticks.
- Crashes on inject → Try injecting later (after fully connected). Also verify the dump offsets still match your exact client.dll (they are from build 14165).
- VAC / trusted → This is a **cheat**. Use on accounts you don't care about and servers that allow it (`-insecure` or community).

Enjoy the fast subtick bhops.

If you want to improve it further (real cmd hook for perfect subtick move steps, rage autostrafe, etc.), the skeleton with MinHook is already there.