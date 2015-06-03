ACLOCAL_AMFLAGS = -I m4

SUBDIRS = calibrator

if HAVE_CXX
  SUBDIRS += tools
endif

if HAVE_CXX11
  SUBDIRS += motion
endif

if HAVE_GL
  SUBDIRS += view
endif
