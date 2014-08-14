
LIB = rsLib

OBJ = timer.o packet.o sio.o adc.o

LIBS =

CFLAGS = -g -O0 -I ./include -I ./

LDFLAGS = -g

TARGET = lib/lib$(LIB).a

all: $(TARGET)

$(TARGET): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

.o: .c

clean:
	rm -rf *.o $(TARGET) *.bak 
