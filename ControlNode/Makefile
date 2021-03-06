
PRG = ControlNode

MCU_TARGET = atmega644

LIB = ../rsLib

OBJ = main.o config.o command.o $(LIB)/led_display.o $(LIB)/rs485.o $(LIB)/timer.o $(LIB)/sio.o $(LIB)/packet.o $(LIB)/adc.o $(LIB)/sw_pwm.o $(LIB)/ds1820.o

AVR_OBJ = $(patsubst %.o,%.obj,$(OBJ))

TARGET = $(PRG).elf

INCLUDE = -I ../rsLib/include -I ./

CC = gcc

CFLAGS = -g -O0 $(INCLUDE)
LDFLAGS = -g -L ../rsLib/lib

AVR_CFLAGS = -g -Wall -O2 -mmcu=$(MCU_TARGET) $(INCLUDE)
AVR_LDFLAGS = -Wl,-Map,target/$(PRG).map

AVR_CC      = avr-gcc
AVR_OBJCOPY = avr-objcopy
AVR_OBJDUMP = avr-objdump

all: $(TARGET) mcu

mcu: target/$(TARGET) lst text eeprom

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.obj: %.c
	$(AVR_CC) $(AVR_CFLAGS) -c -o $@ $^

target/$(TARGET): $(AVR_OBJ)
	$(AVR_CC) $(AVR_CFLAGS) -o $@ $^ $(AVR_LDFLAGS)

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
	rm -f $(OBJ) $(AVR_OBJ) $(TARGET) target/*.*


