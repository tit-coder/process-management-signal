CC = gcc
CFLAGS = -Wall -Wextra -std=c11

SRC = src/main.c \
      src/process_manager.c \
      src/signal_handler.c \
      src/monitor.c

TARGET = process_manager

.PHONY: all run clean

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
