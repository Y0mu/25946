CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = line_lookup_alarm

all: $(TARGET)

$(TARGET): line_lookup_alarm.c ../Task_5/line_lookup.c
	$(CC) $(CFLAGS) -o $(TARGET) line_lookup_alarm.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
