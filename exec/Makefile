TARGET := sio
TARGET_OUT := ../target/
$(shell mkdir -p $(TARGET_OUT))

OBJ_OUTPUT := ../out/
$(shell mkdir -p $(OBJ_OUTPUT))

FLAGS :=
INCLUDES := -I3dparty -I.
LIBS := 

SRCS := sio.c

.PHONY: incre all clean help

ifdef compiler
ifeq ($(compiler), gcc)
FLAGS += -fPIE -g -DLINUX
else
FLAGS += /DWIN32
endif
include ../build/platfrom/make.$(compiler)
endif

INCLUDES := -I../
LIBS := -L../target -lsockio

help:
	@set -e; echo "compiler=<compiler>"; \
	echo "	 <compiler>:gcc mingw cl";


all: clean incre

clean:
	rm -rf $(OBJ_OUTPUT)/* $(TARGET_OUT)/$(TARGET) $(TARGET_OUT)/$(TARGET).*

