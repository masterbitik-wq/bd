CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -O2
LDFLAGS = -lsqlite3 -lm
TARGET = stats

all: $(TARGET)

$(TARGET): bd.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bd.o: bd.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean