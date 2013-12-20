# Targets:
# all: makes the skin kernel, its drivers and services, skin library, its calibrator, regionalizer,
# and visualizer and Skinware's general and developer documentations
# kernel: makes only the skin kernel
# drivers: makes the skin kernel and its drivers
# services: makes the skin kernel and its services
# library: makes the skin library, its calibrator and regionalizer
# doc: makes Skinware's general documentation as well as developer documentation
# install: installs the software
# uninstall: uninstalls the software
#
# Note: all of these targets only build the parts that have been configured for build

NO_CONFIG_ERROR := $(subst \n ,\n,"You need to configure Skinware first.\nOptions are:\n\
					\tmake oldconfig\n\
					\tmake config\n\
					\tmake menuconfig      (needs ncurses)\n\
					\tmake gconfig         (needs GTK+ 2.0)\n\
					\tmake xconfig         (needs QT)\n\n")

.PHONY: all
all:
	@test -r Makefile.config || (printf $(NO_CONFIG_ERROR) && exit 1)
	@$(MAKE) --no-print-directory regenerate_on_reconfig
	@$(MAKE) --no-print-directory kernel-full library-full doc

.PHONY: regenerate_on_reconfig
regenerate_on_reconfig: Makefile.common

Makefile.common: Makefile.config Makefile.generate
	@$(MAKE) --no-print-directory -f Makefile.generate all
	@touch Makefile.common

.PHONY: xconfig gconfig menuconfig mconfig config oldconfig
xconfig gconfig menuconfig mconfig config oldconfig:
	@$(MAKE) --no-print-directory -C kconfig $@
	@$(MAKE) --no-print-directory -f Makefile.generate
	-@$(MAKE) --no-print-directory -C kernel-module clean			# Make sure they rebuild Skinware after configuration is redone
	-@$(MAKE) --no-print-directory -C user-side-lib clean

.PHONY: kernel-full kernel drivers services
kernel-full:
	@$(MAKE) --no-print-directory -C kernel-module
kernel:
	@$(MAKE) --no-print-directory -C kernel-module module
drivers:
	@$(MAKE) --no-print-directory -C kernel-module drivers
services:
	@$(MAKE) --no-print-directory -C kernel-module services

.PHONY: library-full library
library-full:
	@$(MAKE) --no-print-directory -C user-side-lib
library:
	@$(MAKE) --no-print-directory -C user-side-lib library calibrator regionalizer

.PHONY: doc
doc:
	@$(MAKE) --no-print-directory -C doc

.PHONY: install uninstall
install uninstall:
	@$(MAKE) --no-print-directory -f Makefile.install $@

.PHONY: clean
clean:
	-@$(MAKE) --no-print-directory -f Makefile.generate clean
	-@$(MAKE) --no-print-directory -C kernel-module clean
	-@$(MAKE) --no-print-directory -C user-side-lib clean
#	-@$(MAKE) --no-print-directory -C doc clean
