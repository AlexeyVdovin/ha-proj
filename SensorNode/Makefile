
PRG = SensorNode

OBJ = main.o 

LIBS = -lrsLib

CFLAGS = -g -O0 -I ../rsLib/include -I ./

LDFLAGS = -g -L ../rsLib/lib -static

TARGET = $(PRG).elf

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

.o: .c

clean:
	rm -rf *.o $(TARGET) *.bak 
