CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 $(shell pkg-config --cflags gtk4)
LDFLAGS ?= $(shell pkg-config --libs gtk4) -lm

TARGET = app
SRCS = main.c pixel_buffer.c renderer.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
