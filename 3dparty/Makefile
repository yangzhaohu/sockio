
CC  :=gcc
CXX :=g++

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(patsubst %/,%,$(dir $(MKFILE_PATH)))

INSTALL_DIR := $(MKFILE_DIR)/tmp
$(shell mkdir -p $(INSTALL_DIR))

DEPEND_DIR := $(MKFILE_DIR)/../depend
DEPEND_INC_DIR := $(DEPEND_DIR)/include
DEPEND_LIB_DIR := $(DEPEND_DIR)/lib

$(shell mkdir -p $(DEPEND_DIR))
$(shell mkdir -p $(DEPEND_INC_DIR))
$(shell mkdir -p $(DEPEND_LIB_DIR))

ifdef CROSS_COMPILER
ifneq ($(CROSS_COMPILER), )
$(CC)  := $(CROSS_COMPILER)-$(CC)
$(CXX) := $(CROSS_COMPILER)-$(CXX)
endif
endif

all: http_parser-build pcre2-build

HTTP_PARSER := http_parser
http_parser-build:
	-make -C $(HTTP_PARSER) clean
	make -C $(HTTP_PARSER) library -j 4
	make -C $(HTTP_PARSER) PREFIX=$(INSTALL_DIR)/$(HTTP_PARSER) install
	$(shell mkdir -p $(DEPEND_INC_DIR)/$(HTTP_PARSER))
	cp $(INSTALL_DIR)/$(HTTP_PARSER)/include/*  $(DEPEND_INC_DIR)/$(HTTP_PARSER)
	cp $(INSTALL_DIR)/$(HTTP_PARSER)/lib/*.so*  $(DEPEND_LIB_DIR)

PCRE2 := pcre2
pcre2-build:
	-make -C $(PCRE2) clean
	cd $(PCRE2) && ./configure --host=$(CROSS_COMPILER) CC=$(CC) CXX=$(CXX) --prefix=$(INSTALL_DIR)/$(PCRE2)
	make -C $(PCRE2) -j 4
	make -C $(PCRE2) install
	$(shell mkdir -p $(DEPEND_INC_DIR)/$(PCRE2))
	cp $(INSTALL_DIR)/$(PCRE2)/include/*  $(DEPEND_INC_DIR)/$(PCRE2)
	cp $(INSTALL_DIR)/$(PCRE2)/lib/*.so*  $(DEPEND_LIB_DIR)