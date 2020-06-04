target := wallsw

# TODO: Remove dependency on softPwm softTone piHiPri
OBJS = minini/minIni.o wiringPi/wiringPi.o wiringPi/piHiPri.o wiringPi/softPwm.o wiringPi/softTone.o mcp23017.o stm.o main.o 

CC := gcc
OPT := -g -O0
INCLUDE :=
CFLAGS := $(OPT) $(INCLUDE)

all: $(target)

$(target): $(OBJS)
	gcc $(OPT) -o $(target) $(OBJS)

clean:
	rm -f $(target) $(OBJS)
