TOP_PATH := $(shell pwd)

PREFIX := $(TOP_PATH)/depend

.PHONY: all libsio install libsio-install clean libsio-clean

libsio:
	make -C $(TOP_PATH)/lib

all: clean libsio

install: libsio-install

libsio-install:
	make -C $(TOP_PATH)/lib install PREFIX=$(PREFIX)

clean: libsio-clean

libsio-clean:
	make -C $(TOP_PATH)/lib clean
