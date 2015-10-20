AC_PREREQ([2.68])
AC_INIT([skin], [2.0.0], [https://github.com/maclab/skinware/issues])
AC_CONFIG_AUX_DIR([bin])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([1.14 -Wall -Werror foreign subdir-objects])

AC_CONFIG_SRCDIR([skin/src/base.c])
AC_CONFIG_HEADERS([skin/src/config.h])

# First and foremost, check for URT
urt_config_tool=urt-config
AC_MSG_CHECKING([for working urt-config])
AC_ARG_WITH(urt-config,
  [AS_HELP_STRING([--with-urt-config=<path/to/urt-config>], [Path to urt-config @<:@default=urt-config@:>@])],
  [AS_CASE(["$withval"],
    ["" | y | ye | yes | n | no], [AC_MSG_ERROR([Missing argument to --with-urt-config.  See --help])],
    [*], [urt_config_tool="$withval"])])
# Make sure the given urt-config actually works
AS_IF([$urt_config_tool --version > /dev/null 2>&1],
  [AC_MSG_RESULT([$urt_config_tool])],
  [AC_MSG_ERROR(["$urt_config_tool" did not work])])

# Extract suffix URT was built with
AC_MSG_CHECKING([for suffix of URT])
urt_suffix=${urt_config_tool##*/}
urt_suffix=${urt_suffix#urt}
urt_suffix=${urt_suffix%-config}
AC_MSG_RESULT([$urt_suffix])

# Now use urt-config to see whether building in user or kernel spaces
build_user=$($urt_config_tool --build-user)
build_kernel=$($urt_config_tool --build-kernel)
AC_MSG_CHECKING([whether building for user space])
AC_MSG_RESULT([$build_user])
AC_MSG_CHECKING([whether building for kernel space])
AC_MSG_RESULT([$build_kernel])

# And in either case, get space-specific flags
AS_IF([test x"$build_user" = xy],
  [build_user_cflags=$($urt_config_tool --user-cflags)
   build_user_ldflags=$($urt_config_tool --user-ldflags)])
AS_IF([test x"$build_kernel" = xy],
  [build_kernel_cflags=$($urt_config_tool --kernel-cflags)
   build_kernel_symbols=$($urt_config_tool --kernel-symbols)])

# Checks for programs
AC_PROG_CC_C99
AC_PROG_INSTALL
AM_PROG_AR
AC_PROG_MKDIR_P
AS_IF([test x"$build_user" = xy],
  [AX_CXX_COMPILE_STDCXX_11(, [optional])
   cxx11_cxxflags=$CXXFLAGS
   no_cxx11_cxxflags=$(echo "$cxx11_cxxflags" | sed -e 's/-std=@<:@^ @:>@*1//')])

# Note: AC_PROG_CXX doesn't seem to give any usable test output, so I have to test CXX in another way
have_cxx=no
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_MSG_CHECKING([whether the C++ compiler works])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <iostream>],
                     [std::cout << "test" << std::endl;])],
  [have_cxx=yes],
  [have_cxx=no])
AC_LANG_RESTORE
AC_MSG_RESULT($have_cxx)

LT_INIT

# Make sure C99 is available
AS_IF([test x"$ac_cv_prog_cc_c99" = xno],
  [AC_MSG_ERROR([C99 support is required])])

# Check for python if building for user space
AS_IF([test x"$build_user" = xy],
  [AM_PATH_PYTHON([2.7], , [:])],
  [PYTHON=:])

# Sanitize the suffix for use with a python module name
python_suffix=
AS_IF([test x"$PYTHON" != x:],
  [AC_MSG_CHECKING([suffix suitable for a python module name])
   python_suffix=$(sed -e 's/.*/\L&/' -e 's/@<:@^a-z0-9@:>@/_/g' <<< "$urt_suffix")
   AC_MSG_RESULT(["$python_suffix"])])

# If URT is installed in a subdirectory of the normal include paths, add it to flags
urt_headers_include=
AS_IF([test -f /usr/include/urt"$urt_suffix"/urt.h],
  [urt_headers_include="-I/usr/include/urt'$urt_suffix'"],
  [test -f /usr/local/include/urt"$urt_suffix"/urt.h],
  [urt_headers_include="-I/usr/local/include/urt'$urt_suffix'"])
AS_IF([test x"$build_user" = xy],
  [build_user_cflags="$build_user_cflags $urt_headers_include"])
AS_IF([test x"$build_kernel" = xy],
  [build_kernel_cflags="$build_kernel_cflags $urt_headers_include"])

# Set up a short way of invoking kernel Makefile for module builds
AS_IF([test x"$build_kernel" = xy],
  [kernel_path=/lib/modules/$(uname -r)
   kernel_build_path=$kernel_path/build
   kernel_module_path=$kernel_path/skin"$suffix"
   kernel_invoke="\$(MAKE) -C '$kernel_build_path' M='\$(abs_builddir)' V=\$(V) \
EXTRA_CFLAGS='-Wno-declaration-after-statement -I\$(abs_top_srcdir)/skin/include $build_kernel_cflags \$(KBUILD_EXTRA_CFLAGS)' \
KBUILD_EXTRA_SYMBOLS='$build_kernel_symbols $kernel_path/urt\"$urt_suffix\"/Module.symvers \$(KBUILD_EXTRA_SYMBOLS)' \
INSTALL_MOD_DIR=skin'$urt_suffix' INSTALL_MOD_PATH=\$(DESTDIR)"])

# Checks for typedefs, structures, and compiler characteristics
AS_IF([test x"$build_user" = xy],
  [AC_C_INLINE
   AC_TYPE_INT16_T
   AC_TYPE_INT32_T
   AC_TYPE_INT64_T
   AC_TYPE_INT8_T
   AC_TYPE_SIZE_T
   AC_TYPE_UINT16_T
   AC_TYPE_UINT32_T
   AC_TYPE_UINT64_T
   AC_TYPE_UINT8_T])

# Checks for library functions
AC_CHECK_FUNCS([memmove memset])

# Checks for essential libraries
AS_IF([test x"$build_user" = xy],
  [AC_CHECK_LIB([pthread], [pthread_create])
   AC_CHECK_LIB([urt"$urt_suffix"], [urt_mem_new], , [AC_MSG_ERROR(URT library not found)])])

# Check for optional libraries
gl_libs=
have_gl=n
AS_IF([test x"$have_cxx" = xyes],
  [have_gl=y])
AS_IF([test x"$have_gl" = xy],
  [AC_CHECK_LIB([GL], [glVertex3f], [gl_libs="-lGL"], [have_gl=n])])
AS_IF([test x"$have_gl" = xy],
  [AC_CHECK_LIB([GLU], [gluLookAt], [gl_libs="-lGLU $gl_libs"], [have_gl=n])])
AS_IF([test x"$have_gl" = xy],
  [AC_CHECK_LIB([SDL], [SDL_Init], [gl_libs="-lSDLmain -lSDL $gl_libs"], [have_gl=n])])

have_kio=n
AS_IF([test x"$build_kernel" = xy],
  [AC_MSG_CHECKING([for kio])
   AS_IF([test -f "$kernel_path"/kio/kio.h],
     [have_kio=y])
   AC_MSG_RESULT([$(sed -e 's/n/no/' -e 's/y/yes/' <<< $have_kio)])])

# Check for configuration
max_drivers=10
max_services=20
max_buffers=5
max_sensor_types=10
priority_writer=100
priority_reader=200
priority_driver=50
priority_user=150
default_prefix="SKN"
event_max_delay=1000000
stop_min_wait=1500

SH_GET_CONFIG_NUM(max-drivers, max_drivers, [Maximum number of drivers], 10)
SH_GET_CONFIG_NUM(max-services, max_services, [Maximum number of services], 20)
SH_GET_CONFIG_NUM(max-buffers, max_buffers, [Maximum number of buffers for data transfer], 5)
SH_GET_CONFIG_NUM(max-sensor-types, max_sensor_types, [Maximum number of sensor types provided by a single driver], 10)
SH_GET_CONFIG_NUM(writer-priority, priority_writer, [Default priority of service writers], 100)
SH_GET_CONFIG_NUM(reader-priority, priority_reader, [Default priority of service readers], 200)
SH_GET_CONFIG_NUM(driver-priority, priority_driver, [Default priority of driver writers], 50)
SH_GET_CONFIG_NUM(user-priority, priority_user, [Default priority of driver readers], 150)
SH_GET_CONFIG_STR(default-prefix, default_prefix, [Default URT name prefix for the skin kernel (at most 3 characters used)], SKN)
SH_GET_CONFIG_NUM(event-max-delay, event_max_delay, [Maximum delay for threads to respond to events, such
                                                     as pausing, stopping etc.  Higher values decrease
                                                     responsiveness in such cases, but decrease processor
                                                     usage.  Too low values would result in busy-waiting.
                                                     As a side note, a soft reader is forced to sleep
                                                     by this amount to avoid spamming a sporadic writer.
                                                     This value is in nanoseconds], 1000000)
SH_GET_CONFIG_NUM(stop-min-wait, stop_min_wait, [Minimum amount of time to wait for a thread to stop.
                                                 It is a good idea to keep this value higher than the
                                                 maximum task period you are planning to have.  High
                                                 values result in longer shutdown delays when a task
                                                 is not responding (for example because it is dead).
                                                 This value is in milliseconds], 1500)

# Check for DocThis!
AC_CHECK_PROG(DOCTHIS, docthis, yes)

# Check for pdflatex
AC_CHECK_PROG(PDFLATEX, pdflatex, yes)

AC_DEFINE_UNQUOTED(SKIN_CONFIG_SUPPORT_USER_SPACE, [$(sed -e 's/y/1/' -e 's/n/0/' <<< $build_user)], [Define to 1 if build is only for user-space])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_SUPPORT_KERNEL_SPACE, [$(sed -e 's/y/1/' -e 's/n/0/' <<< $build_kernel)], [Define to 1 if build is only for kernel-space])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_SUFFIX, ["$urt_suffix"], [Suffix taken from URT])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_MAX_DRIVERS, [$max_drivers], [Maximum possible number of drivers])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_MAX_SERVICES, [$max_services], [Maximum possible number of services])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_MAX_BUFFERS, [$max_buffers], [Maximum possible number of buffers for data transfer])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_MAX_SENSOR_TYPES, [$max_sensor_types], [Maximum possible number of sensor types from a single driver])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_PRIORITY_WRITER, [$priority_writer], [Default service writer priority])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_PRIORITY_READER, [$priority_reader], [Default service reader priority])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_PRIORITY_DRIVER, [$priority_driver], [Default driver writer priority])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_PRIORITY_USER, [$priority_user], [Default driver reader priority])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_DEFAULT_PREFIX, ["$default_prefix"], [Default URT name prefix for the skin kernel])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_EVENT_MAX_DELAY, [$event_max_delay], [Maximum delay to respond to an event (in ns)])
AC_DEFINE_UNQUOTED(SKIN_CONFIG_STOP_MIN_WAIT, [$stop_min_wait], [Minimum time to wait for a task to stop (in ms)])
AC_DEFINE_UNQUOTED(HAVE_KIO_H, [$(sed -e 's/y/1/' -e 's/n/0/' <<< $have_kio)], [Define to 1 if you have the <kio.h> header file.])

AC_SUBST(SKIN_SUFFIX, [$urt_suffix])
AC_SUBST(SKIN_SUFFIX_PYTHON, [$python_suffix])
AC_SUBST(SKIN_CFLAGS_USER, [$build_user_cflags])
AC_SUBST(SKIN_CXXFLAGS_USER, ["$no_cxx11_cxxflags ${build_user_cflags//-Wstrict-prototypes/}"])
AC_SUBST(SKIN_CXX11FLAGS_USER, ["$cxx11_cxxflags ${build_user_cflags//-Wstrict-prototypes/}"])
AC_SUBST(SKIN_LDFLAGS_USER, [$build_user_ldflags])
AC_SUBST(SKIN_LDFLAGS_GL, [$gl_libs])
AC_SUBST(SKIN_CFLAGS_KERNEL, [$build_kernel_cflags])
AC_SUBST(SKIN_SYMBOLS_KERNEL, [$build_kernel_symbols])
AC_SUBST(SKIN_KERNEL_PATH, [$kernel_path])
AC_SUBST(SKIN_KERNEL_INVOKE, [$kernel_invoke])
AC_SUBST(SKIN_KERNEL_MODULE_PATH, [$kernel_module_path])
AC_SUBST(KBUILD_CFLAGS, [$KBUILD_CFLAGS])
AC_SUBST(KBUILD_SYMBOLS, [$KBUILD_SYMBOLS])

AM_CONDITIONAL(HAVE_USER, [test x"$build_user" = xy])
AM_CONDITIONAL(HAVE_KERNEL, [test x"$build_kernel" = xy])
AM_CONDITIONAL(HAVE_CXX, [test x"$have_cxx" = xyes])
AM_CONDITIONAL(HAVE_CXX11, [test x"$HAVE_CXX11" = x1])
AM_CONDITIONAL(HAVE_PYTHON, [test x"$PYTHON" != x:])
AM_CONDITIONAL(HAVE_GL, [test x"$have_gl" = xy])
AM_CONDITIONAL(HAVE_DOCTHIS, [test x"$DOCTHIS" = xyes])
AM_CONDITIONAL(HAVE_PDFLATEX, [test x"$PDFLATEX" = xyes])
AM_CONDITIONAL(HAVE_KIO, [test x"$have_kio" = xy])

AC_ARG_VAR([KBUILD_CFLAGS],[KBuild C compiler flags])
AC_ARG_VAR([KBUILD_SYMBOLS],[KBuild symbols files])

AC_CONFIG_FILES([Makefile
                 skin/Makefile
                 skin/src/Makefile
                 skin/include/Makefile
                 doc/Makefile
                 tools/Makefile
                 tools/info/Makefile
                 apps/Makefile
                 apps/calibrator/Makefile])

AS_IF([test x"$have_cxx" = xyes],
  [AS_IF([test x"$HAVE_CXX11" = x1],
     [AC_CONFIG_FILES([skin++/Makefile
                       skin++/src/Makefile
                       skin++/include/Makefile
                       apps/motion/Makefile])])
   AS_IF([test x"$have_gl" = xy],
     [AC_CONFIG_FILES([apps/view/Makefile
                       apps/view/settings/Makefile
                       apps/view/gui/Makefile
                       apps/view/gl/Makefile
                       apps/view/gl/shImage/Makefile
                       apps/view/gl/shFont/Makefile
                       apps/view/gl/Ngin/Makefile
                       apps/view/gl/Ngin3d/Makefile])])
   AS_IF([test "(" x"$have_gl" = xy ")" -o "(" x"$HAVE_CXX11" = x1 ")"],
     [AC_CONFIG_FILES([apps/tools/Makefile])])])

AS_IF([test x"$build_kernel" = xy],
  [AC_CONFIG_FILES([skin/src/Kbuild
                    tools/info/Kbuild
                    apps/calibrator/Kbuild])])

AS_IF([test x"$PYTHON" != x:],
  [AC_CONFIG_FILES([pyskin/Makefile
                    pyskin/skin${python_suffix}.py:pyskin/skin.py.in])])

AC_OUTPUT
