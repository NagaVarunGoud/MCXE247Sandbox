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
set(TOOLCHAIN_SYSROOT "/usr/local/arm-gnu-toolchain/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/arm-none-eabi" CACHE PATH "Pinned local arm-none-eabi sysroot")

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc)

set(CMAKE_OBJCOPY ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-objcopy)
set(CMAKE_OBJDUMP ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-objdump)
set(CMAKE_SIZE    ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ----------------------------------------------------------------------
# Project
# ----------------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(PROJ_NAME "${CMAKE_SOURCE_DIR}" NAME)
project(${PROJ_NAME} C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ----------------------------------------------------------------------
# Output directories
# ----------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

# ----------------------------------------------------------------------
# MCU defines
# ----------------------------------------------------------------------
set(MCU_DEFS
    CPU_MCXE247VLQ
    NDEBUG=1
)

# ----------------------------------------------------------------------
# Build-type flags
# ----------------------------------------------------------------------
set(DEBUG_FLAGS   -Og -g3 -fno-omit-frame-pointer)
set(RELEASE_FLAGS -O2 -g3 -fno-omit-frame-pointer)

set(DEBUG_DEFS   DEBUG_MODE=1)
set(RELEASE_DEFS)

# ----------------------------------------------------------------------
# NXP paths (strict local)
# ----------------------------------------------------------------------
set(APP_DIR "${LOCAL_PROJECT_ROOT}")
set(NXP_ROOT "${LOCAL_PROJECT_ROOT}/NXP" CACHE PATH "Pinned local NXP root containing includes/ and drivers/")
set(DEVICE_DIR  "${NXP_ROOT}/includes/MCXE247" CACHE PATH "Pinned local MCXE247 device include directory")
set(DRIVERS_DIR "${NXP_ROOT}/drivers/MCXE247" CACHE PATH "Pinned local MCXE247 drivers directory")
set(CMSIS_DIR "${DEVICE_DIR}/CMSIS")

# Startup / linker / memcpy must be local to this project
set(STARTUP_SOURCE "${DEVICE_DIR}/startup_MCXE247.S")
set(LINKER_SCRIPT "${DEVICE_DIR}/MCXE247_flash.ld")
set(FSL_MEMCPY_SOURCE "${APP_DIR}/src/fsl_memcpy.S")

# Fail fast on any missing configured path (SIL-4 deterministic behavior)
set(REQUIRED_PATHS
    "${LOCAL_PROJECT_ROOT}"
    "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc"
    "${TOOLCHAIN_SYSROOT}"
    "${APP_DIR}/src/min_runtime.c"
    "${APP_DIR}/src/board.c"
    "${APP_DIR}/src/clock_config.c"
    "${APP_DIR}/src/hardware_init.c"
    "${APP_DIR}/src/gpio_input_interrupt.c"
    "${APP_DIR}/src/pin_mux.c"
    "${NXP_ROOT}"
    "${DEVICE_DIR}"
    "${DRIVERS_DIR}"
    "${DRIVERS_DIR}/fsl_clock.c"
    # "${DRIVERS_DIR}/fsl_lpuart.c"
    "${DRIVERS_DIR}/fsl_common.c"
    "${DRIVERS_DIR}/fsl_gpio.c"
    "${STARTUP_SOURCE}"
    "${LINKER_SCRIPT}"
    "${FSL_MEMCPY_SOURCE}"
)

foreach(req IN LISTS REQUIRED_PATHS)
    if(NOT EXISTS "${req}")
        message(FATAL_ERROR "Required local path not found: ${req}")
    endif()
endforeach()

message(STATUS "Toolchain bin: ${TOOLCHAIN_PATH}")
message(STATUS "Toolchain sysroot: ${TOOLCHAIN_SYSROOT}")
message(STATUS "App dir: ${APP_DIR}")
message(STATUS "Device dir: ${DEVICE_DIR}")
message(STATUS "Drivers dir: ${DRIVERS_DIR}")

# ----------------------------------------------------------------------
# Base sources
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

# ----------------------------------------------------------------------
# MCUXpresso driver sources
# ----------------------------------------------------------------------
set(MCUX_DRIVERS
    ${DRIVERS_DIR}/fsl_clock.c
    # ${DRIVERS_DIR}/fsl_lpuart.c
    ${DRIVERS_DIR}/fsl_common.c
    ${DRIVERS_DIR}/fsl_gpio.c
)

list(APPEND SOURCES ${MCUX_DRIVERS})

foreach(vsrc IN LISTS MCUX_DRIVERS)
    set_source_files_properties(
        ${vsrc}
        PROPERTIES COMPILE_OPTIONS "-w"
    )
endforeach()

# ----------------------------------------------------------------------
# BSP / MCUXpresso-generated sources
# ----------------------------------------------------------------------
set(VENDOR_SOURCES
    ${APP_DIR}/src/clock_config.c
    ${APP_DIR}/src/pin_mux.c
    ${APP_DIR}/src/board.c
    ${APP_DIR}/src/hardware_init.c
    ${APP_DIR}/src/gpio_input_interrupt.c
)

foreach(vsrc IN LISTS VENDOR_SOURCES)
    set_source_files_properties(
        ${vsrc}
        PROPERTIES COMPILE_OPTIONS "-w"
    )
endforeach()

# gpio_input_interrupt.c is MCUX-generated application code; keep strict global
# policy, but force vendor header visibility for this translation unit.
set_source_files_properties(
    ${APP_DIR}/src/gpio_input_interrupt.c
    PROPERTIES COMPILE_OPTIONS "-w;-include;fsl_port.h"
)

# ----------------------------------------------------------------------
# Executable
# ----------------------------------------------------------------------
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# ----------------------------------------------------------------------
# Compile definitions
# ----------------------------------------------------------------------
target_compile_definitions(
    ${PROJECT_NAME}.elf PRIVATE
    ${MCU_DEFS}
    $<$<CONFIG:Debug>:${DEBUG_DEFS}>
    $<$<CONFIG:Release>:${RELEASE_DEFS}>
)

# ----------------------------------------------------------------------
# Include directories
# ----------------------------------------------------------------------
target_include_directories(
    ${PROJECT_NAME}.elf PRIVATE
    ${APP_DIR}/inc
)

target_include_directories(
    ${PROJECT_NAME}.elf SYSTEM PRIVATE
    ${DEVICE_DIR}
    ${DEVICE_DIR}/periph0
    ${CMSIS_DIR}/Core/Include
    ${DRIVERS_DIR}
)

# ----------------------------------------------------------------------
# Compile options
# ----------------------------------------------------------------------
target_compile_options(
    ${PROJECT_NAME}.elf PRIVATE
    --sysroot=${TOOLCHAIN_SYSROOT}
    -std=c11
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -ffreestanding
    -fno-builtin
    -fno-strict-aliasing
    -fno-delete-null-pointer-checks
    -fwrapv
    -fdata-sections
    -ffunction-sections
    -Wall
    -Wextra
    -Werror
    -Wundef
    -Wmissing-prototypes
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wstrict-aliasing=2
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wcast-qual
    -Wpointer-arith
    -Wswitch
    -Wswitch-enum
    -Wmissing-declarations
    -Wstrict-prototypes
    -Wfloat-equal
    -Wcast-align
    -Wunreachable-code
    -Wimplicit-fallthrough
    $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
    $<$<CONFIG:Release>:${RELEASE_FLAGS}>
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}=.
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.
    -fstack-usage
    -fno-common
)

# ----------------------------------------------------------------------
# Linker
# ----------------------------------------------------------------------
target_link_options(
    ${PROJECT_NAME}.elf PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -nostdlib
    -nostartfiles
    -nodefaultlibs
    -Wl,-e,Reset_Handler
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.map
    -Wl,--no-undefined
    -Wl,--fatal-warnings
    -Wl,--check-sections
    -Wl,--warn-common
    -Wl,--build-id=none
)

# ----------------------------------------------------------------------
# Post-build
# ----------------------------------------------------------------------
add_custom_command(
    TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -O binary
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.elf
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.bin
    COMMENT "Generating binary"
)
