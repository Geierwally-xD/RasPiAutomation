@echo off
REM ============================================================================
REM Set ToolChain environment variable for current session
REM Usage: Call this before building or run "SetToolChain.bat permanent" to set permanently
REM
REM Auto-detection: Searches recursively upward from project directory for
REM a "ToolChains" folder containing SysGCC\GCC64RasPi4.
REM ============================================================================

setlocal enabledelayedexpansion

REM --- 1. Check if ToolChain is already set and valid ---
if defined ToolChain (
    if exist "!ToolChain!\SysGCC\GCC64RasPi4\gccArmCrossCompiler\bin\aarch64-none-linux-gnu-gcc.exe" (
        echo ToolChain already set and valid: !ToolChain!
        endlocal
        exit /b 0
    )
    echo [WARNING] ToolChain is set to "!ToolChain!" but path is invalid.
    echo           Searching for correct path...
    echo.
)

REM --- 2. Search recursively upward from project directory ---
set "SEARCH_DIR=%~dp0"
set "TOOLCHAIN_PATH="

:search_loop
REM Remove trailing backslash for comparison
if "!SEARCH_DIR:~-1!"=="\" set "SEARCH_DIR=!SEARCH_DIR:~0,-1!"

REM Check if ToolChains exists at this level
if exist "!SEARCH_DIR!\ToolChains\SysGCC\GCC64RasPi4\gccArmCrossCompiler\bin\aarch64-none-linux-gnu-gcc.exe" (
    set "TOOLCHAIN_PATH=!SEARCH_DIR!\ToolChains"
    goto :found
)

REM Go one level up
for %%P in ("!SEARCH_DIR!") do set "PARENT=%%~dpP"
if "!PARENT:~-1!"=="\" set "PARENT=!PARENT:~0,-1!"

REM Stop if we reached the drive root
if "!PARENT!"=="!SEARCH_DIR!" (
    echo [ERROR] Could not find ToolChains directory.
    echo         Searched recursively upward from: %~dp0
    echo         Expected structure: ToolChains\SysGCC\GCC64RasPi4\gccArmCrossCompiler\bin\aarch64-none-linux-gnu-gcc.exe
    endlocal
    exit /b 1
)

set "SEARCH_DIR=!PARENT!"
goto :search_loop

:found
echo Found ToolChains at: !TOOLCHAIN_PATH!

REM --- 3. Verify required tools ---
echo.
echo Verifying tools:
echo   [OK] GCC64 Cross Compiler: !TOOLCHAIN_PATH!\SysGCC\GCC64RasPi4\gccArmCrossCompiler
if exist "!TOOLCHAIN_PATH!\Ninja\ninja.exe" (
    echo   [OK] Ninja build tool:     !TOOLCHAIN_PATH!\Ninja\ninja.exe
) else (
    echo   [--] Ninja not found in ToolChains\Ninja (will use PATH or VS bundled)
)
echo.

REM --- 4. Set the variable (endlocal trick to pass value out) ---
set "ARG1=%1"
set "TC_RESULT=!TOOLCHAIN_PATH!"
endlocal & set "ToolChain=%TC_RESULT%" & set "_TC_ARG=%ARG1%"

if "%_TC_ARG%"=="permanent" (
    echo Setting ToolChain permanently for current user...
    setx ToolChain "%ToolChain%" >nul
    echo ToolChain has been set permanently to: %ToolChain%
    echo Please restart your terminal/VS Code for changes to take effect.
) else (
    echo Setting ToolChain for current session...
    echo ToolChain set to: %ToolChain%
    echo.
    echo To set permanently, run: SetToolChain.bat permanent
)
set "_TC_ARG="
exit /b 0
