ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = Kbuild

if HAVE_USER
lib_LTLIBRARIES = libskin@SKIN_SUFFIX@.la
libskin@SKIN_SUFFIX@_la_SOURCES = \
                                  base.c \
                                  driver.c \
                                  hooks.c \
                                  internal.c \
                                  iterators.c \
                                  load.c \
                                  module.c \
                                  names.c \
                                  patch.c \
                                  reader.c \
                                  sensor.c \
                                  sensor_types.c \
                                  service_provider.c \
                                  service_user.c \
                                  tools.c \
                                  user.c \
                                  writer.c \
                                  inline.c \
                                  data_internal.h \
                                  driver_internal.h \
                                  internal.h \
                                  names.h \
                                  reader_internal.h \
                                  user_internal.h \
                                  writer_internal.h
libskin@SKIN_SUFFIX@_la_CFLAGS = $(SKIN_CFLAGS_USER) -I"$(top_srcdir)/skin/include"
libskin@SKIN_SUFFIX@_la_LDFLAGS = -version-info 0:0:0
endif

if HAVE_KERNEL
KBUILD_EXTRA_CFLAGS = \
                      -I"$(abs_top_srcdir)/skin/src" \
                      -I"$(abs_top_builddir)/skin/src" \
                      $(KBUILD_CFLAGS)
KBUILD_EXTRA_SYMBOLS = $(KBUILD_SYMBOLS)

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
	cp Module.symvers $(DESTDIR)$(SKIN_KERNEL_MODULE_PATH)
endif
