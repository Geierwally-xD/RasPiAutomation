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

# Build-time configuration from a network.cfg file if present; otherwise fall back to VS Code settings.
set(NETWORK_CFG_PATH "${CMAKE_SOURCE_DIR}/../network.cfg")
if(NOT EXISTS "${NETWORK_CFG_PATH}")
    set(NETWORK_CFG_PATH "${CMAKE_SOURCE_DIR}/network.cfg")
endif()

set(RASPI_CONFIG_PTZ_CAM_ENABLED 0)
set(RASPI_CONFIG_IP_ADDRESS "0.0.0.0")

if(EXISTS "${NETWORK_CFG_PATH}")
    file(READ "${NETWORK_CFG_PATH}" RASPI_NETWORK_CFG_CONTENT)

    string(REGEX MATCH "^[ 	]*PTZ_CAM[ 	]*=[ 	]*([^#\r\n]+)" _ptz_match "${RASPI_NETWORK_CFG_CONTENT}")
    if(_ptz_match)
        string(STRIP "${CMAKE_MATCH_1}" _ptz_value)
        string(TOLOWER "${_ptz_value}" _ptz_value_lower)
        if(_ptz_value_lower STREQUAL "true" OR _ptz_value_lower STREQUAL "1" OR _ptz_value_lower STREQUAL "yes" OR _ptz_value_lower STREQUAL "on")
            set(RASPI_CONFIG_PTZ_CAM_ENABLED 1)
        endif()
    endif()

    foreach(_cfg_key IN ITEMS IP_ADDRESS IP IPADDR)
        string(REGEX MATCH "^[ 	]*${_cfg_key}[ 	]*=[ 	]*([^#\r\n]+)" _ip_match "${RASPI_NETWORK_CFG_CONTENT}")
        if(_ip_match)
            string(STRIP "${CMAKE_MATCH_1}" RASPI_CONFIG_IP_ADDRESS)
            break()
        endif()
    endforeach()
endif()

if(RASPI_CONFIG_IP_ADDRESS STREQUAL "0.0.0.0")
    set(RASPI_CONFIG_IP_ADDRESS "$ENV{RASPI_IP_ADDRESS}")
endif()
if(RASPI_CONFIG_IP_ADDRESS STREQUAL "")
    set(RASPI_CONFIG_IP_ADDRESS "0.0.0.0")
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
file(WRITE "${CMAKE_BINARY_DIR}/generated/raspi_build_config.h" "#pragma once\n#ifndef RASPI_BUILD_CONFIG_H\n#define RASPI_BUILD_CONFIG_H\n\n#define RASPI_CONFIG_PTZ_CAM_ENABLED ${RASPI_CONFIG_PTZ_CAM_ENABLED}\n#define RASPI_CONFIG_IP_ADDRESS \"${RASPI_CONFIG_IP_ADDRESS}\"\n\n#endif\n")
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

# When cross-linking, keep the ELF free of Windows host paths.
if(CMAKE_CROSSCOMPILING)
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--allow-shlib-undefined")
endif()

# Compiler flags inherited from Eclipse
target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g3 -Wall -fmessage-length=0)
target_compile_definitions(${PROJECT_NAME} PRIVATE _GNU_SOURCE)

# Include paths from Eclipse project files
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_BINARY_DIR}/generated"
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
# Use Pi-Sysroot for include paths if available.
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
    set(PI_SYSROOT_DIR "${CMAKE_SOURCE_DIR}/sysroot-pi" CACHE PATH "Path to synced Raspberry Pi sysroot")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    m
    dl
    pthread
)

# Custom Pre-Build Commands (chmod for Raspberry Pi)
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND echo "Setting up Raspberry Pi permissions..."
    COMMENT "Pre-build setup for RasPiAutomation"
)

# Install target for remote deployment
install(TARGETS ${PROJECT_NAME} DESTINATION "/home/pi/JokiAutomation/")
