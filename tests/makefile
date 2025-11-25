CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -I../include
ENGINE_DIR = ../engine/serial
ENGINE_DIR_MAIN = ../engine
ENGINE_SOURCES = $(wildcard $(ENGINE_DIR)/*.c)
ENGINE_OBJECTS = $(ENGINE_SOURCES:.c=.o)
RECORD_SCHEMA_OBJ = $(ENGINE_DIR_MAIN)/recordSchema.o

TARGET = bplus-serial-test
TEST_SOURCE = bplus-serial-test.c

all: $(TARGET)

$(TARGET): $(TEST_SOURCE) $(ENGINE_OBJECTS) $(RECORD_SCHEMA_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(ENGINE_DIR)/%.o: $(ENGINE_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ENGINE_DIR_MAIN)/%.o: $(ENGINE_DIR_MAIN)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(TARGET) $(ENGINE_OBJECTS) $(RECORD_SCHEMA_OBJ)