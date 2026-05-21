CC := gcc
CPPFLAGS := -Iinclude
CFLAGS := -Wall -Wextra -std=gnu11 -g

TARGET := main
SRCS := main.c src/animal.c src/cat.c src/dog.c
OBJS := $(SRCS:.c=.o)
HEADERS := include/animal.h include/cat.h include/dog.h

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
