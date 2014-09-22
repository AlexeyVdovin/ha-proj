
PRG = rsGate

LIB = ../rsLib

OBJ = main.o queue.o $(LIB)/udp.o $(LIB)/rs485.o $(LIB)/packet.o $(LIB)/timer.o $(LIB)/sio.o

CC = gcc

CFLAGS = -g -O0 -I../rsLib/include -I./
LDFLAGS = -g

TARGET = $(PRG).elf

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(OBJ) $(TARGET) *.bak 
