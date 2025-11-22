CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE

all: pipe_fork fifo_sender fifo_receiver

pipe_fork: pipe_fork.c
	$(CC) $(CFLAGS) -o pipe_fork pipe_fork.c

fifo_sender: fifo_sender.c
	$(CC) $(CFLAGS) -o fifo_sender fifo_sender.c

fifo_receiver: fifo_receiver.c
	$(CC) $(CFLAGS) -o fifo_receiver fifo_receiver.c

clean:
	rm -f pipe_fork fifo_sender fifo_receiver
	rm -f /tmp/lab6_fifo

.PHONY: all clean
