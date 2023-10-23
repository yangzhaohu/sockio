TARGET := libsockio.so
TARGET_OUT := target/
$(shell mkdir -p $(TARGET_OUT))

OBJ_OUTPUT := out/
$(shell mkdir -p $(OBJ_OUTPUT))

FLAGS :=
INCLUDES := -I. -Iinclude -Idepend/include
LIBS := -Ldepend/lib -ldl -lrt -lhttp_parser -lpcre2-8 -lpcre2-posix

SRCS := sio_global.cpp \
		sio_mplex.c \
		sio_select.c \
		sio_epoll.c \
		sio_iocp.c \
		sio_socket.c \
		sio_server.c \
		sio_servflow.c \
		sio_thread.c \
		sio_thread_pthread.c \
		sio_thread_win.c \
		sio_permplex.c \
		sio_atomic.c \
		sio_stack.c \
		sio_queue.c \
		sio_mempool.c \
		sio_fifo.c \
		sio_block.c \
		sio_taskfifo.c \
		sio_elepool.c \
		sio_service.c \
		sio_regex.c \
		sio_timer_posix.c \
		sio_timer.c \
		utils/sio_dlopen.c \
		proto/sio_httpprot.c \
		proto/sio_rtspprot.c \
		moudle/http/sio_httpmod.c \
		moudle/http/sio_httpmod_html.c \
		moudle/locate/sio_locate.c \
		moudle/locate/sio_locatmod.c \
		moudle/rtsp/sio_rtspmod.c \
		moudle/rtsp/sio_rtpool.c \
		moudle/rtsp/sio_rtpvod.c \
		moudle/rtsp/sio_rtplive.c \
		moudle/rtsp/sio_rtpstream.c \
		moudle/rtsp/sio_rtspipe.c \
		moudle/rtsp/rtpack/sio_avdev.c \
		moudle/rtsp/rtpack/sio_avdev_jpeg.c \
		moudle/rtsp/rtpack/sio_rtpack.c \
		moudle/rtsp/rtpack/sio_rtpack_jpeg.c \

.PHONY: incre all clean help

ifdef compiler
ifeq ($(compiler), gcc)
FLAGS += -fPIC -shared -DLINUX -g -Wl,-z,defs -Wl,-rpath=.
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

