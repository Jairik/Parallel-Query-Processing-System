CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -I../include
ENGINE_DIR = ../engine/serial
ENGINE_DIR_MAIN = ../engine
ENGINE_SOURCES = $(wildcard $(ENGINE_DIR)/*.c)
ENGINE_OBJECTS = $(ENGINE_SOURCES:.c=.o)
BPLUS_OBJ = $(ENGINE_DIR_MAIN)/bplus.o
RECORD_SCHEMA_OBJ = $(ENGINE_DIR_MAIN)/recordSchema.o
TOKENIZER_SRC = ../tokenizer/src/tokenizer.c
TOKENIZER_OBJ = ../tokenizer/src/tokenizer.o

TARGETS = bplus-serial-test serial-SELECT-test

all: $(TARGETS)

bplus-serial-test: bplus-serial-test.c $(ENGINE_OBJECTS) $(RECORD_SCHEMA_OBJ) $(BPLUS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

serial-SELECT-test: serial-SELECT-test.c $(ENGINE_OBJECTS) $(RECORD_SCHEMA_OBJ) $(TOKENIZER_OBJ) $(BPLUS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(TOKENIZER_OBJ): $(TOKENIZER_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(ENGINE_DIR)/%.o: $(ENGINE_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ENGINE_DIR_MAIN)/%.o: $(ENGINE_DIR_MAIN)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(TARGETS) $(ENGINE_OBJECTS) $(RECORD_SCHEMA_OBJ) $(TOKENIZER_OBJ)