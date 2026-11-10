CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2
TARGET = line_lookup_alarm

.PHONY: all clean
all: $(TARGET)

$(TARGET): line_lookup_alarm.c ../common/lookup_io.c ../common/line_lookup.h
	$(CC) $(CFLAGS) -o $(TARGET) line_lookup_alarm.c ../common/lookup_io.c

clean:
	rm -f $(TARGET)
