CC = g++
CFLAGS = -std=c++17 -O3 -flto -funroll-loops -march=native

SRCS = position.cc engine.cc movegen.cc
OBJS = $(SRCS:.cc=.o)

TARGETS = uci

all: $(TARGETS)

uci: uci.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o */*.o $(TARGETS)