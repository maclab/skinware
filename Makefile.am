ACLOCAL_AMFLAGS = -I m4

SUBDIRS = skin

if HAVE_CXX11
  SUBDIRS += skin++
endif

if HAVE_PYTHON
  SUBDIRS += pyskin
endif

if HAVE_DOCTHIS
  SUBDIRS += doc
endif

SUBDIRS += tools apps
