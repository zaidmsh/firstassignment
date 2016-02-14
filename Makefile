CFLAGS=-Wall -O3
LDFLAGS= -L.
LDLIBS=

OBJS=main.o lookup.o

all: main

main: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: clean
clean:
	-rm main *.o
