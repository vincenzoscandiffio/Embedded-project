# Embedded Operating Systems and Architecture course
# Licensed under the MIT License. See LICENSE in the repository root.
#
# Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>

# A convenience to try a practice question's code. It is not part of any
# question and nothing here checks an answer.
#
#   make run Q=q02         questions with an answer.c (Q1 to Q5): builds
#                          your answer.c with play/q02.c and runs it in
#                          QEMU; stop it with Ctrl-C
#   make run Q=q07         questions on the kernel (Q7, Q8): builds the
#                          kernel of week 5 with the two files of the
#                          question's kernel/ folder laid over it, and
#                          runs it in QEMU, as in week 5
#   make clean             removes build/
#
# Q6 is not run from here: its answer is the Makefile of its own folder.

# ARM bare-metal cross-compiler: runs on the host, emits Cortex-M code.
CC := arm-none-eabi-gcc

# The folder of the question: ../q02-ring-buffer for Q=q02.
QDIR := $(firstword $(wildcard ../$(Q)-*))

.PHONY: all run clean

ifeq ($(Q),)
all run:
	@echo "usage: make run Q=q02 (one of q01 to q05, q07, q08)"; false
else ifeq ($(QDIR),)
all run:
	@echo "no question folder ../$(Q)-*"; false
else ifneq ($(wildcard $(QDIR)/answer.c),)
# --- A question with an answer.c: your file, a play/ main, the runtime. ---

# Same flags as the kernel's Makefile (weeks 1 to 5): Thumb code for the
# Cortex-M3, no C library, no start files (runtime/startup.c is ours),
# warnings on, no optimisation and debug information so that gdb follows
# the C. play/qNN.c includes your answer.c, found through -I.
CFLAGS := -mthumb -mcpu=cortex-m3 -nostartfiles -ffreestanding -fno-builtin
CFLAGS += -std=gnu17 -Wall -Wextra -O0 -g3 -Iruntime -I$(QDIR)
LDFLAGS := -T runtime/common.ld -nostdlib -lgcc

SOURCES := play/$(Q).c runtime/startup.c runtime/console.c runtime/format.c
BIN := build/$(Q).axf

all: $(BIN)

$(BIN): $(SOURCES) $(QDIR)/answer.c $(wildcard runtime/*.h) runtime/common.ld
	@mkdir -p build
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $(BIN)

# The program never ends by itself: quit with Ctrl-C. The same QEMU line
# as `make run` in the kernel: UART0 on this terminal.
run: $(BIN)
	qemu-system-arm -machine mps2-an385 -monitor null -nographic -serial stdio -kernel $(BIN)
else ifeq ($(wildcard $(QDIR)/kernel/task.c),)
all run:
	@echo "$(QDIR) is not run from here: run make in that folder"; false
else
# --- A kernel question: week 5's kernel with the question's files. ---

# The kernel you start from, and the folder where it is built.
BASE := ../../../week-05/solution
WORK := build/$(Q)

FILES := $(QDIR)/kernel/task.c $(QDIR)/kernel/task.h

# Copy the kernel, then the question's two files over its own, and build
# there. The copy is made again whenever one of the files changes.
all: $(WORK)/.synced
	$(MAKE) -C $(WORK)

$(WORK)/.synced: $(FILES)
	mkdir -p $(WORK)
	cd $(BASE) && tar cf - --exclude=build . | tar xf - -C $(CURDIR)/$(WORK)
	cp $(FILES) $(WORK)/kernel/
	touch $@

# `make run Q=q07 DEMO=idle` selects a demonstration of week 5, as there.
run: all
	$(MAKE) -C $(WORK) run
endif

clean:
	rm -rf build
