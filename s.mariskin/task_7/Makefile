CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2
TARGET = line_lookup_mmap

.PHONY: all clean
all: $(TARGET)

$(TARGET): line_lookup_mmap.c ../common/line_lookup.h
	$(CC) $(CFLAGS) -o $(TARGET) line_lookup_mmap.c

clean:
	rm -f $(TARGET)
