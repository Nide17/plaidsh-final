CC=gcc
CFLAGS=-Wall -Werror -g
LIBS=-lreadline

all: plaidsh test

plaidsh: parser.o plaidsh.o command.o
	gcc $(LDFLAGS) $^ $(LIBS) -o $@

test_parser: parser.o test_parser.o command.o
	gcc $(LDFLAGS) $^ -o test_parser

test_command: command.c
	gcc $(CFLAGS) -D RUN_TESTS command.c -o test_command

test: test_parser test_command
	./test_command > /dev/null
	./test_parser

%.o: %.c %.h
	gcc -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o test_parser test_command plaidsh
