TOP_PATH := $(shell pwd)

.PHONY: all libsio libsio-clean clean

all: libsio

libsio:
	make -C $(TOP_PATH)/lib

libsio-clean:
	make -C $(TOP_PATH)/lib clean

clean: libsio-clean

