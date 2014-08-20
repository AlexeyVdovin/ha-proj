
PRG = rsGate

OBJ = main.o rs485.o

LIBS = -lrsLib

CFLAGS = -g -O0 -I../rsLib/include -I./
LDFLAGS = -g -L../rsLib/lib

TARGET = $(PRG).elf

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.o: .c

clean:
	rm -rf *.o $(TARGET) *.bak 
