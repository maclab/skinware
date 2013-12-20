from distutils.core import setup, Extension
import sys
try:
	rtai_include = sys.argv[1]
	ver=sys.argv[2]
	del sys.argv[2]
	del sys.argv[1]
	ext = Extension("example_pyservice",
			include_dirs = ['../..', '../../../kernel-module', rtai_include],
			library_dirs = ['../..'],
			libraries = ['stdc++', 'skin-' + ver, 'rt'],
			sources = ["example_pyservice.c"])
	setup(name='example_pyservice',
		version=ver,
		description='Python Skinware service example',
		ext_modules=[ext])
except IndexError:
	print "Usage: python setup.py rtai_include version command [options]"

# Note: see here: http://stackoverflow.com/q/4259170/912144
