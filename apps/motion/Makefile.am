ACLOCAL_AMFLAGS = -I m4

if HAVE_CXX11
noinst_PROGRAMS = motion_detect
motion_detect_SOURCES = \
                        main.cpp \
                        skin_motion.h
motion_detect_CXXFLAGS = \
                       $(SKIN_CXX11FLAGS_USER) \
                       -I"$(top_srcdir)/skin/include" \
                       -I"$(top_srcdir)/skin++/include" \
                       -I"$(top_srcdir)/apps/calibrator" \
                       -I"$(top_srcdir)/apps/tools"
motion_detect_LDADD = \
                      ../tools/libskintools.la \
                      ../calibrator/libskin@SKIN_SUFFIX@_calibrator.la \
                      ../../skin++/src/libskin++@SKIN_SUFFIX@.la \
                      ../../skin/src/libskin@SKIN_SUFFIX@.la \
                      $(SKIN_LDFLAGS_USER)
endif
