cmake_minimum_required(VERSION 3.13)

# TODO(timnn): document usage of Arduino libs, like EEPROM.

# First things first, set up some things that need to be defined early.
if(NOT CALEIDOSCOPE_HOST)
    # Such as the Arduino toolchain.
    set(CMAKE_TOOLCHAIN_FILE dep/arduino-cmake-ng/cmake/Arduino-Toolchain.cmake)
    # And some of its settings.
    set(ARDUINO_CMAKE_PLATFORM_PROPERTIES_FILE_PATH dep/bundle/avr/platform.txt)
    set(ARDUINO_CMAKE_PLATFORM_BOARDS_PATH dep/bundle/avr/boards.txt)
    set(ARDUINO_CMAKE_PLATFORM_VARIANTS_PATH dep/bundle/avr/variants)
    # Make some later conditions nicer.
    set(CALEIDOSCOPE_ARDUINO 1)
else()
    # Build with debug symbols by default.
    set(default_build_type "Debug")
endif()

project(caleidoscope)

# dep/bundle/avr/platform.txt uses these, so let's match that.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# Set up some global defines. [CUSTOMIZE]
add_definitions('-DUSB_VID=0x1209')
add_definitions('-DUSB_PID=0x2301')
add_definitions('-DUSB_MANUFACTURER="Keyboardio"')
add_definitions('-DUSB_PRODUCT="Model 01"')

# Now some platform specific set up, first for Arduino.
if(CALEIDOSCOPE_ARDUINO)
    # Change this when building for something other than the Model01. [CUSTOMIZE]
    add_definitions('-DKALEIDOSCOPE_HARDWARE_H="Kaleidoscope-Hardware-Model01.h"')

    get_board_id(board_id model01)
endif()

# Then for the Host.
if(CALEIDOSCOPE_HOST)
    add_definitions('-DARDUINO_VIRTUAL')
    add_definitions('-DKALEIDOSCOPE_HARDWARE_H="Kaleidoscope-Hardware-Virtual.h"')
endif()

include_directories(
        dep/bundle/avr/libraries/HID/src
        dep/bundle/avr/libraries/Kaleidoscope/src
        dep/bundle/avr/libraries/Kaleidoscope-HIDAdaptor-KeyboardioHID/src
        dep/bundle/avr/libraries/KeyboardioHID/src
        dep/bundle/avr/libraries/KeyboardioScanner)

# Below are lists of various sources. Use something like
# > find dep/bundle/avr/libraries -type f \( -iname '*.cpp' -o -iname '*.c' \) | sort | grep [-v] /whatever/
# to (re-)generate them.

# Generic Kaleidoscope sources, used by all firmware builds (for real hardware).
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

# Hardware specific Kaleidoscope sources, change this when building for alternative hardware. [CUSTOMIZE]
set(kaleidoscope_hw_SOURCES
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/hardware/ATMegaKeyboard.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/hardware/keyboardio/Model01.cpp)

# Source files of all used plugins (and their dependencies). [CUSTOMIZE]
set(kaleidoscope_plugin_SOURCES
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/plugin/HostPowerManagement.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/plugin/LEDControl.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/plugin/LEDControl/LED-Off.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/plugin/LEDControl/LEDUtils.cpp
        dep/bundle/avr/libraries/Kaleidoscope/src/kaleidoscope/plugin/LEDEffect-SolidColor.cpp)

set(my_plugin_INCLUDE_DIRS
        src/plugins)

# Sources for plugins not (yet) included in kaleidoscope. [CUSTOMIZE]
set(my_plugin_SOURCES
        src/plugins/TapMod.cpp)

set(virtual_INCLUDE_DIRS
        dep/bundle/avr/libraries/Kaleidoscope-Hardware-Virtual/src
        dep/bundle/avr/libraries/KeyboardioHID/src
        dep/bundle/avr/libraries/Kaleidoscope/src
        dep/bundle/x86/cores/virtual)

# Targets for the real hardware.
if(CALEIDOSCOPE_ARDUINO)
    # Default firmware. Change the src/sketch/ file here or in a copy. [CUSTOMIZE]
    add_arduino_executable(caleidoscope ${board_id}
            src/sketch/timnn.cpp
            ${my_plugin_SOURCES}
            ${kaleidoscope_gen_SOURCES}
            ${kaleidoscope_hw_SOURCES}
            ${kaleidoscope_plugin_SOURCES})
    target_include_directories(caleidoscope PRIVATE ${my_plugin_INCLUDE_DIRS} ${kaleidoscope_INCLUDE_DIRS})
endif()

function(define_test TEST_BASE_NAME)
    add_executable(${TEST_BASE_NAME} tests/main.cpp "tests/${TEST_BASE_NAME}.cpp" ${my_plugin_SOURCES})
    target_include_directories(${TEST_BASE_NAME} PRIVATE ${my_plugin_INCLUDE_DIRS} ${virtual_INCLUDE_DIRS})
    target_link_libraries(${TEST_BASE_NAME} gtest_main)
    add_test(NAME ${TEST_BASE_NAME} COMMAND ${TEST_BASE_NAME})
endfunction()

# Targets for the host.
if(CALEIDOSCOPE_HOST)
    enable_testing()
    add_subdirectory(dep/googletest EXCLUDE_FROM_ALL)

    define_test(TapModTest)
endif()