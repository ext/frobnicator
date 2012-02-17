m4_define([__VERSION_MAJOR__], [0])
m4_define([__VERSION_MINOR__], [1])

AC_INIT([frobnicator],[__VERSION_MAJOR__.__VERSION_MINOR__],[ext@sidvind.com])

VERSION_MAJOR=__VERSION_MAJOR__
VERSION_MINOR=__VERSION_MINOR__
AC_DEFINE_UNQUOTED([VERSION_MAJOR], [${VERSION_MAJOR}], [Major version number of package])
AC_DEFINE_UNQUOTED([VERSION_MINOR], [${VERSION_MINOR}], [Minor version number of package])
AC_SUBST([VERSION_MAJOR])
AC_SUBST([VERSION_MINOR])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 foreign color-tests -Wall -Werror])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([frobnicator-dbg], [chmod +x frobnicator-dbg])

AH_BOTTOM([#include "forward.hpp"
])

pkgdatadir="${datadir}/${PACKAGE}"
AC_SUBST(pkgdatadir)
AC_DEFINE_DIR([DATA_DIR], [pkgdatadir], [Data directory])

AC_GNU_SOURCE
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CXX
AX_CHECK_COMPILE_FLAG([-std=c++0x], [CXXFLAGS="$CXXFLAGS -std=c++0x"])

PKG_CHECK_MODULES(yaml, [yaml-0.1])

AC_OUTPUT
