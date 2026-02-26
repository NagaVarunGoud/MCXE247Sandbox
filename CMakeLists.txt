cmake_minimum_required(VERSION 3.20)

# -----------------------------------------------------------------------------
# Toolchain (STRICT PINNED — MUST remain before project())
# -----------------------------------------------------------------------------
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX arm-none-eabi)
set(TOOLCHAIN_PATH
    "/usr/local/arm-gnu-toolchain/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin"
)

if(NOT EXISTS "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc")
    message(FATAL_ERROR "Pinned GCC not found at ${TOOLCHAIN_PATH}")
endif()

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-objcopy)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# -----------------------------------------------------------------------------
# Project
# -----------------------------------------------------------------------------
get_filename_component(PROJ_NAME "${CMAKE_SOURCE_DIR}" NAME)
project(${PROJ_NAME} C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# Output directories (must match old behavior for flash.sh)
# -----------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------
set(ROOT_DIR "$ENV{HOME}/Desktop/CbtcGitRepos_test/Code")

set(NXP_ROOT "${ROOT_DIR}/Common/CbtcFX/NXP")
set(DEVICE_DIR "${NXP_ROOT}/includes/MCXE247")
set(NXP_DRIVERS_DIR "${NXP_ROOT}/drivers/MCXE247")
set(CMSIS_DIR "${DEVICE_DIR}/CMSIS")

# Required by board-level profile; point to project root that contains inc/.
set(HBL_DRIVERS_DIR "${CMAKE_SOURCE_DIR}")

set(BOARD_CMAKE_PROFILE "${ROOT_DIR}/../Scripts/Cmake/cmake_mcxe247_gcc.cmake")

set(REQUIRED_PATHS
    "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc"
    "${DEVICE_DIR}/startup_MCXE247.S"
    "${DEVICE_DIR}/system_MCXE247.c"
    "${NXP_DRIVERS_DIR}/fsl_clock.c"
    "${NXP_DRIVERS_DIR}/fsl_common.c"
    "${NXP_DRIVERS_DIR}/fsl_gpio.c"
    "${CMAKE_SOURCE_DIR}/src/min_runtime.c"
    "${CMAKE_SOURCE_DIR}/src/board.c"
    "${CMAKE_SOURCE_DIR}/src/clock_config.c"
    "${CMAKE_SOURCE_DIR}/src/hardware_init.c"
    "${CMAKE_SOURCE_DIR}/src/pin_mux.c"
    "${CMAKE_SOURCE_DIR}/src/gpio_input_interrupt.c"
    "${CMAKE_SOURCE_DIR}/inc"
    "${BOARD_CMAKE_PROFILE}"
)

foreach(req IN LISTS REQUIRED_PATHS)
    if(NOT EXISTS "${req}")
        message(FATAL_ERROR "Required path not found: ${req}")
    endif()
endforeach()

# -----------------------------------------------------------------------------
# Sources (project + required MCU bring-up)
# -----------------------------------------------------------------------------
set(SOURCES
    ${DEVICE_DIR}/startup_MCXE247.S
    ${DEVICE_DIR}/system_MCXE247.c

    src/min_runtime.c
    src/board.c
    src/clock_config.c
    src/hardware_init.c
    src/pin_mux.c
    src/gpio_input_interrupt.c
    src/fsl_memcpy.S

    ${NXP_DRIVERS_DIR}/fsl_clock.c
    ${NXP_DRIVERS_DIR}/fsl_common.c
    ${NXP_DRIVERS_DIR}/fsl_gpio.c
)

add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# -----------------------------------------------------------------------------
# Project-only include directories
# (Board-level handles SYSTEM vendor includes)
# -----------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    inc
)

# -----------------------------------------------------------------------------
# Vendor warning suppression (unchanged behavior)
# -----------------------------------------------------------------------------
set(VENDOR_SOURCES
    src/clock_config.c
    src/pin_mux.c
    src/board.c
    src/hardware_init.c
    src/gpio_input_interrupt.c
    ${NXP_DRIVERS_DIR}/fsl_clock.c
    ${NXP_DRIVERS_DIR}/fsl_common.c
    ${NXP_DRIVERS_DIR}/fsl_gpio.c
)

foreach(vsrc IN LISTS VENDOR_SOURCES)
    set_source_files_properties(${vsrc}
        PROPERTIES COMPILE_OPTIONS "-w"
    )
endforeach()

# -----------------------------------------------------------------------------
# Build-type specific project definitions ONLY
# -----------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG_MODE=1>
)

# -----------------------------------------------------------------------------
# Include fixed board-level CMake (STRICT ORDER — after target creation)
# -----------------------------------------------------------------------------
include(${BOARD_CMAKE_PROFILE})

# -----------------------------------------------------------------------------
# Post-build binary (flash.sh compatibility)
# -----------------------------------------------------------------------------
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
        -O binary
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.bin"
)
