
TARGET_OUT := D:

CFLAGS += $(FLAGS)
CXXFLAGS += $(FLAGS)

OBJS := $(SRCS:%.c=$(OBJ_OUTPUT)%.obj)
OBJS := $(OBJS:%.cpp=$(OBJ_OUTPUT)%.obj)

$(OBJ_OUTPUT)%.obj : %.c
	cl /c $< /nologo $(CFLAGS) /Fo: $@

$(OBJ_OUTPUT)/%.obj : %.cpp
	cl /c $< /nologo $(CXXFLAGS) /Fo: $@

incre: $(OBJS)
	cl $(OBJS) mswsock.lib ws2_32.lib /nologo /Fe: $(TARGET_OUT)/$(TARGET)