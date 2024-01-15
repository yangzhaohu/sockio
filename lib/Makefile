TARGET := libsio

ifndef TOP_PATH
TOP_PATH := ..
endif

ifndef PREFIX
$(info $(PREFIX))
PREFIX := $(TOP_PATH)/target/
$(shell mkdir -p $(PREFIX))
endif

INSTALL_INCDIR = $(PREFIX)/include/sio
$(shell mkdir -p $(INSTALL_INCDIR))

INSTALL_LIBDIR = $(PREFIX)/lib
$(shell mkdir -p $(INSTALL_LIBDIR))

ifndef TMP_DIR
TMP_DIR := $(TOP_PATH)/tmp
endif

TMPLIB_DIR := $(TOP_PATH)/tmp/lib/
$(shell mkdir -p $(TMP_DIR))

FLAGS :=
LFLAGS :=
INCLUDES :=
LIBPATH :=
STATICLIB :=
DYNAMICLIB :=

ifdef compiler
ifeq ($(compiler), gcc)
FLAGS +=
INCLUDES += -I. -I$(TOP_PATH)/include -I$(TOP_PATH)/depend/include
LIBPATH += -L$(TOP_PATH)/depend/lib
STATICLIB += -Wl,-Bstatic -lhttp_parser -lpcre2-posix -lpcre2-8
DYNAMICLIB += -Wl,-Bdynamic -ldl -lrt -luring -lpthread -lssl -lcrypto
else
FLAGS :=
INCLUDES += /I. /I$(TOP_PATH)/include /I$(TOP_PATH)/depend/include
LIBPATH += /LIBPATH:$(TOP_PATH)/depend/lib
STATICLIB += libhttp_parser.lib pcre2-posix-static.lib pcre2-8-static.lib
DYNAMICLIB += mswsock.lib ws2_32.lib libssl.lib libcrypto.lib
endif
endif

LIBS := $(STATICLIB) $(DYNAMICLIB)

SRCS := sio_global.cpp \
		sio_log.c \
		sio_time.c \
		sio_mplex.c \
		sio_select.c \
		sio_epoll.c \
		sio_iocp.c \
		sio_uring.c \
		sio_socket.c \
		sio_sslctx.c \
		sio_sockssl.c \
		sio_aio.c \
		sio_server.c \
		sio_thread.c \
		sio_thread_pthread.c \
		sio_thread_win.c \
		sio_pmplex.c \
		sio_atomic.c \
		sio_stack.c \
		sio_queue.c \
		sio_mempool.c \
		sio_regex.c \
		sio_timer_posix.c \
		sio_timer_win.c \
		sio_timer.c \
		sio_dlopen.c

.PHONY: incre all clean help

ifdef compiler
ifeq ($(compiler), gcc)
FLAGS += -fPIC -Wl,-z,defs -shared -g -Wall -Werror -Wno-multichar
LFLAGS += -fPIC -Wl,-z,defs -shared -Wl,-rpath=.
else
FLAGS +=
LFLAGS += /dll
endif
include ../build/platfrom/make.$(compiler)
endif

help:
	@set -e; echo "compiler=<compiler>"; \
	echo "	 <compiler>:gcc mingw cl";


all: clean build

install:
	install -D -m 644 sio_def.h $(INSTALL_INCDIR)/sio_def.h
	install -D -m 644 sio_common.h $(INSTALL_INCDIR)/sio_common.h
	install -D -m 644 sio_event.h $(INSTALL_INCDIR)/sio_event.h
	install -D -m 644 sio_socket.h $(INSTALL_INCDIR)/sio_socket.h
	install -D -m 644 sio_server.h $(INSTALL_INCDIR)/sio_server.h
	install -D -m 644 sio_mplex.h $(INSTALL_INCDIR)/sio_mplex.h
	install -D -m 644 sio_pmplex.h $(INSTALL_INCDIR)/sio_pmplex.h
	install -D -m 644 sio_global.h $(INSTALL_INCDIR)/sio_global.h
	install -D -m 644 sio_thread.h $(INSTALL_INCDIR)/sio_thread.h
	install -D -m 644 sio_atomic.h $(INSTALL_INCDIR)/sio_atomic.h
	install -D -m 644 sio_stack.h $(INSTALL_INCDIR)/sio_stack.h
	install -D -m 644 sio_queue.h $(INSTALL_INCDIR)/sio_queue.h
	install -D -m 644 sio_mempool.h $(INSTALL_INCDIR)/sio_mempool.h
	install -D -m 644 sio_regex.h $(INSTALL_INCDIR)/sio_regex.h
	install -D -m 644 sio_time.h $(INSTALL_INCDIR)/sio_time.h
	install -D -m 644 sio_timer.h $(INSTALL_INCDIR)/sio_timer.h
	install -D -m 644 sio_dlopen.h $(INSTALL_INCDIR)/sio_dlopen.h
	install -D -m 644 sio_log.h $(INSTALL_INCDIR)/sio_log.h
	install -D -m 644 $(TARGET).* $(INSTALL_LIBDIR)/
