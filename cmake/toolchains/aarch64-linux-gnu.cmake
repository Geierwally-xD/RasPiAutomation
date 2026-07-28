# CMake Toolchain File for Raspberry Pi 4 (64-Bit / aarch64) Cross-Compilation
# Toolchain: SysGCC GCC 10.3 for aarch64-none-linux-gnu

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TOOLCHAIN_PREFIX "aarch64-none-linux-gnu-" CACHE STRING "Cross compiler prefix" FORCE)

# ── 1. Toolchain path from environment variable ToolChain ───────────────────
# ToolChain should point to the ToolChains root folder, e.g.:
#   I:\MyGitRepos\JoKiAutomation\ToolChains
if(DEFINED ENV{ToolChain})
    set(TOOLCHAINS_ROOT "$ENV{ToolChain}")
    message(STATUS "Using ToolChain from environment: ${TOOLCHAINS_ROOT}")
else()
    # Fallback: Search recursively upward from the workspace root.
    # Expected layout: <workspace-or-parent>/ToolChains/SysGCC/GCC64RasPi4/gccArmCrossCompiler/bin/<prefix>gcc
    set(_search_dir "${CMAKE_SOURCE_DIR}")
    set(_found_toolchains "")
    foreach(_i RANGE 0 20)
        set(_candidate "${_search_dir}/ToolChains")
        if(EXISTS "${_candidate}/SysGCC/GCC64RasPi4/gccArmCrossCompiler/bin/${TOOLCHAIN_PREFIX}gcc.exe"
           OR EXISTS "${_candidate}/SysGCC/GCC64RasPi4/gccArmCrossCompiler/bin/${TOOLCHAIN_PREFIX}gcc")
            set(_found_toolchains "${_candidate}")
            break()
        endif()

        get_filename_component(_parent_dir "${_search_dir}/.." ABSOLUTE)
        if(_parent_dir STREQUAL _search_dir)
            break()
        endif()
        set(_search_dir "${_parent_dir}")
    endforeach()

    if(_found_toolchains)
        set(TOOLCHAINS_ROOT "${_found_toolchains}")
        message(STATUS "ToolChain env not set, found ToolChains at: ${TOOLCHAINS_ROOT}")
    else()
        # Last resort: relative path from workspace folder
        get_filename_component(TOOLCHAINS_ROOT "${CMAKE_SOURCE_DIR}/../ToolChains" ABSOLUTE)
        message(WARNING "ToolChain env not set and auto-search failed, using fallback: ${TOOLCHAINS_ROOT}")
    endif()
endif()

set(TOOLCHAIN_ROOT "${TOOLCHAINS_ROOT}/SysGCC/GCC64RasPi4/gccArmCrossCompiler" CACHE PATH "Path to aarch64 cross compiler" FORCE)
set(TOOLCHAIN_BIN_DIR "${TOOLCHAIN_ROOT}/bin" CACHE PATH "Directory containing cross compiler binaries (bin)" FORCE)

# Cross-compiling: avoid try-compile requiring runnable executables
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY CACHE STRING "Try-compile target type" FORCE)

# ── 2. Ninja auto-detection ────────────────────────────────────────────────
if(CMAKE_GENERATOR STREQUAL "Ninja" AND NOT DEFINED CMAKE_MAKE_PROGRAM)
    # Check ToolChains folder first, then PATH, then Visual Studio
    if(EXISTS "${TOOLCHAINS_ROOT}/Ninja/ninja.exe")
        set(CMAKE_MAKE_PROGRAM "${TOOLCHAINS_ROOT}/Ninja/ninja.exe" CACHE FILEPATH "Ninja executable" FORCE)
    else()
        find_program(_NINJA_EXE NAMES ninja ninja.exe)
        if(NOT _NINJA_EXE AND WIN32)
            file(GLOB _VS_NINJA
                "C:/Program Files/Microsoft Visual Studio/2022/*/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
                "C:/Program Files (x86)/Microsoft Visual Studio/2022/*/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
            )
            list(LENGTH _VS_NINJA _VS_NINJA_COUNT)
            if(_VS_NINJA_COUNT GREATER 0)
                list(GET _VS_NINJA 0 _NINJA_EXE)
            endif()
        endif()
        if(_NINJA_EXE)
            set(CMAKE_MAKE_PROGRAM "${_NINJA_EXE}" CACHE FILEPATH "Ninja executable" FORCE)
        endif()
    endif()
endif()

# ── 3. Cross compiler ──────────────────────────────────────────────────────
set(CMAKE_C_COMPILER "${TOOLCHAIN_BIN_DIR}/${TOOLCHAIN_PREFIX}gcc.exe" CACHE FILEPATH "" FORCE)

# ── 4. Sysroot ─────────────────────────────────────────────────────────────
# Use the toolchain's built-in libc sysroot for standard headers/libs.
# Reason: Pi-sysroot /usr/lib/aarch64-linux-gnu libs use RELR relocations
# that are not supported by this toolchain's ld.exe (GCC 10.3.1).
set(CMAKE_SYSROOT "${TOOLCHAIN_ROOT}/aarch64-none-linux-gnu/libc" CACHE PATH "Sysroot used for includes and linking" FORCE)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Sysroot for Raspberry Pi target libraries (pigpio, ft4222, etc.)
set(PI_SYSROOT_DIR "${TOOLCHAIN_ROOT}/sysroot-pi" CACHE PATH "Path to synced Raspberry Pi sysroot")

# ── 5. Linker flags ────────────────────────────────────────────────────────
# Allow unresolved references from shared libs (resolved on target)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-shlib-undefined" CACHE STRING "" FORCE)

link_directories(
    "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu"
    "${CMAKE_SYSROOT}/lib/aarch64-linux-gnu"
    "${PI_SYSROOT_DIR}/usr/local/lib"
)

# Additional system include directories
include_directories(SYSTEM
    "${TOOLCHAIN_ROOT}/aarch64-none-linux-gnu/libc/usr/local/include"
    "${TOOLCHAIN_ROOT}/aarch64-none-linux-gnu/libc/usr/include/aarch64-linux-gnu"
    "${TOOLCHAIN_ROOT}/sysroot-pi/usr/local/include"
    "${TOOLCHAIN_ROOT}/sysroot-pi/usr/include/aarch64-linux-gnu"
)

# Ensure cross-compilation is correctly identified
set(CMAKE_CROSSCOMPILING TRUE CACHE BOOL "" FORCE)

# Explicitly set the linker to the cross-compiler's linker
set(CMAKE_LINKER "${TOOLCHAIN_BIN_DIR}/${TOOLCHAIN_PREFIX}ld" CACHE FILEPATH "" FORCE)

# ── 6. Status output ───────────────────────────────────────────────────────
if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "Cross compiler not found. Set ToolChain environment variable or place ToolChains folder in a parent directory.")
endif()

message(STATUS "[toolchain] TOOLCHAINS_ROOT=${TOOLCHAINS_ROOT}")
message(STATUS "[toolchain] TOOLCHAIN_ROOT=${TOOLCHAIN_ROOT}")
message(STATUS "[toolchain] TOOLCHAIN_BIN_DIR=${TOOLCHAIN_BIN_DIR}")
message(STATUS "[toolchain] C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "[toolchain] CMAKE_SYSROOT=${CMAKE_SYSROOT}")
message(STATUS "[toolchain] PI_SYSROOT_DIR=${PI_SYSROOT_DIR}")
