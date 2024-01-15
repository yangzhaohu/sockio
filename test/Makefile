CC  := gcc
CXX := g++

PREFIX := ../target/

TMP_DIR := ../out/

ifeq ($(compiler), gcc)
FLAGS := -fPIE -g
INCLUDES := -I../ -I../depend/include -I../depend/include/sio
LIBS := -L../depend/lib -lsio -ldl
else
INCLUDES := -I../ -I../depend/include -I../depend/include/sio
LIBS := libsio.lib
endif

CFLAGS += $(FLAGS)
CXXFLAGS += $(FLAGS)

SRCS := test_mplex_close.c \
	test_socket_select.c \
	test_socket_iocp.c \
	test_udp.c \
	test_connect_mplex.c \
	test_udp2.c \
	test_ssl_client2.c \
	test_ssl_server.c \
	test_ssl_server2.c \
	test_ssl_server3.c \
	test_server.c

ifeq ($(compiler), gcc)
OBJS := $(SRCS:%.c=$(OBJ_OUTPUT)%.o)
OBJS := $(OBJS:%.cpp=$(OBJ_OUTPUT)%.o)

$(OBJ_OUTPUT)%.o : %.c
	$(CC) -c $< $(CFLAGS) $(INCLUDES) -o $@

$(OBJ_OUTPUT)%.o : %.cpp
	$(CXX) -c $< $(CXXFLAGS) $(INCLUDES) -o $@

endif

TARGET_LIST := $(patsubst %.c, %, $(notdir $(SRCS)))
TARGET_LIST := $(patsubst %.cpp, %, $(TARGET_LIST))

ifeq ($(compiler), gcc)
all: $(OBJS) $(TARGET_LIST)
$(TARGET_LIST):
	$(CXX) $(OBJ_OUTPUT)$@.o $(LIBS) -Wl,-rpath=./ -Wl,-rpath-link=../depend/lib -o $(PREFIX)$@

else
winall: $(TARGET_LIST)
$(TARGET_LIST):
	cl $@.c $(INCLUDES) /MD libsio.lib /link /LIBPATH:../depend/lib /out:$(PREFIX)/$@.exe
endif

clean:
	rm -f $(OBJS)
	cd $(PREFIX) && rm -f $(TARGET_LIST)