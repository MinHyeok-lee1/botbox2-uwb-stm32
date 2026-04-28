cmake_minimum_required(VERSION 3.22)

enable_language(C ASM)

# -----------------------------------------------------------------------------
# STM32CubeMX generated symbols (macros)
# -----------------------------------------------------------------------------
set(MX_Defines_Syms
    USE_HAL_DRIVER
    STM32G474xx
    $<$<CONFIG:Debug>:DEBUG>
)

# NUCLEO BSP를 실제로 쓰는 프로젝트면 ON, 아니면 OFF
option(USE_G4_NUCLEO_BSP "Include STM32G4xx Nucleo BSP" OFF)

if(USE_G4_NUCLEO_BSP)
    list(APPEND MX_Defines_Syms USE_NUCLEO_64)
endif()

# -----------------------------------------------------------------------------
# STM32CubeMX generated include paths
# -----------------------------------------------------------------------------
set(MX_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Device/ST/STM32G4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Include
)

if(USE_G4_NUCLEO_BSP)
    list(APPEND MX_Include_Dirs
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/BSP/STM32G4xx_Nucleo
    )
endif()

# -----------------------------------------------------------------------------
# STM32CubeMX generated application sources
# -----------------------------------------------------------------------------
set(MX_Application_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/fdcan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/fmac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32g4xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32g4xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup_stm32g474xx.s
)

# 프로젝트에 따라 있는 파일만 추가
foreach(extra_src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/uwb_filter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/uwb_quality.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/uwb_cfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/can_controller.c
)
    if(EXISTS ${extra_src})
        list(APPEND MX_Application_Src ${extra_src})
    endif()
endforeach()

# -----------------------------------------------------------------------------
# STM32 HAL Drivers
# -----------------------------------------------------------------------------
set(STM32_Drivers_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/system_stm32g4xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ramfunc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_fmac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_uart_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_spi_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim_ex.c
)

# 프로젝트에 따라 있는 HAL 파일만 추가
foreach(extra_hal
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_fdcan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_usart_ex.c
)
    if(EXISTS ${extra_hal})
        list(APPEND STM32_Drivers_Src ${extra_hal})
    endif()
endforeach()

if(USE_G4_NUCLEO_BSP)
    list(APPEND STM32_Drivers_Src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/BSP/STM32G4xx_Nucleo/stm32g4xx_nucleo.c
    )
endif()

# -----------------------------------------------------------------------------
# Link directories / libs
# -----------------------------------------------------------------------------
set(MX_LINK_DIRS)

set(MX_LINK_LIBS
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
)

# -----------------------------------------------------------------------------
# MCU flags for STM32G474xx (Cortex-M4F)
# -----------------------------------------------------------------------------
set(MCU_CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
)

set(MCU_FPU_FLAGS
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

set(MCU_COMMON_FLAGS
    ${MCU_CPU_FLAGS}
    ${MCU_FPU_FLAGS}
)

# -----------------------------------------------------------------------------
# Interface library for common CubeMX settings
# -----------------------------------------------------------------------------
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})
target_compile_options(stm32cubemx INTERFACE ${MCU_COMMON_FLAGS})
target_link_options(stm32cubemx INTERFACE ${MCU_COMMON_FLAGS})

# -----------------------------------------------------------------------------
# HAL drivers object library
# -----------------------------------------------------------------------------
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)
target_compile_options(STM32_Drivers PRIVATE ${MCU_COMMON_FLAGS})

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup_stm32g474xx.s
    PROPERTIES LANGUAGE ASM
)

# -----------------------------------------------------------------------------
# Apply CubeMX sources/settings to main target
# -----------------------------------------------------------------------------
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_LIBS} stm32cubemx)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${MCU_COMMON_FLAGS})
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE ${MCU_COMMON_FLAGS})

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map
)

if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(FATAL_ERROR "Generated code requires C11 or higher")
endif()