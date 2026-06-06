@echo off
setlocal

echo === CS2 Bhop DLL Build Script (generates real .sln) ===
echo.

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: cmake not found in PATH. Install CMake and add to PATH.
    pause
    exit /b 1
)

if not exist "build" mkdir build
cd build

echo Configuring with Visual Studio 2022 x64 (this will also fetch MinHook + ImGui)...
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo.
    echo Configure failed. Run this from "x64 Native Tools Command Prompt for VS 2022" (or VS Developer PowerShell).
    pause
    exit /b 1
)

echo.
echo Building Release|x64...
cmake --build . --config Release

echo.
echo =====================================================
echo Done!
echo Open the generated .sln in the build\ folder (or use the hand-written CS2Bhop.sln in the project root).
echo DLL will be under build\bin\Release\ or your configured output directory.
echo =====================================================
pause
endlocal