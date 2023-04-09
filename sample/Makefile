CC  := gcc
CXX := g++

TARGET_OUT := ../target/
$(shell mkdir -p $(TARGET_OUT))

OBJ_OUTPUT := ../out/
$(shell mkdir -p $(OBJ_OUTPUT))

FLAGS := -fPIE
INCLUDES := -I../
LIBS := -L../target -lsockio

SRCS := sample_server.c \
		sample_servflow.c \
		sample_socket_select.c \
		sample_socket_destory.c \
		sample_epoll_wait_wakeup.c \
		sample_socket_sendbuff.c

OBJS := $(SRCS:%.c=$(OBJ_OUTPUT)%.o)
OBJS := $(OBJS:%.cpp=$(OBJ_OUTPUT)%.o)

$(OBJ_OUTPUT)%.o : %.c
	$(CC) -c $< $(CFLAGS) $(INCLUDES) -o $@

$(OBJ_OUTPUT)%.o : %.cpp
	$(CXX) -c $< $(CXXFLAGS) $(INCLUDES) -o $@

TARGET_LIST := $(patsubst %.c, %, $(notdir $(SRCS)))
TARGET_LIST := $(patsubst %.cpp, %, $(TARGET_LIST))

all: $(OBJS) $(TARGET_LIST)
$(TARGET_LIST):
	$(CXX) $(OBJ_OUTPUT)$@.o $(LIBS) -Wl,-rpath=./ -o $(TARGET_OUT)$@

clean:
	rm -rf $(OBJ_OUTPUT)*.o $(OBJ_OUTPUT)*.d $(OBJ_OUTPUT)*.d.tmp $(TARGET_OUT)/$(TARGET) $(TARGET_OUT)/$(TARGET).*