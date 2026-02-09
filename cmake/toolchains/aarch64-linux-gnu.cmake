# Toolchain file for cross-compiling RasPiAutomation to Linux/aarch64 (Raspberry Pi)
# Mirrors the Eclipse CDT "Cross GCC" setup.

set(CMAKE_SYSTEM_NAME Linux CACHE STRING "Target system" FORCE)
set(CMAKE_SYSTEM_PROCESSOR aarch64 CACHE STRING "Target arch" FORCE)

# Update toolchain path and prefix based on Eclipse configuration
set(TOOLCHAIN_PREFIX "aarch64-none-linux-gnu-" CACHE STRING "Cross compiler prefix" FORCE)

# Prefer the toolchain shipped next to this repo (relative to workspace) to
# avoid picking up a different SysGCC installation from PATH (e.g. D:\SysGCC).
# Expected layout:
#   <MyGitRepos>/ToolChains/SysGCC/GCC64RasPi4/gccArmCrossCompiler/bin/<prefix>gcc(.exe)
get_filename_component(_MYGITREPOS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(_REPO_TOOLCHAIN_BIN "${_MYGITREPOS_ROOT}/ToolChains/SysGCC/GCC64RasPi4/gccArmCrossCompiler/bin")
if(EXISTS "${_REPO_TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}gcc.exe" OR EXISTS "${_REPO_TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}gcc")
    set(TOOLCHAIN_BIN_DIR "${_REPO_TOOLCHAIN_BIN}" CACHE PATH "Directory containing cross compiler binaries (bin)" FORCE)
endif()
# If TOOLCHAIN_BIN_DIR is not provided (e.g. via -DTOOLCHAIN_BIN_DIR=...)
# try to derive it from TOOLCHAIN_PATH.
if(NOT DEFINED TOOLCHAIN_BIN_DIR AND DEFINED TOOLCHAIN_PATH)
    set(TOOLCHAIN_BIN_DIR "${TOOLCHAIN_PATH}/bin" CACHE PATH "Directory containing cross compiler binaries" FORCE)
endif()

# Allow relative paths on the command line; interpret them relative to this toolchain file.
if(TOOLCHAIN_BIN_DIR AND NOT IS_ABSOLUTE "${TOOLCHAIN_BIN_DIR}")
    get_filename_component(TOOLCHAIN_BIN_DIR "${TOOLCHAIN_BIN_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    set(TOOLCHAIN_BIN_DIR "${TOOLCHAIN_BIN_DIR}" CACHE PATH "Directory containing cross compiler binaries (bin)" FORCE)
endif()

# Allow TOOLCHAIN_BIN_DIR to be supplied via environment variable.
if(NOT TOOLCHAIN_BIN_DIR AND DEFINED ENV{TOOLCHAIN_BIN_DIR})
    set(TOOLCHAIN_BIN_DIR "$ENV{TOOLCHAIN_BIN_DIR}" CACHE PATH "Directory containing cross compiler binaries (bin)" FORCE)
endif()

# If the user added the toolchain to PATH, derive TOOLCHAIN_BIN_DIR from the found compiler.
if(NOT TOOLCHAIN_BIN_DIR)
    find_program(_AARCH64_GCC_FROM_PATH NAMES "${TOOLCHAIN_PREFIX}gcc.exe" "${TOOLCHAIN_PREFIX}gcc")
    if(_AARCH64_GCC_FROM_PATH)
        get_filename_component(TOOLCHAIN_BIN_DIR "${_AARCH64_GCC_FROM_PATH}" DIRECTORY)
        set(TOOLCHAIN_BIN_DIR "${TOOLCHAIN_BIN_DIR}" CACHE PATH "Directory containing cross compiler binaries (bin)" FORCE)
    endif()
endif()

# Auto-discover TOOLCHAIN_BIN_DIR if not set.
# Searches in:
# - workspace root (../.. from this file)
# - one level above workspace
# - two levels above workspace
set(TOOLCHAIN_SEARCH_ROOT "" CACHE PATH "Optional single root folder to search for the cross toolchain if TOOLCHAIN_BIN_DIR is empty")
if(NOT TOOLCHAIN_BIN_DIR)
    set(_search_roots "")
    if(TOOLCHAIN_SEARCH_ROOT)
        list(APPEND _search_roots "${TOOLCHAIN_SEARCH_ROOT}")
    else()
        list(APPEND _search_roots
            "${CMAKE_CURRENT_LIST_DIR}/../.."        # RasPiAutomation/
            "${CMAKE_CURRENT_LIST_DIR}/../../.."     # JoKiAutomation/
            "${CMAKE_CURRENT_LIST_DIR}/../../../.."  # MyGitRepos/
        )
    endif()

    foreach(_root IN LISTS _search_roots)
        get_filename_component(_search_root "${_root}" ABSOLUTE)

        # Prefer .exe on Windows hosts, but also allow non-.exe toolchains.
        file(GLOB_RECURSE _gcc_candidates
            LIST_DIRECTORIES false
            "${_search_root}/**/bin/${TOOLCHAIN_PREFIX}gcc.exe"
            "${_search_root}/**/bin/${TOOLCHAIN_PREFIX}gcc"
        )
        list(LENGTH _gcc_candidates _gcc_count)
        if(_gcc_count GREATER 0)
            list(GET _gcc_candidates 0 _gcc_path)
            get_filename_component(_gcc_dir "${_gcc_path}" DIRECTORY)
            set(TOOLCHAIN_BIN_DIR "${_gcc_dir}" CACHE PATH "Directory containing cross compiler binaries (bin)" FORCE)
            break()
        endif()
    endforeach()
endif()

# Cross-compiling: avoid try-compile requiring runnable executables
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY CACHE STRING "Try-compile target type" FORCE)

# If the Ninja generator is used, ensure CMake can find ninja.exe.
# Many Windows setups have Ninja bundled with Visual Studio's CMake tools but not on PATH.
if(CMAKE_GENERATOR STREQUAL "Ninja" AND NOT DEFINED CMAKE_MAKE_PROGRAM)
    find_program(_NINJA_EXE NAMES ninja ninja.exe)

    if(NOT _NINJA_EXE AND WIN32)
        file(GLOB _VS_NINJA
            "C:/Program Files/Microsoft Visual Studio/2022/*/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
            "C:/Program Files (x86)/Microsoft Visual Studio/2022/*/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
            "C:/Program Files/Microsoft Visual Studio/2019/*/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/*/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
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

# Derive SysGCC layout defaults from TOOLCHAIN_BIN_DIR (Eclipse uses .../bin and .../usr/local)
if(TOOLCHAIN_BIN_DIR)
    get_filename_component(_TOOLCHAIN_ROOT "${TOOLCHAIN_BIN_DIR}" DIRECTORY)

    # Default: use the toolchain's built-in libc sysroot for standard headers/libs.
    # Reason: Pi-sysroot /usr/lib/aarch64-linux-gnu libs use RELR relocations 
    # that are not supported by this toolchain's ld.exe (GCC 10.3.1).
    set(_LIBC_SYSROOT "${_TOOLCHAIN_ROOT}/aarch64-none-linux-gnu/libc")

    # Optional: prefer a sysroot synced from the target Raspberry Pi for custom libs only.
    # Use Pi-sysroot for headers and custom libs (e.g., wiringPi), but NOT for standard libs.
    set(_PI_SYSROOT_TOOLCHAINS "${TOOLCHAIN_BIN_DIR}/../sysroot-pi")
    get_filename_component(_PI_SYSROOT_TOOLCHAINS "${_PI_SYSROOT_TOOLCHAINS}" ABSOLUTE)
    
    # Always use toolchain's libc sysroot to avoid RELR incompatibility
    if(EXISTS "${_LIBC_SYSROOT}/usr/include" AND EXISTS "${_LIBC_SYSROOT}/usr/lib")
        set(CMAKE_SYSROOT "${_LIBC_SYSROOT}" CACHE PATH "Sysroot used for includes and linking" FORCE)
    else()
        set(CMAKE_SYSROOT "${_TOOLCHAIN_ROOT}" CACHE PATH "Sysroot used for includes and linking" FORCE)
    endif()
    
    # Store Pi-sysroot path for finding custom libraries like wiringPi
    if(EXISTS "${_PI_SYSROOT_TOOLCHAINS}/usr/include" AND EXISTS "${_PI_SYSROOT_TOOLCHAINS}/lib")
        set(PI_SYSROOT_DIR "${_PI_SYSROOT_TOOLCHAINS}" CACHE PATH "Pi sysroot for custom libs (wiringPi)" FORCE)
    endif()

    if(NOT DEFINED SYSGCC_USR_LOCAL)
        if(EXISTS "${_TOOLCHAIN_ROOT}/usr/local/include")
            set(SYSGCC_USR_LOCAL "${_TOOLCHAIN_ROOT}/usr/local" CACHE PATH "SysGCC /usr/local prefix (contains include/ and lib/)" FORCE)
        endif()
    endif()

    if(NOT DEFINED SYSGCC_ROOT)
        if(EXISTS "${_TOOLCHAIN_ROOT}/usr/aarch64-none-linux-gnu")
            set(SYSGCC_ROOT "${_TOOLCHAIN_ROOT}/usr/aarch64-none-linux-gnu" CACHE PATH "Sysroot root for find_* (e.g. .../usr/aarch64-linux-gnu)" FORCE)
        elseif(EXISTS "${_TOOLCHAIN_ROOT}/usr/aarch64-linux-gnu")
            set(SYSGCC_ROOT "${_TOOLCHAIN_ROOT}/usr/aarch64-linux-gnu" CACHE PATH "Sysroot root for find_* (e.g. .../usr/aarch64-linux-gnu)" FORCE)
        endif()
    endif()
endif()

if(TOOLCHAIN_BIN_DIR)
    set(_tc "${TOOLCHAIN_BIN_DIR}/${TOOLCHAIN_PREFIX}")

    if(EXISTS "${_tc}gcc.exe")
        set(CMAKE_C_COMPILER "${_tc}gcc.exe" CACHE FILEPATH "" FORCE)
    else()
        set(CMAKE_C_COMPILER "${_tc}gcc" CACHE FILEPATH "" FORCE)
    endif()

    if(EXISTS "${_tc}g++.exe")
        set(CMAKE_CXX_COMPILER "${_tc}g++.exe" CACHE FILEPATH "" FORCE)
    else()
        set(CMAKE_CXX_COMPILER "${_tc}g++" CACHE FILEPATH "" FORCE)
    endif()

    if(EXISTS "${_tc}ar.exe")
        set(CMAKE_AR "${_tc}ar.exe" CACHE FILEPATH "" FORCE)
    else()
        set(CMAKE_AR "${_tc}ar" CACHE FILEPATH "" FORCE)
    endif()

    if(EXISTS "${_tc}ranlib.exe")
        set(CMAKE_RANLIB "${_tc}ranlib.exe" CACHE FILEPATH "" FORCE)
    else()
        set(CMAKE_RANLIB "${_tc}ranlib" CACHE FILEPATH "" FORCE)
    endif()

    if(EXISTS "${_tc}strip.exe")
        set(CMAKE_STRIP "${_tc}strip.exe" CACHE FILEPATH "" FORCE)
    else()
        set(CMAKE_STRIP "${_tc}strip" CACHE FILEPATH "" FORCE)
    endif()
else()
    find_program(CMAKE_C_COMPILER NAMES ${TOOLCHAIN_PREFIX}gcc)
    find_program(CMAKE_CXX_COMPILER NAMES ${TOOLCHAIN_PREFIX}g++)
    find_program(CMAKE_AR NAMES ${TOOLCHAIN_PREFIX}ar)
    find_program(CMAKE_RANLIB NAMES ${TOOLCHAIN_PREFIX}ranlib)
    find_program(CMAKE_STRIP NAMES ${TOOLCHAIN_PREFIX}strip)
endif()

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "Cross compiler not found. Set TOOLCHAIN_BIN_DIR or put ${TOOLCHAIN_PREFIX}gcc on PATH.")
endif()

message(STATUS "[toolchain] TOOLCHAIN_PREFIX=${TOOLCHAIN_PREFIX}")
message(STATUS "[toolchain] TOOLCHAIN_BIN_DIR=${TOOLCHAIN_BIN_DIR}")
message(STATUS "[toolchain] C compiler: ${CMAKE_C_COMPILER}")
if(CMAKE_CXX_COMPILER)
    message(STATUS "[toolchain] C++ compiler: ${CMAKE_CXX_COMPILER}")
endif()
if(SYSGCC_USR_LOCAL)
    message(STATUS "[toolchain] SYSGCC_USR_LOCAL=${SYSGCC_USR_LOCAL}")
endif()
if(SYSGCC_ROOT)
    message(STATUS "[toolchain] SYSGCC_ROOT=${SYSGCC_ROOT}")
endif()

# Optional sysroot/find-root for headers/libs (SysGCC, etc.)
set(SYSGCC_ROOT "${SYSGCC_ROOT}" CACHE PATH "Sysroot root for find_* (e.g. .../usr/aarch64-linux-gnu)")
if(SYSGCC_ROOT)
    set(CMAKE_FIND_ROOT_PATH "${SYSGCC_ROOT}" CACHE PATH "" FORCE)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

# Add library search paths for linker to find crt1.o, crti.o, libm, libdl, libpthread
if(CMAKE_SYSROOT)
    set(_SYSROOT_LIB_FLAGS "-L${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu -L${CMAKE_SYSROOT}/lib/aarch64-linux-gnu")
    set(CMAKE_EXE_LINKER_FLAGS "${_SYSROOT_LIB_FLAGS}" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS "${_SYSROOT_LIB_FLAGS}" CACHE STRING "" FORCE)
    set(CMAKE_MODULE_LINKER_FLAGS "${_SYSROOT_LIB_FLAGS}" CACHE STRING "" FORCE)
else()
    # Explicitly override linker flags to prevent Windows-specific flags from being passed
    set(CMAKE_EXE_LINKER_FLAGS "" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS "" CACHE STRING "" FORCE)
    set(CMAKE_MODULE_LINKER_FLAGS "" CACHE STRING "" FORCE)
endif()

# Ensure cross-compilation is correctly identified
set(CMAKE_CROSSCOMPILING TRUE CACHE BOOL "" FORCE)

# Explicitly set the linker to the cross-compiler's linker
if(TOOLCHAIN_BIN_DIR)
    set(CMAKE_LINKER "${TOOLCHAIN_BIN_DIR}/${TOOLCHAIN_PREFIX}ld" CACHE FILEPATH "" FORCE)
endif()
