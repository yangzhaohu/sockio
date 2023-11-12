TARGET := libsockio
TARGET_OUT := target/
$(shell mkdir -p $(TARGET_OUT))

OBJ_OUTPUT := out/
$(shell mkdir -p $(OBJ_OUTPUT))

FLAGS :=
INCLUDES :=
LIBPATH :=
STATICLIB :=
DYNAMICLIB :=

ifdef compiler
ifeq ($(compiler), gcc)
FLAGS +=
INCLUDES += -I. -Iinclude -Idepend/include
LIBPATH += -Ldepend/lib
STATICLIB += -Wl,-Bstatic -lhttp_parser -lpcre2-posix -lpcre2-8
DYNAMICLIB += -Wl,-Bdynamic -ldl -lrt -lpthread
else
FLAGS :=
INCLUDES += /I. /Iinclude /Idepend/include
LIBPATH += /LIBPATH:"depend/lib"
STATICLIB += libhttp_parser.lib pcre2-posix-static.lib pcre2-8-static.lib
DYNAMICLIB += mswsock.lib ws2_32.lib
endif
endif

LIBS := $(STATICLIB) $(DYNAMICLIB)

SRCS := sio_global.cpp \
		sio_mplex.c \
		sio_select.c \
		sio_epoll.c \
		sio_iocp.c \
		sio_socket.c \
		sio_server.c \
		sio_thread.c \
		sio_thread_pthread.c \
		sio_thread_win.c \
		sio_permplex.c \
		sio_atomic.c \
		sio_stack.c \
		sio_queue.c \
		sio_mempool.c \
		sio_regex.c \
		sio_timer_posix.c \
		sio_timer_win.c \
		sio_timer.c \
		utils/sio_dlopen.c

.PHONY: incre all clean help

ifdef compiler
ifeq ($(compiler), gcc)
FLAGS += -fPIC -shared -g -Wall -Werror -Wno-multichar -Wl,-z,defs -Wl,-rpath=.
else
FLAGS +=
endif
include build/platfrom/make.$(compiler)
endif

help:
	@set -e; echo "compiler=<compiler>"; \
	echo "	 <compiler>:gcc mingw cl";


all: clean build

clean:
	rm -rf $(OBJ_OUTPUT)/* $(TARGET_OUT)/$(TARGET) $(TARGET_OUT)/$(TARGET).*

