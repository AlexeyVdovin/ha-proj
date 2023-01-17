target := green

OBJS = minini/minIni.o wiringPi/wiringPi.o pca9554.o ds2482.o stm.o main.o uplink.o

CC := gcc
OPT := -g -O0
INCLUDE := -Imqtt-c/include
CFLAGS := $(OPT) $(INCLUDE)

all: $(target)

$(target): $(OBJS)
	gcc $(OPT) -o $(target) $(OBJS) -lmemcached -lcurl

clean:
	rm -f $(target) $(OBJS)
