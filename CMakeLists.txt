cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME stm32-dw3000-g474re-ioc-shell)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# -----------------------------------------------------------------------------
# ROLE handling (TX / RX)
# -----------------------------------------------------------------------------
if(NOT DEFINED ROLE)
    set(ROLE "TX")   # default
endif()

string(TOUPPER "${ROLE}" ROLE_UPPER)
message(STATUS "ROLE from build: ${ROLE_UPPER}")

set(UWB_ROLE_DEFINE "")
if("${ROLE_UPPER}" STREQUAL "TX")
    set(UWB_ROLE_DEFINE UWB_ROLE_TX)
elseif("${ROLE_UPPER}" STREQUAL "RX")
    set(UWB_ROLE_DEFINE UWB_ROLE_RX)
else()
    message(FATAL_ERROR "Invalid ROLE: ${ROLE_UPPER} (use TX or RX)")
endif()

# -----------------------------------------------------------------------------
# Optional user config header
# build option: -DUWB_USE_USER_CFG_FILE=ON
# -----------------------------------------------------------------------------

option(UWB_USE_USER_CFG_FILE "Enable uwb_user_cfg.h override" OFF)
option(UWB_RXDIAG_LOG_ENABLE "Enable UWB RX diagnostic telemetry logs" OFF)
option(UWB_VERBOSE_TAG_DIAG_LOG_ENABLE "Enable verbose UWB TAG diagnostic logs" OFF)

if(UWB_USE_USER_CFG_FILE)
    message(STATUS "UWB user config ENABLED")
    add_compile_definitions(UWB_USE_USER_CFG_FILE=1)
endif()

enable_language(C ASM)

add_executable(${CMAKE_PROJECT_NAME})

add_subdirectory(cmake/stm32cubemx)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Drivers/decadriver/deca_device.c
    ${CMAKE_SOURCE_DIR}/Drivers/decadriver/deca_port.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Drivers/decadriver
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    ${UWB_ROLE_DEFINE}
)

if(UWB_RXDIAG_LOG_ENABLE)
    message(STATUS "UWB_RXDIAG_LOG_ENABLE enabled")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE UWB_RXDIAG_LOG_ENABLE=1)
endif()

if(UWB_VERBOSE_TAG_DIAG_LOG_ENABLE)
    message(STATUS "UWB_VERBOSE_TAG_DIAG_LOG_ENABLE enabled")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE UWB_VERBOSE_TAG_DIAG_LOG_ENABLE=1)
endif()

if(DEFINED PRODUCT_LOW_POWER_MODE)
    message(STATUS "PRODUCT_LOW_POWER_MODE defined: ${PRODUCT_LOW_POWER_MODE}")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PRODUCT_LOW_POWER_MODE=${PRODUCT_LOW_POWER_MODE})
endif()

if(DEFINED PRODUCT_LOW_POWER_AUTO_ENTRY)
    message(STATUS "PRODUCT_LOW_POWER_AUTO_ENTRY defined: ${PRODUCT_LOW_POWER_AUTO_ENTRY}")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PRODUCT_LOW_POWER_AUTO_ENTRY=${PRODUCT_LOW_POWER_AUTO_ENTRY})
endif()

if(DEFINED PRODUCT_PC0_USER_ACTION)
    message(STATUS "PRODUCT_PC0_USER_ACTION defined: ${PRODUCT_PC0_USER_ACTION}")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PRODUCT_PC0_USER_ACTION=${PRODUCT_PC0_USER_ACTION})
endif()

if(DEFINED PRODUCT_UART_DIAG)
    message(STATUS "PRODUCT_UART_DIAG defined: ${PRODUCT_UART_DIAG}")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PRODUCT_UART_DIAG=${PRODUCT_UART_DIAG})
endif()

if(DEFINED PRODUCT_GPIO_DIAG)
    message(STATUS "PRODUCT_GPIO_DIAG defined: ${PRODUCT_GPIO_DIAG}")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PRODUCT_GPIO_DIAG=${PRODUCT_GPIO_DIAG})
endif()

if(DEFINED PRODUCT_DUAL_UART_DIAG)
    message(STATUS "PRODUCT_DUAL_UART_DIAG defined: ${PRODUCT_DUAL_UART_DIAG}")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PRODUCT_DUAL_UART_DIAG=${PRODUCT_DUAL_UART_DIAG})
endif()

if(DEFINED PRODUCT_MINIMAL_BOOT_LED_DIAG)
    message(STATUS "PRODUCT_MINIMAL_BOOT_LED_DIAG defined: ${PRODUCT_MINIMAL_BOOT_LED_DIAG}")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PRODUCT_MINIMAL_BOOT_LED_DIAG=${PRODUCT_MINIMAL_BOOT_LED_DIAG})
endif()

if(DEFINED PRODUCT_UART_SMOKE_KNOWN_GOOD)
    message(STATUS "PRODUCT_UART_SMOKE_KNOWN_GOOD defined: ${PRODUCT_UART_SMOKE_KNOWN_GOOD}")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PRODUCT_UART_SMOKE_KNOWN_GOOD=${PRODUCT_UART_SMOKE_KNOWN_GOOD})
endif()

if(DEFINED PRODUCT_EXPERIMENTAL_STOP1_SLOW_LISTEN)
    message(STATUS "PRODUCT_EXPERIMENTAL_STOP1_SLOW_LISTEN defined: ${PRODUCT_EXPERIMENTAL_STOP1_SLOW_LISTEN}")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PRODUCT_EXPERIMENTAL_STOP1_SLOW_LISTEN=${PRODUCT_EXPERIMENTAL_STOP1_SLOW_LISTEN})
endif()

list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    stm32cubemx
)

option(PRODUCT_COPY_DELIVERY_ARTIFACTS "Copy post-build firmware artifacts into delivery/" ON)

# Post-build: always generate local .bin/.hex; release builds copy them to delivery by default.
set(PRODUCT_POST_BUILD_ARTIFACT_COMMANDS
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin

    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
)

if(PRODUCT_COPY_DELIVERY_ARTIFACTS)
    message(STATUS "PRODUCT_COPY_DELIVERY_ARTIFACTS enabled")
    list(APPEND PRODUCT_POST_BUILD_ARTIFACT_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_SOURCE_DIR}/delivery

        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
                ${CMAKE_SOURCE_DIR}/delivery/firmware.bin

        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
                ${CMAKE_SOURCE_DIR}/delivery/firmware.hex
    )
else()
    message(STATUS "PRODUCT_COPY_DELIVERY_ARTIFACTS disabled; firmware artifacts stay in the build directory")
endif()

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    ${PRODUCT_POST_BUILD_ARTIFACT_COMMANDS}
    COMMENT "Generating firmware .bin and .hex artifacts"
)
