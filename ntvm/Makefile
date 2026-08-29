CC ?= x86_64-w64-mingw32-gcc
ROOT := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
CFLAGS := -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -Wall -Wextra -I$(ROOT)/../freedll/include
all: $(ROOT)/build/ntvm.obj
$(ROOT)/build/ntvm.obj: $(ROOT)/ntvm.c $(ROOT)/ntvm.h | $(ROOT)/build
	$(CC) $(CFLAGS) -c $(ROOT)/ntvm.c -o $@
$(ROOT)/build:
	mkdir -p $@
clean:
	rm -rf $(ROOT)/build
.PHONY: all clean
