.PHONY = all clean prepare

ALL_TARGET = readID spi_flash.o

CFLAGS = -Wall -Iinclude -lwiringPi

all: prepare $(ALL_TARGET)

clean:
	rm -rf output
	rm -f src/*.o

prepare:
	mkdir -p output

readID: src/util/readID.c spi_flash.o
	gcc src/util/readID.c src/spi_flash.o -o output/readID $(CFLAGS)

spi_flash.o: src/spi_flash.c include/spi_flash.h
	gcc -c src/spi_flash.c -o src/spi_flash.o $(CFLAGS)
