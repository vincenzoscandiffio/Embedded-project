# Embedded Operating Systems and Architecture course
# Licensed under the MIT License. See LICENSE in the repository root.
#
# Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>

# Q6. This Makefile is your answer: change it as STATEMENT.md asks.

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

.PHONY: clean
clean:
	rm -rf build
