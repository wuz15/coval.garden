CC = gcc
LFLAGS = -lm -lpthread
CFLAGS = -Wall -Werror -D_GNU_SOURCE

BIN_DIR := $(PWD)/../bin

all: mmio_dump mmio_write tpmi_dump devmem

%.o: %.c 
	$(CC) -o $@ $(CFLAGS) -c $<

mmio_dump: mmio_dump.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(LFLAGS)

mmio_write: mmio_write.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(LFLAGS)

tpmi_dump: tpmi_dump.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(LFLAGS) -lpcre

devmem: devmem.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(LFLAGS)

.PHONY: clean
clean:
	rm -f *.o $(BIN_DIR)/mmio_dump $(BIN_DIR)/mmio_write $(BIN_DIR)/tpmi_dump $(BIN_DIR)/devmem
