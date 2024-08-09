CC = gcc
LFLAGS = -lm -lpthread
CFLAGS = -Wall -Werror -D_GNU_SOURCE

all: mmio_dump mmio_write tpmi_dump devmem

%.o: %.c 
	$(CC) -o $@ $(CFLAGS) -c $<

mmio_dump: mmio_dump.o
	$(CC) -o $@ $^ $(LFLAGS)

mmio_write: mmio_write.o
	$(CC) -o $@ $^ $(LFLAGS)

tpmi_dump: tpmi_dump.o
	$(CC) -o $@ $^ $(LFLAGS) -lpcre

devmem: devmem.o
	$(CC) -o $@ $^ $(LFLAGS)

.PHONY: clean
clean:
	rm -f *.o mmio_dump mmio_write tpmi_dump devmem
