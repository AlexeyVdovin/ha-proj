target := wallsw

OBJS = minini/minIni.o wiringPi/wiringPi.o mqtt-c/src/mqtt.o mqtt-c/src/mqtt_pal.o uplink.o mcp23017.o pca9454.o stm.o main.o 

CC := gcc
OPT := -g -O0
INCLUDE := -Imqtt-c/include
CFLAGS := $(OPT) $(INCLUDE)

all: $(target)

$(target): $(OBJS)
	gcc $(OPT) -o $(target) $(OBJS)

clean:
	rm -f $(target) $(OBJS)
