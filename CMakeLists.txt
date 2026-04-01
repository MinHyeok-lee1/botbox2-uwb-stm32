cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME stm32-dw3000-g474re-ioc-shell)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME} C ASM)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

set(CMAKE_C_LINK_EXECUTABLE
  "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
)

set(CMAKE_ASM_LINK_EXECUTABLE
  "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
)

message("Build type: " ${CMAKE_BUILD_TYPE})

# -----------------------------------------------------------------------------
# ROLE handling (TX / RX)
# -----------------------------------------------------------------------------
if(NOT DEFINED ROLE)
    set(ROLE "TX")
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
# -----------------------------------------------------------------------------
option(UWB_USE_USER_CFG_FILE "Enable uwb_user_cfg.h override" OFF)

if(UWB_USE_USER_CFG_FILE)
    message(STATUS "UWB user config ENABLED")
    add_compile_definitions(UWB_USE_USER_CFG_FILE=1)
endif()

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

list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    stm32cubemx
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32G474XX_FLASH.ld)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
            ${CMAKE_SOURCE_DIR}/delivery/firmware.bin
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
            ${CMAKE_SOURCE_DIR}/delivery/firmware.hex
    COMMENT "Generating and copying .bin and .hex for delivery"
)