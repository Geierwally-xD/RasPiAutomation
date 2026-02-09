cmake_minimum_required(VERSION 3.20)
project(RasPiAutomation C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Cross builds: never embed host paths (e.g. Windows drive paths) as RPATH into the target ELF.
if(CMAKE_CROSSCOMPILING)
    set(CMAKE_SKIP_RPATH TRUE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "")
endif()

# Toolchain is provided via `-DCMAKE_TOOLCHAIN_FILE=...` from the VS Code task.
# Keep this file free of compiler/sysroot selection.

# Toolchain root (folder that contains `bin/` and `usr/`).
# Prefer deriving it from the configured cross compiler so it matches whatever
# `-DTOOLCHAIN_BIN_DIR=...` points to.
set(TOOLCHAIN_ROOT "")
if(CMAKE_C_COMPILER)
    get_filename_component(_tc_bin_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    get_filename_component(TOOLCHAIN_ROOT "${_tc_bin_dir}" DIRECTORY)
endif()
if(NOT TOOLCHAIN_ROOT)
    # Fallback relative to workspace
    set(TOOLCHAIN_ROOT "${CMAKE_SOURCE_DIR}/../../ToolChains/SysGCC/GCC64RasPi4")
endif()

# Source Files - exclude RasPiAutomation subdirectory (PlatformIO project)
file(GLOB SOURCES 
    "${CMAKE_SOURCE_DIR}/main.c"
)
file(GLOB_RECURSE SOURCES_MODULES
    "System_Raspberry/*.c"
    "PositionControl/*.c"
    "AudioControl/*.c"
    "Command_Interpreter/*.c"
    "Timer/*.c"
    "IRcontrol/*.c"
    "TestProg/*.c"
)
list(APPEND SOURCES ${SOURCES_MODULES})

## Explicitly set library paths for linker so crt1.o, crti.o, -lm, -ldl, -lpthread can be found
## Must be placed BEFORE add_executable()!
if(CMAKE_CROSSCOMPILING AND CMAKE_SYSROOT)
    link_directories(
        "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu"
        "${CMAKE_SYSROOT}/lib/aarch64-linux-gnu"
    )
endif()

# Executable erstellen
add_executable(${PROJECT_NAME} ${SOURCES})

# When cross-linking, shared libs (e.g. WiringPi) may contain undefined symbols
# that are resolved at runtime on the target (Raspberry Pi).
if(CMAKE_CROSSCOMPILING)
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--allow-shlib-undefined")
endif()

# Compiler flags inherited from Eclipse
target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g3 -Wall -fmessage-length=0)
target_compile_definitions(${PROJECT_NAME} PRIVATE _GNU_SOURCE)

# Include paths from Eclipse project files
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/System_Raspberry"
    "${CMAKE_SOURCE_DIR}/PositionControl"
    "${CMAKE_SOURCE_DIR}/AudioControl"
    "${CMAKE_SOURCE_DIR}/Command_Interpreter"
    "${CMAKE_SOURCE_DIR}/Timer"
    "${CMAKE_SOURCE_DIR}/IRcontrol"
    "${CMAKE_SOURCE_DIR}/TestProg"
)

# Sysroot is provided via --sysroot, aber Multiarch-Header wie <bits/wordsize.h>
# liegen unter usr/include/aarch64-linux-gnu und werden nicht automatisch gefunden.
# Use Pi-Sysroot for include paths if available (for wiringPi headers etc.)
if(PI_SYSROOT_DIR)
    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
        "${CMAKE_SYSROOT}/usr/local/include"
        "${PI_SYSROOT_DIR}/usr/local/include"
        "${CMAKE_SYSROOT}/usr/include/aarch64-linux-gnu"
        "${PI_SYSROOT_DIR}/usr/include/aarch64-linux-gnu"
    )
else()
    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
        "${CMAKE_SYSROOT}/usr/local/include"
        "${CMAKE_SYSROOT}/usr/include/aarch64-linux-gnu"
    )
endif()

# PI_SYSROOT_DIR is set in the toolchain file if Pi-Sysroot is available
if(NOT PI_SYSROOT_DIR)
    set(PI_SYSROOT_DIR "${CMAKE_SOURCE_DIR}/sysroot-pi" CACHE PATH "Path to synced Raspberry Pi sysroot (for matching /usr/local libs like wiringPi)")
endif()

set(_WIRINGPI_SEARCH_PATHS
    "${CMAKE_SYSROOT}/usr/local/lib"
    "${CMAKE_SYSROOT}/usr/lib"
    "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu"
)

# Also search for wiringPi in Pi-Sysroot (if available)
if(PI_SYSROOT_DIR AND EXISTS "${PI_SYSROOT_DIR}/usr/local/lib")
    list(APPEND _WIRINGPI_SEARCH_PATHS "${PI_SYSROOT_DIR}/usr/local/lib")
endif()

unset(WIRINGPI_LIB CACHE)
unset(WIRINGPI_DEV_LIB CACHE)
find_library(WIRINGPI_LIB wiringPi
    PATHS ${_WIRINGPI_SEARCH_PATHS}
    NO_DEFAULT_PATH
)
find_library(WIRINGPI_DEV_LIB wiringPiDev
    PATHS ${_WIRINGPI_SEARCH_PATHS}
    NO_DEFAULT_PATH
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    m
    dl
    pthread
)

if(WIRINGPI_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${WIRINGPI_LIB})
endif()
if(WIRINGPI_DEV_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${WIRINGPI_DEV_LIB})
endif()

# Custom Pre-Build Commands (chmod for Raspberry Pi)
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND echo "Setting up Raspberry Pi permissions..."
    COMMENT "Pre-build setup for RasPiAutomation"
)

# Install target for remote deployment
install(TARGETS ${PROJECT_NAME} DESTINATION "/home/pi/JokiAutomation/")
