CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall -Wextra $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lm
TARGET  = chip
SRC     = chip.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

# usage: make run ROM=Pong.ch8
run: $(TARGET)
	./$(TARGET) $(ROM)

clean:
	rm -f $(TARGET)
