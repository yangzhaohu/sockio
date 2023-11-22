
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

all: http_parser-build pcre2-build openssl-build

HTTP_PARSER := http_parser
http_parser-build:
	-make -C $(HTTP_PARSER) clean
	make -C $(HTTP_PARSER) libpackage -j 4
	make -C $(HTTP_PARSER) library -j 4
	make -C $(HTTP_PARSER) PREFIX=$(INSTALL_DIR)/$(HTTP_PARSER) install
	$(shell mkdir -p $(DEPEND_INC_DIR)/$(HTTP_PARSER))
	cp $(INSTALL_DIR)/$(HTTP_PARSER)/include/*  $(DEPEND_INC_DIR)/$(HTTP_PARSER)
	cp $(INSTALL_DIR)/$(HTTP_PARSER)/lib/*.so*  $(DEPEND_LIB_DIR)
	cp $(HTTP_PARSER)/*.a  $(DEPEND_LIB_DIR)

PCRE2 := pcre2
pcre2-build:
	-make -C $(PCRE2) clean
	cd $(PCRE2) && ./configure --host=$(CROSS_COMPILER) CC=$(CC) CXX=$(CXX) CFLAGS='-fPIC -O3' --enable-static --prefix=$(INSTALL_DIR)/$(PCRE2)
	make -C $(PCRE2) -j 4
	make -C $(PCRE2) install
	$(shell mkdir -p $(DEPEND_INC_DIR)/$(PCRE2))
	cp $(INSTALL_DIR)/$(PCRE2)/include/*  $(DEPEND_INC_DIR)/$(PCRE2)
	cp $(INSTALL_DIR)/$(PCRE2)/lib*/*.so*  $(DEPEND_LIB_DIR)
	cp $(INSTALL_DIR)/$(PCRE2)/lib*/*.a  $(DEPEND_LIB_DIR)

OPENSSL := openssl
openssl-build:
	-make -C $(OPENSSL) clean
	cd $(OPENSSL) && ./Configure shared no-asm --cross-compile-prefix= --prefix=$(INSTALL_DIR)/$(OPENSSL)
	make -C $(OPENSSL) -j 4
	make -C $(OPENSSL) install
	$(shell mkdir -p $(DEPEND_INC_DIR)/$(OPENSSL))
	cp $(INSTALL_DIR)/$(OPENSSL)/include/openssl/*  $(DEPEND_INC_DIR)/$(OPENSSL)
	cp $(INSTALL_DIR)/$(OPENSSL)/lib/*.so*  $(DEPEND_LIB_DIR)

download: http_parser-download pcre2-download openssl-download

http_parser-download:
	$(info "http_parser download")
	$(info "from download https://github.com/nodejs/http-parser/archive/refs/tags/v2.9.4.1.tar.gz")
	rm -rf $(HTTP_PARSER)
	wget https://github.com/yangzhaohu/http-parser/archive/refs/tags/v2.9.4.2.tar.gz --no-check-certificate -O http-parser-2.9.4.2.tar.gz
	tar -zxvf http-parser-2.9.4.2.tar.gz >/dev/null
	cp -r http-parser-2.9.4.2 $(HTTP_PARSER)
	rm -rf http-parser-2.9.4.2

pcre2-download:
	$(info "pcre2 download")
	$(info "form download https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.42/pcre2-10.42.tar.gz")
	rm -rf $(PCRE2)
	wget https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.42/pcre2-10.42.tar.gz --no-check-certificate -O pcre2-10.42.tar.gz
	tar -zxvf pcre2-10.42.tar.gz >/dev/null
	cp -r pcre2-10.42 $(PCRE2)
	rm -rf pcre2-10.42

openssl-download:
	$(info "openssl download")
	$(info "form download https://github.com/openssl/openssl/releases/download/openssl-3.1.4/openssl-3.1.4.tar.gz")
	rm -rf $(OPENSSL)
	wget https://github.com/openssl/openssl/releases/download/openssl-3.1.4/openssl-3.1.4.tar.gz --no-check-certificate -O openssl-3.1.4.tar.gz
	tar -zxvf openssl-3.1.4.tar.gz >/dev/null
	cp -r openssl-3.1.4 $(OPENSSL)
	rm -rf openssl-3.1.4