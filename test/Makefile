CC  := gcc
CXX := g++

TARGET_OUT := ../target/
$(shell mkdir -p $(TARGET_OUT))

OBJ_OUTPUT := ../out/
$(shell mkdir -p $(OBJ_OUTPUT))

FLAGS := -fPIE
INCLUDES := -I../ -I../depend/include
LIBS := -L../target -lsockio  -ldl -lpcre2-8 -lpcre2-posix

SRCS := test_http_parser.c \
	pcre2demo.c \
	test_rtspprot.c


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
	$(CXX) $(OBJ_OUTPUT)$@.o $(LIBS) -Wl,-rpath=./ -Wl,-rpath-link=../depend/lib -o $(TARGET_OUT)$@

clean:
	rm -rf $(OBJ_OUTPUT)*.o $(OBJ_OUTPUT)*.d $(OBJ_OUTPUT)*.d.tmp $(TARGET_OUT)/$(TARGET) $(TARGET_OUT)/$(TARGET).*