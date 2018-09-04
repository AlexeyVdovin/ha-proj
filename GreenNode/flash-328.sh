#!/bin/bash

id=$(id -u)

if [ _$id != _"0" ]; then
  echo "Error: ROOT privelege is required."
  exit 1
fi

device=$(ls -l /sys/bus/usb/drivers/ftdi_sio/ | grep devices | awk '{print $9}')
name=$(ls -1d /sys/bus/usb/drivers/ftdi_sio/$device/ttyU*)
tty=/dev/$(basename $name)

echo USB: $device
echo TTY: $tty

if [ _$device == _ ]; then
  echo "Error: USB device not found."
  exit 1
fi

function bind_tty {
  # Bind tty device
  echo -n "$device" > /sys/bus/usb/drivers/ftdi_sio/bind
  trap EXIT
}

function unbind_tty {
  # Unbind tty device
  echo -n "$device" > /sys/bus/usb/drivers/ftdi_sio/unbind
}

trap 'bind_tty' EXIT

unbind_tty

# Flash FUSE bits if requested
if [ _$1 == _"-F" ]; then
  shift
  avrdude -v -C ./avrdude.conf -c ft232r -P ft0 -p m328p -B 1200 -U lfuse:w:0xff:m -U hfuse:w:0xd1:m -U efuse:w:0xfd:m
  rc=$?
  if [ $rc -ne 0 ]; then
    echo "Error: FUSE failed. Exit code: $rc"
    exit 1
  fi
fi

# Flash firmware
avrdude -vvv -C ./avrdude.conf -c ft232r -P ft0 -p m328p -B 1200 -U flash:w:target/GreenNode.hex
rc=$?
if [ $rc -ne 0 ]; then
  echo "Error: Flash failed. Exit code: $rc"
  exit 1
fi

# Restore binding to tty
bind_tty

sleep 1

exit 0

cat $tty &
cat=$!

sleep 1

echo "VER:" > $tty

sleep 3

kill $cat

echo -e "\nDone."
