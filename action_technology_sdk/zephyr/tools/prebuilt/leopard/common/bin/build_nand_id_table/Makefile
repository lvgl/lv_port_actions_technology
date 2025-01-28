.PHONY: all clean install

LD = ld
CC = gcc

CFLAGS := -Wall -O2 -g
LDFLAGS :=

TARGET := id_tbl_gen
SOURCES := $(wildcard *.c)
SOURCES_O := $(patsubst %c, %o, $(SOURCES))
ID_TABLE_OUTPUT := nand_id.bin

CFLAGS += -DID_TABLE_NAME=\"$(ID_TABLE_OUTPUT)\"

all: clean $(TARGET)

$(TARGET): $(SOURCES_O)
	$(CC) -o $(TARGET) $(SOURCES_O) $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(TARGET)
	rm -f $(ID_TABLE_OUTPUT)
