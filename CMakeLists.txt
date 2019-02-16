cmake_minimum_required(VERSION 3.13)

set(CMAKE_TOOLCHAIN_FILE dep/arduino-cmake-ng/cmake/Arduino-Toolchain.cmake)
set(ARDUINO_CMAKE_PLATFORM_PROPERTIES_FILE_PATH dep/bundle/avr/platform.txt)
set(ARDUINO_CMAKE_PLATFORM_BOARDS_PATH dep/bundle/avr/boards.txt)
set(ARDUINO_CMAKE_PLATFORM_VARIANTS_PATH dep/bundle/avr/variants)

add_definitions('-DKALEIDOSCOPE_HARDWARE_H="Kaleidoscope-Hardware-Model01.h"')
add_definitions('-DUSB_VID=0x1209')
add_definitions('-DUSB_PID=0x2301')
add_definitions('-DUSB_MANUFACTURER="Keyboardio"')
add_definitions('-DUSB_PRODUCT="Model 01"')

project(caleidoscope)
get_board_id(board_id model01)

# To match dep/bundle/avr/platform.txt
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

include_directories(
        dep/bundle/avr/libraries/HID/src
        dep/bundle/avr/libraries/Kaleidoscope/src
        dep/bundle/avr/libraries/Kaleidoscope-HIDAdaptor-KeyboardioHID/src
        dep/bundle/avr/libraries/KeyboardioHID/src
        dep/bundle/avr/libraries/KeyboardioScanner)

set(kaleidoscope_gen_SOURCES
        dep/bundle/avr/libraries/HID/src/HID.cpp
        dep/bundle/avr/libraries/Kaleidoscope-HIDAdaptor-KeyboardioHID/src/kaleidoscope/hid.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/Kaleidoscope.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/hooks.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/key_events.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/layers.cpp
        dep/bundle/avr/libraries/KeyboardioHID/src/BootKeyboard/BootKeyboard.cpp
        dep/bundle/avr/libraries/KeyboardioHID/src/MultiReport/AbsoluteMouse.cpp
        dep/bundle/avr/libraries/KeyboardioHID/src/MultiReport/ConsumerControl.cpp
        dep/bundle/avr/libraries/KeyboardioHID/src/MultiReport/Gamepad.cpp
        dep/bundle/avr/libraries/KeyboardioHID/src/MultiReport/Keyboard.cpp
        dep/bundle/avr/libraries/KeyboardioHID/src/MultiReport/Mouse.cpp
        dep/bundle/avr/libraries/KeyboardioHID/src/MultiReport/SystemControl.cpp
        dep/bundle/avr/libraries/KeyboardioHID/src/SingleReport/SingleAbsoluteMouse.cpp
        dep/bundle/avr/libraries/KeyboardioScanner/KeyboardioScanner.cpp
        dep/bundle/avr/libraries/KeyboardioScanner/twi.c)

set(kaleidoscope_hw_SOURCES
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/hardware/ATMegaKeyboard.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/hardware/keyboardio/Model01.cpp)

set(kaleidoscope_plugin_SOURCES
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/plugin/HostPowerManagement.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/plugin/LEDControl.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/plugin/LEDControl/LED-Off.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/plugin/LEDControl/LEDUtils.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/plugin/LEDEffect-SolidColor.cpp)

set(custom_SOURCES
        main.cpp)

add_arduino_executable(caleidoscope ${board_id}
        ${custom_SOURCES}
        ${kaleidoscope_gen_SOURCES}
        ${kaleidoscope_hw_SOURCES}
        ${kaleidoscope_plugin_SOURCES})
