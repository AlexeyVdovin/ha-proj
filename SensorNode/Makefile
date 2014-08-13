
PRG = SensorNode

OBJ = main.o sio.o timer.o packet.o adc.o

LIBS =

CFLAGS = -g -O0

LDFLAGS = -g

TARGET = $(PRG).elf

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

.o: .c

clean:
	rm -rf *.o $(TARGET) *.bak 
