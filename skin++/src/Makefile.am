ACLOCAL_AMFLAGS = -I m4

if HAVE_CXX
lib_LTLIBRARIES = libskin++@SKIN_SUFFIX@.la
libskin__@SKIN_SUFFIX@_la_SOURCES = \
                                    hooks.cpp \
                                    iterators.cpp \
                                    objects.cpp \
                                    skin.cpp
libskin__@SKIN_SUFFIX@_la_CXXFLAGS = $(SKIN_CXX11FLAGS_USER) -I"$(top_srcdir)/skin/include" -I"$(top_srcdir)/skin++/include"
libskin__@SKIN_SUFFIX@_la_LDFLAGS = -version-info 0:0:0
endif
