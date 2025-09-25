CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: mycat mygrep

mycat: mycat.c
	$(CC) $(CFLAGS) -o mycat mycat.c

mygrep: mygrep.c
	$(CC) $(CFLAGS) -o mygrep mygrep.c

clean:
	rm -f mycat mygrep

.PHONY: all clean
