CC  := cl
CXX := cl
AR := lib

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
	$(CXX) $(OBJS) $(LIBS) /nologo /DEF: $(SYMBOlS) /link $(LIBPATH) /dll /out:$(TARGET_OUT)/$(TARGET).dll
