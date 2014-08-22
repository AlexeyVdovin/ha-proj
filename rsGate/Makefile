
PRG = rsGate

OBJ = main.o rs485.o queue.o

LIBS = -lrsLib

CC = gcc

CFLAGS = -g -O0 -I../rsLib/include -I./
LDFLAGS = -g -L../rsLib/lib

TARGET = $(PRG).elf

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf *.o $(TARGET) *.bak 
