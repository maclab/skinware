ACLOCAL_AMFLAGS = -I m4

if HAVE_GL
SUBDIRS= gl settings gui

bin_PROGRAMS = skin@SKIN_SUFFIX@_view
skin@SKIN_SUFFIX@_view_SOURCES = \
                                 main.cpp \
                                 vecmath.h
skin@SKIN_SUFFIX@_view_CXXFLAGS = \
                                  $(SKIN_CXXFLAGS_USER) \
                                  -I"$(top_srcdir)/skin/include" \
                                  -I"$(top_srcdir)/apps/calibrator" \
                                  -I"$(top_srcdir)/apps/motion" \
                                  -I"$(top_srcdir)/apps/tools" \
                                  -I"$(srcdir)/gl/shImage" \
                                  -I"$(srcdir)/gl/shFont" \
                                  -I"$(srcdir)/gl/Ngin" \
                                  -I"$(srcdir)/gl/Ngin3d"
skin@SKIN_SUFFIX@_view_LDADD = \
                               ../tools/libskintools.la \
                               ../calibrator/libskin@SKIN_SUFFIX@_calibrator.la \
                               ../../skin/src/libskin@SKIN_SUFFIX@.la \
                               gl/Ngin3d/libNgin3d.la \
                               gl/Ngin/libNgin.la \
                               gl/shFont/libshFont.la \
                               gl/shImage/libshImage.la \
                               $(SKIN_LDFLAGS_GL) \
                               $(SKIN_LDFLAGS_USER)
endif
