LIB = rsLib

MCU_TARGET = atmega8

OBJ = timer.o packet.o sio.o adc.o

AVR_OBJ = $(patsubst %.o,%.obj,$(OBJ))

LIBS =
INCLUDE = -I ./include -I ./

CFLAGS = -g -O0 $(INCLUDE)

AVR_LIBS = 
AVR_CFLAGS = -g -Wall -O2 -mmcu=$(MCU_TARGET) $(INCLUDE)

AVR_CC      = avr-gcc
AVR_AR      = avr-ar

CC = gcc

TARGET = lib/lib$(LIB).a

all: $(TARGET) mcu

mcu: target/lib$(LIB).a

$(TARGET): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

target/lib$(LIB).a: $(AVR_OBJ)
	$(AVR_AR) $(ARFLAGS) $@ $^

%.obj: %.c
	$(AVR_CC) $(AVR_CFLAGS) -c -o $@ $^

clean:
	rm -f *.o *.obj $(TARGET) target/lib$(LIB).a

