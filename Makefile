CFLAGS=-Wall -O3 `pkg-config --cflags glib-2.0`
LDFLAGS= -L.
LDLIBS=`pkg-config --libs glib-2.0`

OBJS=main.o lookup.o

all: main

main: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: clean
clean:
	-rm main *.o
