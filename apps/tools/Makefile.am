ACLOCAL_AMFLAGS = -I m4

if HAVE_CXX
noinst_LTLIBRARIES = libskintools.la
libskintools_la_SOURCES = \
                          amplifier.cpp \
                          amplifier.h \
                          filter.cpp \
                          filter.h \
                          scaler.cpp \
                          scaler.h
libskintools_la_CXXFLAGS = $(SKIN_CXXFLAGS_USER) -I"$(top_srcdir)/skin/include"
endif
