.PHONY = all clean prepare

CC = gcc
CFLAGS = -Wall -Iinclude
LDLIBS = -lwiringPi

ALL_UTILS = readID

all: $(addprefix output/, $(ALL_UTILS))

clean:
	rm -rf output
	-rm src/*.o util/*.o

prepare:
	mkdir -p output

src/spi_flash.o: src/spi_flash.c include/spi_flash.h
	$(CC) $(CFLAGS) -c $< -o $@

util/readID.o: util/readID.c include/spi_flash.h
	$(CC) $(CFLAGS) -c $< -o $@

output/readID: util/readID.o src/spi_flash.o | prepare
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@
