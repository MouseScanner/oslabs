CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread -D_DEFAULT_SOURCE

all: threads sender receiver

threads: main.c
	$(CC) $(CFLAGS) -o threads main.c

sender: sender.c
	$(CC) $(CFLAGS) -o sender sender.c

receiver: receiver.c
	$(CC) $(CFLAGS) -o receiver receiver.c

clean:
	rm -f threads sender receiver
	ipcrm -M 5678 2>/dev/null || true
	ipcrm -S 5679 2>/dev/null || true

.PHONY: all clean
