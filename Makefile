CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -I. $(shell pkg-config --cflags gtk4)
LDFLAGS ?= $(shell pkg-config --libs gtk4) -lm

TARGET = polypen
SRCS = app/main.c controller/drawing_area_controller.c controller/edge_menu_controller.c model/app_state.c model/edge.c model/point.c view/pixel_buffer.c view/renderer.c
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
