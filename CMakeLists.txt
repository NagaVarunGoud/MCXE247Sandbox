cmake_minimum_required(VERSION 3.20)

# ----------------------------------------------------------------------
# Toolchain (must be before project())
# ----------------------------------------------------------------------
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX arm-none-eabi)

# SIL-4 strict local paths (no fallbacks)
set(LOCAL_PROJECT_ROOT "/home/hbl/Desktop/varun_hbl/Project/Interrupt/working_zed_v3" CACHE PATH "Pinned local project root")
set(TOOLCHAIN_PATH "/usr/local/arm-gnu-toolchain/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin" CACHE PATH "Pinned local arm-none-eabi toolchain bin directory")
set(BOARD_CMAKE_PROFILE "/home/hbl/Desktop/CbtcGitRepos_test/Scripts/Cmake/cmake_mcxe247_gcc.cmake" CACHE FILEPATH "Pinned approved board-level CMake profile")

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc)

set(CMAKE_OBJCOPY ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-objcopy)
set(CMAKE_OBJDUMP ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-objdump)
set(CMAKE_SIZE    ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----------------------------------------------------------------------
# Project
# ----------------------------------------------------------------------
get_filename_component(PROJ_NAME "${CMAKE_SOURCE_DIR}" NAME)
project(${PROJ_NAME} C ASM)

# ----------------------------------------------------------------------
# Output directories
# ----------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

# ----------------------------------------------------------------------
# Project / NXP paths
# ----------------------------------------------------------------------
set(ROOT_DIR "${CMAKE_SOURCE_DIR}")
set(APP_DIR "${LOCAL_PROJECT_ROOT}")

set(NXP_ROOT "${LOCAL_PROJECT_ROOT}/NXP" CACHE PATH "Pinned local NXP root containing includes/ and drivers/")
set(DEVICE_DIR "${NXP_ROOT}/includes/MCXE247" CACHE PATH "Pinned local MCXE247 device include directory")
set(DRIVERS_DIR "${NXP_ROOT}/drivers/MCXE247" CACHE PATH "Pinned local MCXE247 drivers directory")
set(CMSIS_DIR "${DEVICE_DIR}/CMSIS")

# Variables expected by approved board-level profile
set(HBL_DRIVERS_DIR "${APP_DIR}" CACHE PATH "Local project root containing inc/")
set(NXP_DRIVERS_DIR "${DRIVERS_DIR}" CACHE PATH "NXP driver directory")

# Startup / linker / memcpy must be local to this project
set(STARTUP_SOURCE "${DEVICE_DIR}/startup_MCXE247.S")
set(FSL_MEMCPY_SOURCE "${APP_DIR}/src/fsl_memcpy.S")

# Fail fast on any missing configured path (SIL-4 deterministic behavior)
set(REQUIRED_PATHS
    "${LOCAL_PROJECT_ROOT}"
    "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc"
    "${APP_DIR}/src/min_runtime.c"
    "${APP_DIR}/src/board.c"
    "${APP_DIR}/src/clock_config.c"
    "${APP_DIR}/src/hardware_init.c"
    "${APP_DIR}/src/gpio_input_interrupt.c"
    "${APP_DIR}/src/pin_mux.c"
    "${APP_DIR}/inc"
    "${NXP_ROOT}"
    "${DEVICE_DIR}"
    "${DRIVERS_DIR}"
    "${DRIVERS_DIR}/fsl_clock.c"
    "${DRIVERS_DIR}/fsl_common.c"
    "${DRIVERS_DIR}/fsl_gpio.c"
    "${STARTUP_SOURCE}"
    "${DEVICE_DIR}/MCXE247_flash.ld"
    "${FSL_MEMCPY_SOURCE}"
    "${BOARD_CMAKE_PROFILE}"
)

foreach(req IN LISTS REQUIRED_PATHS)
    if(NOT EXISTS "${req}")
        message(FATAL_ERROR "Required local path not found: ${req}")
    endif()
endforeach()

message(STATUS "Toolchain bin: ${TOOLCHAIN_PATH}")
message(STATUS "App dir: ${APP_DIR}")
message(STATUS "Device dir: ${DEVICE_DIR}")
message(STATUS "Drivers dir: ${DRIVERS_DIR}")
message(STATUS "Board profile: ${BOARD_CMAKE_PROFILE}")

# ----------------------------------------------------------------------
# Project sources (project-level ownership)
# ----------------------------------------------------------------------
set(SOURCES
    ${STARTUP_SOURCE}
    ${DEVICE_DIR}/system_MCXE247.c
    ${APP_DIR}/src/min_runtime.c
    ${APP_DIR}/src/board.c
    ${APP_DIR}/src/clock_config.c
    ${APP_DIR}/src/hardware_init.c
    ${APP_DIR}/src/gpio_input_interrupt.c
    ${APP_DIR}/src/pin_mux.c
    ${FSL_MEMCPY_SOURCE}
)

set(MCUX_DRIVERS
    ${DRIVERS_DIR}/fsl_clock.c
    ${DRIVERS_DIR}/fsl_common.c
    ${DRIVERS_DIR}/fsl_gpio.c
)

list(APPEND SOURCES ${MCUX_DRIVERS})

# ----------------------------------------------------------------------
# Executable
# ----------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# Project-local include path(s) only. Board profile adds device/vendor includes.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${APP_DIR}/inc
)

# ----------------------------------------------------------------------
# Vendor source warning relaxations (project-specific)
# ----------------------------------------------------------------------
foreach(vsrc IN LISTS MCUX_DRIVERS)
    set_source_files_properties(${vsrc} PROPERTIES COMPILE_OPTIONS "-w")
endforeach()

set(VENDOR_SOURCES
    ${APP_DIR}/src/clock_config.c
    ${APP_DIR}/src/pin_mux.c
    ${APP_DIR}/src/board.c
    ${APP_DIR}/src/hardware_init.c
    ${APP_DIR}/src/gpio_input_interrupt.c
)

foreach(vsrc IN LISTS VENDOR_SOURCES)
    set_source_files_properties(${vsrc} PROPERTIES COMPILE_OPTIONS "-w")
endforeach()

# gpio_input_interrupt.c is MCUX-generated application code; keep strict global
# policy, but force vendor header visibility for this translation unit.
set_source_files_properties(
    ${APP_DIR}/src/gpio_input_interrupt.c
    PROPERTIES COMPILE_OPTIONS "-w;-include;fsl_port.h"
)

# ----------------------------------------------------------------------
# Approved board-level hardening profile (single source of truth)
# ----------------------------------------------------------------------
include(${BOARD_CMAKE_PROFILE})

# ----------------------------------------------------------------------
# Post-build: binary conversion
# ----------------------------------------------------------------------
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -O binary
            $<TARGET_FILE:${PROJECT_NAME}>
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.bin
    COMMENT "Generating binary"
)
