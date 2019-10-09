#!/bin/bash

avrdude -v -C ./avrdude.conf -c ft232r -P ft0 -p m328p -U flash:w:target/GreenNode.hex
