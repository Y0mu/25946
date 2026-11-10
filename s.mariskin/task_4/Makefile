CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2

TARGET = list_strings

.PHONY: all clean

all: $(TARGET)

$(TARGET): list_strings.c
	$(CC) $(CFLAGS) -o $(TARGET) list_strings.c

clean:
	rm -f $(TARGET)
