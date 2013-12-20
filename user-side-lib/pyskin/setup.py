from distutils.core import setup, Extension
import sys
try:
	rtai_include = sys.argv[1]
	ver=sys.argv[2]
	del sys.argv[2]
	del sys.argv[1]
	ext = Extension("pyskin",
			include_dirs = ['..', '../../kernel-module', rtai_include],
			library_dirs = ['..'],
			libraries = ['pthread', 'skin-' + ver, 'rt'],
			sources = ["pyskin.cpp"])
	setup(name='pyskin',
		version=ver,
		description='Python wrapper for Skinware',
		ext_modules=[ext])
except IndexError:
	print "Usage: python setup.py rtai_include version command [options]"

# Note: see here: http://stackoverflow.com/q/4259170/912144
