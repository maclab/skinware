ACLOCAL_AMFLAGS = -I m4

include_HEADERS = skin_calibrator.h

if HAVE_USER
bin_PROGRAMS = skin@SKIN_SUFFIX@_calibrate
skin@SKIN_SUFFIX@_calibrate_SOURCES = \
                                      main.c \
                                      calib_internal.h \
                                      skin_calibrator.h
skin@SKIN_SUFFIX@_calibrate_CFLAGS = $(SKIN_CFLAGS_USER) -I"$(top_srcdir)/skin/include"
skin@SKIN_SUFFIX@_calibrate_LDADD = ../../skin/src/libskin@SKIN_SUFFIX@.la $(SKIN_LDFLAGS_USER)

lib_LTLIBRARIES = libskin@SKIN_SUFFIX@_calibrator.la
libskin@SKIN_SUFFIX@_calibrator_la_SOURCES = \
                                             calib.c \
                                             calib_internal.h \
                                             skin_calibrator.h
libskin@SKIN_SUFFIX@_calibrator_la_CFLAGS = $(SKIN_CFLAGS_USER) -I"$(top_srcdir)/skin/include"
libskin@SKIN_SUFFIX@_calibrator_la_LDFLAGS = -version-info 0:0:0
endif

if HAVE_KERNEL
KBUILD_EXTRA_CFLAGS = -I"$(abs_top_builddir)/skin/src"
KBUILD_EXTRA_SYMBOLS = \
                       "$(abs_top_builddir)/skin/src/Module.symvers"

if HAVE_KIO
KBUILD_EXTRA_CFLAGS += -I"$(SKIN_KERNEL_PATH)/kio"
KBUILD_EXTRA_SYMBOLS += \
                        "$(SKIN_KERNEL_PATH)/kio/Module.symvers"
endif

KBUILD_EXTRA_CFLAGS += $(KBUILD_CFLAGS)
KBUILD_EXTRA_SYMBOLS += $(KBUILD_SYMBOLS)

V_KERN_INVOKE = $(V_KERN_INVOKE_@AM_V@)
V_KERN_INVOKE_ = $(V_KERN_INVOKE_@AM_DEFAULT_V@)
V_KERN_INVOKE_0 = @
V_KERN_INVOKE_1 =

all-local:
	$(V_KERN_INVOKE)$(SKIN_KERNEL_INVOKE) modules
clean-local:
	$(V_KERN_INVOKE)$(SKIN_KERNEL_INVOKE) clean
	rm -f "$(srcdir)"/*.o "$(srcdir)"/.*.cmd
install-exec-local:
	$(V_KERN_INVOKE)$(SKIN_KERNEL_INVOKE) modules_install
	cat Module.symvers | grep --color=none calibrator >> $(DESTDIR)$(SKIN_KERNEL_MODULE_PATH)/Module.symvers
endif
