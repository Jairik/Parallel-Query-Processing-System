CC=gcc
CFLAGS=-I../include -Wall -g

all: sql

sql: src/main.o src/tokenizer.o
	$(CC) $^ -o sql

clean:
	rm -f src/*.o sql
