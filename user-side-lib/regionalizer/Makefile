include ../../Makefile.config
include ../../Makefile.common

TARGET := regionalize
KERNEL_HOME := ../../kernel-module
USER_HOME := ..
TOOLS_HOME := $(USER_HOME)/tools
AUX_HOME := $(USER_HOME)/aux
NGIN_HOME := $(AUX_HOME)/Ngin
NGIN3D_HOME := $(AUX_HOME)/Ngin3d
SHIMAGE_HOME := $(AUX_HOME)/shImage
SHFONT_HOME := $(AUX_HOME)/shFont
CXXFLAGS += -I$(KERNEL_HOME) -I$(USER_HOME) -I$(NGIN_HOME) -I$(NGIN3D_HOME) -I$(SHIMAGE_HOME) -I$(SHFONT_HOME) -I$(TOOLS_HOME) $(USER_EXTRA_CXXFLAGS) $(EXTRA_CXXFLAGS)
NGIN_LINK := -L$(NGIN_HOME) -lNgin -lSDLmain -lSDL -lGLU
NGIN3D_LINK := -L$(NGIN3D_HOME) -lNgin3d
SHIMAGE_LINK := -L$(SHIMAGE_HOME) -lshImage
SHFONT_LINK := -L$(SHFONT_HOME) -lshFont
SKIN_LINK := -L$(USER_HOME) -lskin-$(USER_VERSION) -lrt
LDFLAGS += $(NGIN_LINK) $(NGIN3D_LINK) $(SHIMAGE_LINK) $(SHFONT_LINK) $(SKIN_LINK) $(USER_EXTRA_LDFLAGS) $(EXTRA_LDFLAGS)

VPATH = $(TOOLS_HOME)

.PHONY: all
all:
ifneq ($(CONFIG_SKIN_ROSSKIN_LIB_ONLY), y)
	@$(MAKE) --no-print-directory -C $(TOOLS_HOME)
	@$(MAKE) --no-print-directory $(TARGET)
endif
$(TARGET): main.o triangulate.o $(USER_HOME)/libskin.a
	$(LD) -o $@ $(filter-out %.a,$^) $(LDFLAGS)
%.o: %.cpp $(wildcard $(USER_HOME)/*.h) $(wildcard $(KERNEL_HOME)/*.h)
	$(CXX) $(CXXFLAGS) -c $< -o $@
.PHONY: clean
clean:
	-$(RM) *.o $(TARGET)
