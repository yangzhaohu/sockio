CC  := cl
CXX := cl
AR := lib
LINK := link

# runtime library
FLAGS += /DWIN32 /MD /Fd

CFLAGS += $(FLAGS) /TC
CXXFLAGS += $(FLAGS) /TP

OBJS := $(SRCS:%.c=$(TMPLIB_DIR)%.obj)
OBJS := $(OBJS:%.cpp=$(TMPLIB_DIR)%.obj)

SYMBOlS := symbols.def

$(TMPLIB_DIR)%.obj : %.c
	@set -e; mkdir -p $(dir $@);
	$(CC) /c $< $(INCLUDES) /nologo $(CFLAGS) /Fo: $@

$(TMPLIB_DIR)%.obj : %.cpp
	@set -e; mkdir -p $(dir $@);
	$(CXX) /c $< $(INCLUDES) /nologo $(CXXFLAGS) /Fo: $@

build: linklib linkdll

linklib: $(OBJS)
	$(AR) $(OBJS) /nologo /out:$(TARGET).a

linkdll: $(OBJS)
	$(LINK) $(OBJS) $(LIBS) /nologo /DEF:$(SYMBOlS) $(LIBPATH) $(LFLAGS) /out:$(TARGET).dll

clean:
	@set -e; rm -rf $(OBJS) $(TARGET).a $(TARGET).dll $(TARGET).lib
