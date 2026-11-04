cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_BOARD)
  set(PICO_BOARD pico2_w)
endif()

if(NOT PICO_BOARD STREQUAL "pico2_w")
  message(FATAL_ERROR "MC BlueBridge requires PICO_BOARD=pico2_w")
endif()

if(DEFINED ENV{BTSTACK_ROOT})
  set(PICO_BTSTACK_PATH $ENV{BTSTACK_ROOT} CACHE PATH "" FORCE)
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/deps/btstack/src/btstack.h")
  set(PICO_BTSTACK_PATH "${CMAKE_CURRENT_LIST_DIR}/deps/btstack" CACHE PATH "" FORCE)
endif()

include(pico_sdk_import.cmake)
project(mc_bluebridge C CXX ASM)

option(BB_ENABLE_OLED "SSD1306 OLED and rotary encoder control panel" ON)
set(BB_OLED_I2C 0 CACHE STRING "I2C peripheral: 0 or 1")
set(BB_OLED_SDA 4 CACHE STRING "OLED SDA GPIO")
set(BB_OLED_SCL 5 CACHE STRING "OLED SCL GPIO")
set(BB_OLED_ADDRESS 0x3c CACHE STRING "SSD1306 7-bit I2C address")
set(BB_ENCODER_A 6 CACHE STRING "Encoder A GPIO")
set(BB_ENCODER_B 7 CACHE STRING "Encoder B GPIO")
set(BB_ENCODER_SW 8 CACHE STRING "Encoder push switch GPIO")
set(BB_ENCODER_STEPS 4 CACHE STRING "Quadrature transitions per detent: 2 or 4")
if(BB_ENABLE_OLED)
  set(panel_pins "")
  foreach(pin BB_OLED_SDA BB_OLED_SCL BB_ENCODER_A BB_ENCODER_B BB_ENCODER_SW)
    if(NOT "${${pin}}" MATCHES "^[0-9]+$")
      message(FATAL_ERROR "${pin} must be a GPIO number")
    endif()
    if(${${pin}} GREATER 28 OR (${${pin}} GREATER 22 AND ${${pin}} LESS 26))
      message(FATAL_ERROR "${pin} must be an exposed Pico GPIO: 0-22 or 26-28")
    endif()
    if("${${pin}}" IN_LIST panel_pins)
      message(FATAL_ERROR "Control panel GPIOs must be distinct")
    endif()
    list(APPEND panel_pins "${${pin}}")
  endforeach()
  if(NOT BB_OLED_I2C MATCHES "^[01]$" OR NOT BB_ENCODER_STEPS MATCHES "^[24]$")
    message(FATAL_ERROR "I2C peripheral must be 0/1 and encoder steps must be 2/4")
  endif()
  math(EXPR sda_function "${BB_OLED_SDA} % 4")
  math(EXPR expected_function "${BB_OLED_I2C} * 2")
  math(EXPR expected_scl "${BB_OLED_SDA} + 1")
  if(NOT sda_function EQUAL expected_function OR NOT BB_OLED_SCL EQUAL expected_scl)
    message(FATAL_ERROR "SDA/SCL do not form a valid pin pair for the selected I2C peripheral")
  endif()
  if(NOT BB_OLED_ADDRESS MATCHES "^(0[xX]3[cCdD]|60|61)$")
    message(FATAL_ERROR "SSD1306 address must be 0x3c or 0x3d")
  endif()
endif()

if(DEFINED ENV{LIBXSM3_ROOT})
  set(LIBXSM3_SOURCE_DIR $ENV{LIBXSM3_ROOT})
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/deps/libxsm3/xsm3.h")
  set(LIBXSM3_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/deps/libxsm3")
else()
  include(FetchContent)
  FetchContent_Declare(
    libxsm3
    GIT_REPOSITORY https://github.com/InvoxiPlayGames/libxsm3.git
    GIT_TAG master
    GIT_SHALLOW TRUE
  )
  FetchContent_GetProperties(libxsm3)
  if(NOT libxsm3_POPULATED)
    FetchContent_Populate(libxsm3)
  endif()
  set(LIBXSM3_SOURCE_DIR ${libxsm3_SOURCE_DIR})
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(mc_bluebridge
  src/main.c
  src/bluebridge_state.c
  src/bluebridge_platform.c
  src/bt_host.c
  src/controller_input.c
  src/controller_driver.c
  src/hid_feedback.c
  src/xinput_device.c
  src/xbox360_auth.c
  ${LIBXSM3_SOURCE_DIR}/xsm3.c
  ${LIBXSM3_SOURCE_DIR}/usbdsec.c
  ${LIBXSM3_SOURCE_DIR}/excrypt_des.c
  ${LIBXSM3_SOURCE_DIR}/excrypt_parve.c
  src/xsm3_excrypt_sha.c
  src/usb_device.c
  src/usb_descriptors.c
  src/cdc_protocol.c
  src/config_store.c
  src/led_status.c
  src/bootsel_button.c
  src/control_panel.c
)

target_compile_definitions(mc_bluebridge PRIVATE BB_ENABLE_OLED=$<BOOL:${BB_ENABLE_OLED}>)
if(BB_ENABLE_OLED)
  target_sources(mc_bluebridge PRIVATE src/oled_display.c src/oled_gamepad.c src/panel_controls.c)
  target_link_libraries(mc_bluebridge PRIVATE hardware_i2c)
  foreach(setting BB_OLED_I2C BB_OLED_SDA BB_OLED_SCL BB_OLED_ADDRESS BB_ENCODER_A BB_ENCODER_B BB_ENCODER_SW BB_ENCODER_STEPS)
    target_compile_definitions(mc_bluebridge PRIVATE ${setting}=${${setting}})
  endforeach()
endif()

target_include_directories(mc_bluebridge PRIVATE
  src
  ${LIBXSM3_SOURCE_DIR}
)

target_compile_definitions(mc_bluebridge PRIVATE XSM3_NO_DEBUGGING=1)

target_link_libraries(mc_bluebridge PUBLIC
  pico_stdlib
  pico_multicore
  pico_flash
  pico_unique_id
  hardware_flash
  hardware_watchdog
  hardware_sync
  tinyusb_device
  tinyusb_board
  pico_cyw43_arch_none
  pico_btstack_classic
  pico_btstack_ble
  pico_btstack_cyw43
  pico_btstack_flash_bank
)

pico_enable_stdio_usb(mc_bluebridge 0)
pico_enable_stdio_uart(mc_bluebridge 0)
pico_add_extra_outputs(mc_bluebridge)
