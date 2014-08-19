
PRG = SensorNode

MCU_TARGET = atmega8

OBJ = main.o 

AVR_OBJ = $(patsubst %.o,%.obj,$(OBJ))
#AVR_SRC = $(patsubst target/%.o,%.c,AVR_OBJ)

TARGET = $(PRG).elf

LIBS = -lrsLib
CFLAGS = -g -O0 -I ../rsLib/include -I ./
LDFLAGS = -g -L ../rsLib/lib -static

AVR_LIBS = 
AVR_CFLAGS = -g -Wall -O2 -mmcu=$(MCU_TARGET)
AVR_LDFLAGS = -Wl,-Map,target/$(PRG).map

AVR_CC      = avr-gcc
AVR_OBJCOPY = avr-objcopy
AVR_OBJDUMP = avr-objdump

all: $(TARGET) target/$(TARGET)

mcu: target/$(TARGET) lst text eeprom

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

# .o: .c

%.obj: %.c
	$(AVR_CC) $(AVR_CFLAGS) -c -o $@ $^

target/$(TARGET): $(AVR_OBJ)
	$(AVR_CC) $(AVR_CFLAGS) -o $@ $^ $(AVR_LDFLAGS) $(AVR_LIBS)

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
	rm -rf *.o $(TARGET) *.bak 
