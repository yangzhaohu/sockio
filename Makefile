TARGET := libsockio.so
TARGET_OUT := target/
$(shell mkdir -p $(TARGET_OUT))

OBJ_OUTPUT := out/
$(shell mkdir -p $(OBJ_OUTPUT))

FLAGS :=
INCLUDES :=
LIBS := 

SRCS := sio_mplex.c \
		sio_select.c \
		sio_epoll.c \
		sio_iocp.c \
		sio_socket.c \
		sio_server.c \
		sio_servers.c \
		sio_thread.c \
		sio_thread_pthread.c \
		sio_thread_win.c \
		sio_mplex_thread.c \
		sio_atomic.c \
		sio_stack.c \
		sio_queue.c \
		sio_mempool.c

.PHONY: incre all clean help

ifdef compiler
ifeq ($(compiler), gcc)
FLAGS += -fPIC -shared -Wl,-z,defs -g -DLINUX
else
FLAGS += /DWIN32
endif
include build/platfrom/make.$(compiler)
endif

help:
	@set -e; echo "compiler=<compiler>"; \
	echo "	 <compiler>:gcc mingw cl";


all: clean incre

clean:
	rm -rf $(OBJ_OUTPUT)/* $(TARGET_OUT)/$(TARGET) $(TARGET_OUT)/$(TARGET).*

