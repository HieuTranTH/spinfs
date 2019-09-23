.PHONY = all clean prepare

CC = gcc
CFLAGS = -Wall -Iinclude -g
LDLIBS = -lwiringPi

ifneq ($(VALGRIND),)
CFLAGS += -DSPI_SPEED=25000000
endif

ALL_UTILS = dump_flash erase_block erase_chip erase_sector readID read write \
			read_security write_security erase_security

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
