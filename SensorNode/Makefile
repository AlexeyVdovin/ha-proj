
PRG = SensorNode

MCU_TARGET = atmega8

LIB = ../rsLib

OBJ = main.o command.o config.o $(LIB)/timer.o $(LIB)/udp.o $(LIB)/adc.o $(LIB)/packet.o $(LIB)/pwm.o $(LIB)/sio.o $(LIB)/rs485.o $(LIB)/ds1820.o

AVR_OBJ = $(patsubst %.o,%.obj,$(OBJ))
BTL_OBJ = $(LIB)/sio.obj $(LIB)/timer.obj $(LIB)/rs485.obj $(LIB)/packet.obj config.obj bootloader.obj

TARGET = $(PRG).elf
BOOTLOADER = bootloader.elf

INCLUDE = -I ../rsLib/include -I ./

CC = gcc

CFLAGS = -g -O0 $(INCLUDE)
LDFLAGS = -g

AVR_CFLAGS = -g -Wall -Os -mmcu=$(MCU_TARGET) $(INCLUDE) -D_CFG_NODE_MAC=0x12345678
AVR_TARGET_LDFLAGS = -Wl,-Map,target/$(PRG).map
AVR_BTLDR_LDFLAGS = -Wl,-section-start=.text=0x1800,-Map,target/bootloader.map 

AVR_CC      = avr-gcc
AVR_OBJCOPY = avr-objcopy
AVR_OBJDUMP = avr-objdump

all: $(TARGET) mcu

mcu: target/$(TARGET) target/$(BOOTLOADER) lst text eeprom

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.obj: %.c
	$(AVR_CC) $(AVR_CFLAGS) -c -o $@ $^

target/$(TARGET): $(AVR_OBJ)
	$(AVR_CC) $(AVR_CFLAGS) -o $@ $^ $(AVR_TARGET_LDFLAGS)
	
target/$(BOOTLOADER): $(BTL_OBJ)
	$(AVR_CC) $(AVR_CFLAGS) -o $@ $^ $(AVR_BTLDR_LDFLAGS)

lst:  target/$(PRG).lst target/bootloader.lst

%.lst: %.elf
	$(AVR_OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin

hex:  target/$(PRG).hex target/bootloader.hex
bin:  target/$(PRG).bin target/bootloader.bin

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
	rm -f $(OBJ) $(TARGET)
	rm -f $(AVR_OBJ) $(BTL_OBJ) target/*.*

