target := slave

OBJS = slave.o proto.o

CC := gcc
CXX := g++
OPT := -g -O0
INCLUDE :=
CFLAGS := $(OPT) $(INCLUDE)
CXXFLAGS := $(OPT) $(INCLUDE)

all: $(target)

$(target): $(OBJS)
	$(CXX) $(OPT) -o $(target) $(OBJS)

clean:
	rm -f $(target) $(OBJS)
