.PHONY = all clean prepare

CC = gcc
CFLAGS = -Wall -Iinclude -g
LDLIBS = -lwiringPi

ALL_UTILS = dump_flash readID read

all: $(addprefix output/, $(ALL_UTILS))

clean:
	rm -rf output
	-rm src/*.o util/*.o

prepare:
	mkdir -p output

src/spi_flash.o: src/spi_flash.c include/spi_flash.h
	$(CC) $(CFLAGS) -c $< -o $@

$(addprefix output/, $(ALL_UTILS)): output/%: util/%.o src/spi_flash.o | prepare
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(addprefix util/, $(addsuffix .o, $(ALL_UTILS))): util/%.o: util/%.c include/spi_flash.h
	$(CC) $(CFLAGS) -c $< -o $@
