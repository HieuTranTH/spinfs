.PHONY = all clean prepare

CC = gcc
CFLAGS = -Wall -Iinclude -g
LDLIBS = -lwiringPi

ifneq ($(VALGRIND),)
CFLAGS += -DSPI_SPEED=25000000
endif

ALL_UTILS = dump_flash erase_block erase_chip erase_sector readID read write \
			read_security write_security erase_security \
			file_ops_example \
			#spinfs_cp

all: $(addprefix output/, $(ALL_UTILS)) \
	src/create_sim_flash src/scan_sim_flash

clean:
	rm -rf output
	-rm src/*.o util/*.o

prepare:
	mkdir -p output

src/spi_flash.o: src/spi_flash.c include/spi_flash.h
	$(CC) $(CFLAGS) -c $< -o $@

src/spinfs.o: src/spinfs.c include/spinfs.h include/spi_flash.h
	$(CC) $(CFLAGS) -c $< -o $@

src/create_sim_flash.o: src/create_sim_flash.c include/spinfs.h
	$(CC) $(CFLAGS) -c $< -o $@

src/create_sim_flash: src/create_sim_flash.o src/spinfs.o src/spi_flash.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

src/scan_sim_flash.o: src/scan_sim_flash.c include/spinfs.h
	$(CC) $(CFLAGS) -c $< -o $@

src/scan_sim_flash: src/scan_sim_flash.o src/spinfs.o src/spi_flash.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(addprefix util/, $(addsuffix .o, $(ALL_UTILS))): util/%.o: util/%.c include/spi_flash.h
	$(CC) $(CFLAGS) -c $< -o $@

$(addprefix output/, $(ALL_UTILS)): output/%: util/%.o src/spi_flash.o | prepare
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@
