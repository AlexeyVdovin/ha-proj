
PRG = GreenNode

MCU_TARGET = atmega328p

LIB = ../rsLib

OBJ = main.o config.o $(LIB)/timer.o $(LIB)/adc.o $(LIB)/sio.o

AVR_OBJ = $(patsubst %.o,%.obj,$(OBJ))

INCLUDE = -I ../rsLib/include -I ./

AVR_CFLAGS = -g -Wall -Os -mmcu=$(MCU_TARGET) $(INCLUDE)
AVR_TARGET_LDFLAGS = -Wl,-Map,target/$(PRG).map

AVR_CC      = avr-gcc
AVR_OBJCOPY = avr-objcopy
AVR_OBJDUMP = avr-objdump

all: mcu

mcu: target/$(PRG).elf lst text eeprom

%.obj: %.c
	$(AVR_CC) $(AVR_CFLAGS) -c -o $@ $^

target/$(PRG).elf: $(AVR_OBJ)
	$(AVR_CC) $(AVR_CFLAGS) -o $@ $^ $(AVR_TARGET_LDFLAGS)

lst:  target/$(PRG).lst

%.lst: %.elf
	$(AVR_OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin

hex:  target/$(PRG).hex
bin:  target/$(PRG).bin

%.hex: %.elf
	$(AVR_OBJCOPY) -j .text -j .data -O ihex $< $@

%.bin: %.elf
	$(AVR_OBJCOPY) -j .text -j .data -O binary $< $@

# Rules for building the .eeprom rom images

eeprom: ehex ebin

ehex:  target/$(PRG)_eeprom.hex
ebin:  target/$(PRG)_eeprom.bin

%_eeprom.hex: %.elf
	$(AVR_OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@ || { echo empty $@ not generated; exit 0; }

%_eeprom.bin: %.elf
	$(AVR_OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@ || { echo empty $@ not generated; exit 0; }

clean:
	rm -f $(AVR_OBJ) target/*.*

