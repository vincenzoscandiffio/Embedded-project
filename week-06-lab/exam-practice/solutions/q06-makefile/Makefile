# Embedded Operating Systems and Architecture course
# Licensed under the MIT License. See LICENSE in the repository root.
#
# Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>

# Q6: the reference solution.

# ARM bare-metal cross-compiler, and its options.
CC := arm-none-eabi-gcc
CFLAGS := -mthumb -mcpu=cortex-m3 -ffreestanding -Wall -Iinclude
LDFLAGS := -nostdlib -Wl,-e,main

# The objects of the program build/prog.axf.
PROG_OBJECTS := build/main.o build/led.o build/util.o

# The program. It comes first, so a plain `make` builds it.
build/prog.axf: $(PROG_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(PROG_OBJECTS) -o $@

# Any object, from the source of the same name.
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# (a) An object is built again when a header its source includes
# changes. util.h is included by main.c, util.c and selftest.c; board.h
# by main.c, led.c and selftest.c. The pattern rule still gives the
# recipe, and its first prerequisite, the source, is still $<.
build/main.o build/selftest.o: include/util.h include/board.h
build/util.o: include/util.h
build/led.o: include/board.h

# (b) The second program: selftest.c has its own main, so it replaces
# main.o and the other two objects are shared. It is not the first rule,
# so the default goal is still build/prog.axf.
build/check.axf: build/led.o build/util.o build/selftest.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -rf build
