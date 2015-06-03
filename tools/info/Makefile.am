ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = Kbuild

if HAVE_USER
bin_PROGRAMS = skin@SKIN_SUFFIX@_info
skin@SKIN_SUFFIX@_info_SOURCES = main.c
skin@SKIN_SUFFIX@_info_CFLAGS = $(SKIN_CFLAGS_USER) -I"$(top_srcdir)/skin/include"
skin@SKIN_SUFFIX@_info_LDADD = ../../skin/src/libskin@SKIN_SUFFIX@.la $(SKIN_LDFLAGS_USER)
endif

if HAVE_KERNEL
KBUILD_EXTRA_CFLAGS = $(KBUILD_CFLAGS)
KBUILD_EXTRA_SYMBOLS = \
                       "$(abs_top_builddir)/skin/src/Module.symvers"
                       $(KBUILD_SYMBOLS)

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
endif
