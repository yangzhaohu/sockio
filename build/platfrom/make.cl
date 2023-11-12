CC  := cl
CXX := cl
AR := lib
LINK := link

# runtime library
FLAGS += /DWIN32 /MD /Fd

CFLAGS += $(FLAGS) /TC
CXXFLAGS += $(FLAGS) /TP

OBJS := $(SRCS:%.c=$(OBJ_OUTPUT)%.obj)
OBJS := $(OBJS:%.cpp=$(OBJ_OUTPUT)%.obj)

SYMBOlS := symbols.def

$(OBJ_OUTPUT)%.obj : %.c
	@set -e; mkdir -p $(dir $@);
	$(CC) /c $< $(INCLUDES) /nologo $(CFLAGS) /Fo: $@

$(OBJ_OUTPUT)%.obj : %.cpp
	@set -e; mkdir -p $(dir $@);
	$(CXX) /c $< $(INCLUDES) /nologo $(CXXFLAGS) /Fo: $@

build: linklib linkdll

linklib: $(OBJS)
	$(AR) $(OBJS) /nologo /out:$(TARGET_OUT)/$(TARGET)_static.lib

linkdll: $(OBJS)
	$(LINK) $(OBJS) $(LIBS) /nologo /DEF:$(SYMBOlS) $(LIBPATH) $(LFLAGS) /out:$(TARGET_OUT)/$(TARGET).dll

clean:
	@set -e; rm -rf $(OBJS) $(TARGET_OUT)/$(TARGET)_static.lib $(TARGET_OUT)/$(TARGET).dll $(TARGET_OUT)/$(TARGET).lib
