#!/usr/bin/env bash
# kaleidoscope-builder - Kaleidoscope helper tool
# Copyright (C) 2017-2018  Keyboard.io, Inc.
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <http://www.gnu.org/licenses/>.

set -e

ELF_FILE_PATH="./caleidoscope.elf"
AVRDUDE="/Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin/avrdude"
AVRDUDE_CONF="/Applications/Arduino.app/Contents/Java/hardware/tools/avr/etc/avrdude.conf"
MCU="atmega32u4"

reset_device_cmd() {
    stty -F ${DEVICE_PORT} 1200 hupcl
}

prepare_to_flash () {
    echo "To update your keyboard's firmware, hold down the 'Prog' key on your keyboard,"
    echo "and then press 'Enter'."
    echo ""
    echo "When the 'Prog' key glows red, you can release it."
    echo ""

    # We do not want to permit line continuations here. We just want a newline.
    # shellcheck disable=SC2162
    read
}

flash () {
    prepare_to_flash

    reset_device
    DEVICE_PORT_BOOTLOADER="/dev/tty.usbmodemkbio01"
    check_bootloader_port_and_flash
}


check_bootloader_port_and_flash () {
    if [ -z "${DEVICE_PORT_BOOTLOADER}" ]; then
        echo "Unable to detect a keyboard in bootloader mode. You may need to hold the 'Prog' key or hit a reset button"
        return 1
    fi
    flash_over_usb || flash_over_usb
}

flash_over_usb () {
    sleep 1
    ${AVRDUDE} -q -q -C "${AVRDUDE_CONF}" -p"${MCU}" -cavr109 -D -P "${DEVICE_PORT_BOOTLOADER}" -b57600 "-Uflash:w:${ELF_FILE_PATH}"
}

reset_device() {
    DEVICE_PORT="/dev/tty.usbmodemCkbio01E"
    check_device_port
    reset_device_cmd
}

check_device_port () {
    if [ -z "$DEVICE_PORT" ]; then
        cat <<EOF >&2

I couldn't autodetect the keyboard's serial port.

If you see this message and your keyboard is connected to your computer,
it may mean that our serial port detection logic is buggy or incomplete.
In that case, please report this issue at:
     https://github.com/keyboardio/Kaleidoscope
EOF
        exit 1
    elif echo "$DEVICE_PORT" | grep -q '[[:space:]]'; then
        cat <<EOF >&2
Unexpected whitespace found in detected serial port:

    $DEVICE_PORT

If you see this message, it means that our serial port
detection logic is buggy or incomplete.

Please report this issue at:
     https://github.com/keyboardio/Kaleidoscope
EOF
        exit 1
    fi

    if ! [ -w "$DEVICE_PORT" ]; then
        cat <<EOF >&2

In order to update your keyboard's firmware you need to have permission
to write to its serial port $DEVICE_PORT.

It appears that you do not have this permission:

  $(ls -l "$DEVICE_PORT")

This may be because you're not in the correct unix group:

  $(stat -c %G "$DEVICE_PORT").

You are currently in the following groups:

  $(id -Gn)

Please ensure you have followed the instructions on setting up your
account to be in the right group:

https://github.com/keyboardio/Kaleidoscope/wiki/Install-Arduino-support-on-Linux

EOF
        exit 1
    fi
}

flash
