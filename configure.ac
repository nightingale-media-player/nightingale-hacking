#
#=BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2010 POTI, Inc.
# http://www.songbirdnest.com
#
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the ``GPL'').
#
# Software distributed under the License is distributed
# on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
# express or implied. See the GPL for the specific language
# governing rights and limitations.
#
# You should have received a copy of the GPL along with this
# program. If not, go to http://www.gnu.org/licenses/gpl.html
# or write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#=END SONGBIRD GPL
#

#
# Process this file with autoconf to produce a configure script.
#

AC_PREREQ(2.56)
AC_INIT(build/rules.mk)
AC_CONFIG_AUX_DIR(${srcdir}/build/autoconf)
AC_CANONICAL_SYSTEM

#
# We're going to hardcode our obj and dist directory names for the moment.
# These MUST match the vars in nightingale.mk or bad things will happen!
#

OBJDIRNAME="compiled"
DISTDIRNAME="dist"

#
# Defaults, we should probably add more as time permits.
#

SB_ENABLE_TEST_HARNESS=


AC_SUBST(OBJDIRNAME)
AC_SUBST(DISTDIRNAME)

#
# Check for --enable-debug
#

AC_ARG_ENABLE(debug,
              [  --enable-debug          compile a debug version (default=no)])
if test "$enable_debug" = "yes"; then
  DEBUG=1
  SB_CONFIGURATION="debug"
  AC_DEFINE(DEBUG)
  AC_DEFINE(_DEBUG)
else
  DEBUG=
  SB_CONFIGURATION="release"
  AC_DEFINE(NDEBUG)
fi
AC_SUBST(DEBUG)
AC_SUBST(SB_CONFIGURATION)

#
# Check for --enable-jars
#

AC_ARG_ENABLE(jars,
              [  --enable-jars           create jar files (default=yes,debug=no])
if test "$enable_jars" == "yes"; then
  FORCE_JARS=1
fi
if test "$enable_jars" == "no"; then
  PREVENT_JARS=1
fi
AC_SUBST(FORCE_JARS)
AC_SUBST(PREVENT_JARS)

#
#  Check for --enable-official
#

SONGBIRD_BRANDING_DIR='branding'
AC_ARG_ENABLE(official,
              [  --enable-official       create an official build (default=no)])
if test "$enable_official" == "yes"; then
  SONGBIRD_OFFICIAL=1
  SONGBIRD_BRANDING_DIR='branding'
fi
AC_SUBST(SONGBIRD_OFFICIAL)
AC_SUBST(SONGBIRD_BRANDING_DIR)

#
#  Check for --enable-nightly
#

AC_ARG_ENABLE(nightly,
              [  --enable-nightly        create a nightly build (default=no)])
if test "$enable_nightly" == "yes"; then
  SONGBIRD_NIGHTLY=1
  SB_ENABLE_TEST_HARNESS=1
fi
AC_SUBST(SONGBIRD_NIGHTLY)

#
#  Check for --enable-tests
#

AC_ARG_ENABLE(tests,
              [  --enable-tests          build tests (defaults: debug=yes release=no)])
if test "$enable_tests" == "yes"; then
  SB_ENABLE_TESTS=1
  SB_ENABLE_TEST_HARNESS=1
fi
AC_SUBST(SB_ENABLE_TESTS)

#
# Check for --enable-test-harness
#

AC_ARG_ENABLE(test-harness,
              [  --enable-test-harness   build the test harness (default=when tests enabled)])
if test "$enable_test_harness" == "yes"; then
  SB_ENABLE_TEST_HARNESS=1
fi

if test "$enable_test_harness" == "no"; then
  SB_ENABLE_TEST_HARNESS=
fi

AC_SUBST(SB_ENABLE_TEST_HARNESS)

#
#  Check for --enable-jemalloc
#

SB_USE_JEMALLOC=1

AC_ARG_ENABLE(jemalloc,
              [  --enable-jemalloc       use the jemalloc memory allocator (default=yes)])
if test "$enable_jemalloc" == "no"; then
  SB_USE_JEMALLOC=
fi
AC_SUBST(SB_USE_JEMALLOC)

#
# Check for --enable-update-channel
#

AC_ARG_ENABLE(update-channel,
              [  --enable-update-channel=CHANNEL    which update channel to use ("default")],
              SB_UPDATE_CHANNEL=`echo $enableval | tr A-Z a-z`)
if test -z "$SB_UPDATE_CHANNEL"; then
  SB_UPDATE_CHANNEL=default
fi
AC_SUBST(SB_UPDATE_CHANNEL)

#
# Check for --with-deadly-warnings
#

SB_WITH_DEADLY_WARNINGS=

AC_ARG_WITH(deadly-warnings,
            [  --with-deadly-warnings=<debug,release,all,none,force>,conditions under which compiler warnings are considered build failures (default=release)],
            [ SB_WITH_DEADLY_WARNINGS=$withval ],
            [ SB_WITH_DEADLY_WARNINGS=all])

case "$SB_WITH_DEADLY_WARNINGS" in
  debug|release|all|none|force)
  ;;
  *)
    AC_MSG_ERROR([Unknown --with-deadly warnings option; use debug, release, all, none, or force])
  ;;
esac

AC_SUBST(SB_WITH_DEADLY_WARNINGS)

#
# Check for --with-songbird-extensions
#

SB_BIRD_EXTENSIONS=default

AC_ARG_WITH(songbird-extensions,
            [  --with-songbird-extensions=<list of songbird extensions>,list of songbird extensions to build (default=platform dependent)],
            [ SB_BIRD_EXTENSIONS=$withval ],
            [ SB_BIRD_EXTENSIONS=default])
AC_SUBST(SB_BIRD_EXTENSIONS)

#
# Check for --with-extensions
#

SB_EXTENSIONS=default

AC_ARG_WITH(extensions,
            [  --with-extensions=<list of extensions>,list of extensions to build (default: xpcom_helloworld)],
            [ SB_EXTENSIONS=$withval ],
            [ SB_EXTENSIONS=default])
AC_SUBST(SB_EXTENSIONS)

#
# Check for enabled library performance tests
#

AC_SUBST(SB_ENABLE_LIBRARY_PERF)

#
# Check for --enable-sqlite-debug
#

SB_SQLITE_DEBUG=

AC_ARG_ENABLE(sqlite-debug,
              [  --enable-sqlite-debug   enable sqlite debug assertions (default=no)])
if test "$enable_sqlite_debug" == "yes"; then
  SB_SQLITE_DEBUG=1
fi
AC_SUBST(SB_SQLITE_DEBUG)

#
#  Check for --enable-device-drivers
#

AC_ARG_ENABLE(device-drivers,
              [  --enable-device-drivers  include device drivers (default=yes)])
if test "$enable_device_drivers" == "no"; then
  SB_ENABLE_DEVICE_DRIVERS=0
else
  SB_ENABLE_DEVICE_DRIVERS=1
fi
AC_SUBST(SB_ENABLE_DEVICE_DRIVERS)

#
# Check platform
#

case "$target" in
  *-cygwin*|*-mingw*|*-msvc*|*-mks*)
    SB_PLATFORM="windows"
    case "$target_cpu" in
      *i*86*)
        SB_ARCH="$target_cpu"
      ;;
      *)
        AC_MSG_ERROR(Unsupported architecture)
      ;;
    esac
    # Check that we're MSYS
    case "$target" in
      *-mingw*)
        # no cygwin wrapper for mingw
      ;;
      *)
        AC_MSG_ERROR(Songbird must be built using MozillaBuild/MSYS)
      ;;
    esac
  ;;
  *darwin*)
    SB_PLATFORM="macosx"
    case "$target_cpu" in
      powerpc* | ppc)
        SB_ARCH="ppc"
      ;;
      *i*86*)
        SB_ARCH="$target_cpu"
      ;;
      *)
        AC_MSG_ERROR(Unsupported architecture)
      ;;
    esac
  ;;
  *linux*)
    SB_PLATFORM="linux"
    case "$target_cpu" in
      *86)
        SB_ARCH="$target_cpu"
      ;;
      x86_64)
        SB_ARCH="$target_cpu"
      ;;
      powerpc | ppc)
        SB_ARCH="ppc"
      ;;
      powerpc64 | ppc64)
        SB_ARCH="ppc64"
      ;;
      *)
        AC_MSG_ERROR(Unsupported architecture)
      ;;
    esac
  ;;
  *solaris*)
    SB_PLATFORM="solaris"
    case "$target_cpu" in
      *86)
        SB_ARCH="$target_cpu"
      ;;
      x86_64)
        SB_ARCH="$target_cpu"
      ;;
      sparc)
        SB_ARCH="$target_cpu"
      ;;
      *)
        AC_MSG_ERROR(Unsupported architecture)
      ;;
    esac
  ;;
  *)
    AC_MSG_ERROR(Unsupported platform $target)
  ;;
esac
AC_SUBST(SB_PLATFORM)
AC_SUBST(SB_ARCH)

#
#  Check for --enable-installer
#

# Always set SB_INSTALLER_SUFFIXES so that the installer can be built
# manually as expected. These are the defaults.

case $SB_PLATFORM in
  windows) SB_INSTALLER_SUFFIXES=exe;;
  macosx)  SB_INSTALLER_SUFFIXES=dmg;;
  *)       SB_INSTALLER_SUFFIXES=tar.gz;;
esac

# Now process --enable-installer options.

AC_ARG_ENABLE(installer,
              [  --enable-installer=TYPE create installer (default=no, use "nsis", "dmg", "pkg", "tar". "yes" will build default installer type.)],
              [enable_installer=`echo $enableval | tr 'A-Z,' 'a-z '`])
case $enable_installer in
  yes)
    MAKE_INSTALLER=1
    ;;
  no)
    AS_UNSET(MAKE_INSTALLER)
    ;;
  *)
    AS_UNSET(MAKE_INSTALLER)
    for word in $enable_installer; do
      case $word in
        nsis)
          AS_IF([test $SB_PLATFORM = "windows"],
                [SB_INSTALLER_SUFFIXES="$SB_INSTALLER_SUFFIXES $word"],
                [AC_MSG_ERROR([NSIS installer not supported on $SB_PLATFORM])])
          MAKE_INSTALLER=1
          ;;
        dmg|pkg)
          AS_IF([test $SB_PLATFORM = "macosx"],
                [SB_INSTALLER_SUFFIXES="$SB_INSTALLER_SUFFIXES $word"],
                [AC_MSG_ERROR([$word installer not supported on $SB_PLATFORM])])
          MAKE_INSTALLER=1
          ;;
        tar.gz)
          AS_IF([test $SB_PLATFORM != "windows" -a $SB_PLATFORM != "macosx"],
                [SB_INSTALLER_SUFFIXES="$SB_INSTALLER_SUFFIXES $word"],
                [AC_MSG_ERROR([$word installer not supported on $SB_PLATFORM])])
          MAKE_INSTALLER=1
          ;;
        *)
          AC_MSG_ERROR([Installer type $word not recognized])
          ;;
      esac
    done
    ;;
esac
AC_SUBST(MAKE_INSTALLER)
AC_SUBST(SB_INSTALLER_SUFFIXES)

#
# Check for --with-macosx-developer-tools
#

SB_MACOSX_DEV_ROOT_DEFAULT=
SB_MACOSX_DEV_ROOT=

case $SB_PLATFORM in
  macosx)
    SB_MACOSX_DEV_ROOT_DEFAULT=/Developer
  ;;
esac
AC_SUBST(SB_MACOSX_DEV_ROOT_DEFAULT)

AC_ARG_WITH(macosx-developer-tools,
            [  --with-macosx-developer-tools=<directory>,path to directory containing the Mac OS X developer tools (default=/Developer) ],
            [ SB_MACOSX_DEV_ROOT=$withval ],
            [ SB_MACOSX_DEV_ROOT=$SB_MACOSX_DEV_ROOT_DEFAULT])
AC_SUBST(SB_MACOSX_DEV_ROOT)

#
# Handle SDK detection
#
#
# Initialize SDK paths to nothing for all platforms, and try to guess the
# location of SDKs for certain platforms; we protect this in a case statement
# so we're not running e.g. win32-detection commands on mac.
#

SB_MACOSX_SDK_DEFAULT=

case $SB_PLATFORM in
  macosx)
    SB_MACOSX_SDK_DEFAULT=$SB_MACOSX_DEV_ROOT/SDKs/MacOSX10.5.sdk
  ;;
esac

#
# Check for --with-macosx-sdk
#

AC_ARG_WITH(macosx-sdk,
            [  --with-macosx-sdk=<directory>,path to directory containing the Mac OS X SDK (default=<dev-tools>/SDKs/MacOSX10.5.sdk) ],
            [ SB_MACOSX_SDK=$withval ],
            [ SB_MACOSX_SDK=$SB_MACOSX_SDK_DEFAULT])

#
# Breakpad support
#

AC_ARG_ENABLE(breakpad,
              [  --enable-breakpad       enable breakpad (default=yes in official builds)])
# off by default in non-official builds
if test -n "$SONGBIRD_OFFICIAL"; then
  SB_ENABLE_BREAKPAD=1
fi

# allow explicit overrides
if test "$enable_breakpad" == "yes"; then
  SB_ENABLE_BREAKPAD=1
elif test "$enable_breakpad" == "no"; then
  SB_ENABLE_BREAKPAD=
fi

# breakpad is not yet implemented on x86_64
if test "$SB_ARCH" == "x86_64"; then
  SB_ENABLE_BREAKPAD=
fi

AC_SUBST(SB_ENABLE_BREAKPAD)

#
# Experimental static build support
#

AC_ARG_ENABLE(static,
              [  --enable-static         experimental single-component static build support (default=no)])
if test "$enable_static" == "yes"; then
  SB_ENABLE_STATIC=1
else
  SB_ENABLE_STATIC=
fi

AC_SUBST(SB_ENABLE_STATIC)

#
# Compiler environment checks
#

AC_ARG_ENABLE(compiler-environment-checks,
              [  --enable-compiler-environment-checks       check the compiler environment (default=yes)])
if test "$enable_compiler_environment_checks" == "no"; then
  SB_ENABLE_COMPILER_ENVIRONMENT_CHECKS=
  AC_MSG_WARN([Compiler environment checks disabled.])
else
  SB_ENABLE_COMPILER_ENVIRONMENT_CHECKS=1
fi


#
# Checks for programs.
#

# Use the developer tools specified by --with-macosx-developer-tools
if test "$SB_PLATFORM" == "macosx"; then
  PATH="${SB_MACOSX_DEV_ROOT}/usr/bin:${PATH}"
fi

# Don't let autoconf hijack our flags
_SAVE_CFLAGS=$CFLAGS
_SAVE_CXXFLAGS=$CXXFLAGS

#
# Must use gcc 4.x on Mac OS X with the 10.5 SDK.
# NOTE: gcc4.0 is the default compiler on 10.5 but not on 10.6 and we still
#       support back to 10.4. See bug 17752 for juicy details.
#
case "$SB_PLATFORM" in
  windows)
  AC_PROG_CC(cl gcc)
  AC_PROG_CXX(cl g++)
  ;;
  solaris)
  AC_PROG_CC(cc gcc)
  AC_PROG_CXX(CC g++)
  ;;
  macosx)
  AC_PROG_CC(gcc-4.0 gcc)
  AC_PROG_CXX(g++-4.0 g++)
  ;;
  *)
  AC_PROG_CC
  AC_PROG_CXX
  ;;
esac

# Restore saved flags
CFLAGS=$_SAVE_CFLAGS
CXXFLAGS=$_SAVE_CXXFLAGS
GNU_CC=$GCC
AC_SUBST(GNU_CC)

AC_CHECK_PROGS(AWK, ${AWK} gawk awk)
if test -z "$AWK"; then AC_MSG_ERROR(Awk not found); fi

AC_CHECK_PROG(CHMOD, chmod, chmod)
if test -z "$CHMOD"; then AC_MSG_ERROR(Chmod not found); fi

AC_CHECK_PROGS(CP, ${CP} gnucp gcp cp)
if test -z "$CP"; then AC_MSG_ERROR(Cp not found); fi

AC_CHECK_PROGS(DIFF, ${DIFF} gdiff diff, "")

AC_CHECK_PROGS(DOXYGEN, ${DOXYGEN} doxygen, "")

AC_CHECK_PROGS(GREP, ${GREP} ggrep grep)

AC_CHECK_PROG(GZIP, gzip, gzip)

AC_CHECK_PROG(GUNZIP, gunzip, gunzip)

AC_CHECK_PROG(FIND, find, find)
if test -z "$FIND"; then AC_MSG_ERROR(Find not found); fi

AC_CHECK_PROGS(INSTALL, ${INSTALL} install)
if test -z "$INSTALL"; then AC_MSG_ERROR(Install not found); fi

AC_CHECK_PROG(LN, ln, ln)
if test -z "$LN"; then AC_MSG_ERROR(Ln not found); fi

AC_PATH_PROGS(MACPKGMAKER, packagemaker, false, $SB_MACOSX_DEV_ROOT/usr/bin)

AC_CHECK_PROGS(MD5SUM, ${MD5SUM} gmd5sum md5sum md5, "")

AC_CHECK_PROG(MKDIR, mkdir, mkdir)
if test -z "$MKDIR"; then AC_MSG_ERROR(Mkdir not found); fi

AC_CHECK_PROG(MV, mv, mv)
if test -z "$MV"; then AC_MSG_ERROR(Mv not found); fi

AC_CHECK_PROG(PERL, perl, perl)
if test -z "$PERL"; then AC_MSG_ERROR(Perl not found); fi

AC_CHECK_PROGS(PYTHON, python2 python, python)
if test -z "$PYTHON"; then AC_MSG_ERROR(Python not found); fi

AC_CHECK_PROG(RM, rm, rm)
if test -z "$RM"; then AC_MSG_ERROR(Rm not found); fi

AC_CHECK_PROGS(SED, ${SED} gsed sed, "")

AC_CHECK_PROGS(SHA1SUM, ${SHA1SUM} gsha1sum sha1sum, "")

AC_CHECK_PROGS(SORT, ${SORT} sort, "")

AC_CHECK_PROGS(TAR, ${TAR} gtar tar)

AC_CHECK_PROGS(TOUCH, ${TOUCH} touch, "")

AC_CHECK_PROG(UNZIP, unzip, unzip)
if test -z "$UNZIP"; then AC_MSG_ERROR(Unzip not found); fi

AC_CHECK_PROG(ZIP, zip, zip)
if test -z "$ZIP"; then AC_MSG_ERROR(Zip not found); fi

#
# Check for extras.
#

HAS_EXTRAS=
if test -d "${srcdir}/extras"; then
  HAS_EXTRAS=1
fi
AC_SUBST(HAS_EXTRAS)

case "$SB_PLATFORM" in
  windows)
    AC_CHECK_PROG(LD, link, link)
    if test -z "$LD"; then AC_MSG_ERROR(Linker not found); fi
    AC_CHECK_PROG(AR, lib, lib)
    if test -z "$AR"; then AC_MSG_ERROR(Librarian not found); fi
    AC_CHECK_PROG(RC, rc, rc)
    if test -z "$RC"; then AC_MSG_ERROR(Rc not found); fi
    AC_CHECK_PROG(MIDL, midl, midl)
    if test -z "$MIDL"; then AC_MSG_ERROR(Midl not found); fi
    AC_CHECK_PROG(REBASE, rebase, rebase)
    if test -z "$REBASE"; then AC_MSG_ERROR(Rebase not found); fi

    # Check midl version
    changequote(,)
    _MIDL_VER_FILTER='s|.* \([0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*|\1|p'
    changequote([,])
    
    _MIDL_FULL_VERSION=`"${MIDL}" -v 2>&1 | sed -ne "$_MIDL_VER_FILTER"`
    _MIDL_MAJOR_VERSION=`echo ${_MIDL_FULL_VERSION} | $AWK -F\. '{ print $1 }'`
    _MIDL_MINOR_VERSION=`echo ${_MIDL_FULL_VERSION} | $AWK -F\. '{ print $2 }'`
    _MIDL_REV_VERSION=`echo ${_MIDL_FULL_VERSION} | $AWK -F\. '{ print $3 }'`
    
    # Add flags if necessary
    AC_MSG_CHECKING([for midl flags])
    case "$target" in
    i*86-*)
        if test \( "$_MIDL_MAJOR_VERSION" = "7" -a "$_MIDL_MINOR_VERSION" = "00" -a "$_MIDL_REV_VERSION" = "0499" \); then
            # MIDL version 7.00.0499 defaults /env win64 on x64 platforms.
            # MIDL version 7.00.0500 or later has no problem.
            DEFAULT_MIDL_FLAGS="${MIDL_FLAGS} -env win32 -Oicf -robust"
            AC_MSG_RESULT([need -env win32])
        else
            AC_MSG_RESULT([none needed])
        fi
        ;;
    *)
        AC_MSG_RESULT([none needed])
        ;;
    esac

    # Tests for bits of the Windows SDK
    AC_CHECK_HEADERS(wmsdk.h, [], AC_MSG_ERROR(Missing wmsdk.h; install the Windows SDK))

    # Only run this check if we don't have extras; if we do, we'll find it
    # that way.
    if test -z "$HAS_EXTRAS"; then
       AC_CHECK_HEADERS([devioctl.h], [], AC_MSG_ERROR([Missing devioctl.h; install the Windows Driver Kit]))
    fi

    _SAVE_LIBS=$LIBS
    LIBS="$LIBS wmvcore.lib"
    AC_MSG_CHECKING([wmvcore.lib])
    AC_LINK_IFELSE([AC_LANG_SOURCE([#include <wmsdk.h>
                    int main() { return WMCreateSyncReader(NULL, 0, NULL); }])],
                    AC_MSG_RESULT([yes]),
                    AC_MSG_ERROR([Missing wmvcore.lib; install the Windows SDK]))
    LIBS=$_SAVE_LIBS

    #
    # Force use of the freely-downloadable MSVC Express compiler
    #

    AC_LANG_PUSH([C++])
    AC_ARG_WITH(msvc-express,
                [  --with-msvc-express       limit to features buildable with MSVC Express],
                [SB_WITH_MSVC_EXPRESS=1
                 AC_MSG_WARN([Limiting functionality to MSVC Express-compatible])],
                [SB_WITH_MSVC_EXPRESS=
                 AC_CHECK_HEADER([atlbase.h],
                                 [],
                                 [AC_MSG_ERROR([Missing altbase.h])])])
    AC_SUBST([SB_WITH_MSVC_EXPRESS])
    AC_LANG_POP([C++])
  ;;
  *)
    AC_CHECK_PROG(LD, ${CXX}, ${CXX})
    if test -z "$LD"; then AC_MSG_ERROR(Linker not found); fi
    AC_CHECK_PROG(AR, ar, ar)
    if test -z "$AR"; then AC_MSG_ERROR(Archiver not found); fi
    AC_CHECK_PROG(RANLIB, ranlib, ranlib)
    if test -z "$RANLIB"; then AC_MSG_ERROR(Ranlib not found); fi
    if test -z "$GZIP"; then AC_MSG_ERROR(Gzip not found); fi
    if test -z "$GUNZIP"; then AC_MSG_ERROR(Gunzip not found); fi
    if test -z "$TAR"; then AC_MSG_ERROR(Tar not found); fi
    AC_CHECK_PROGS(STRIP, ${STRIP} strip)
    if test -z "$STRIP"; then AC_MSG_WARN(strip not found); fi
  ;;
esac

#
# Set platform-specific defines
#

case "$SB_PLATFORM" in
  windows)
    BIN_SUFFIX=".exe"
    OBJ_SUFFIX=".obj"
    LIB_PREFIX=""
    LIB_SUFFIX=".lib"
    DLL_SUFFIX=".dll"
    INSTALLER_SUFFIX=".exe"
    AC_DEFINE(UNICODE)
    AC_DEFINE(_UNICODE)
    AC_DEFINE(_WINDOWS)
    AC_DEFINE(_WIN32)
    AC_DEFINE(WIN32)
    AC_DEFINE(XP_WIN)
    AC_DEFINE(XP_WIN32)
    AC_DEFINE(WIN32_LEAN_AND_MEAN)
    AC_DEFINE(SB_USE_JEMALLOC)
  ;;
  macosx)
    BIN_SUFFIX=
    OBJ_SUFFIX=".o"
    LIB_PREFIX="lib"
    LIB_SUFFIX=".a"
    DLL_SUFFIX=".dylib"
    INSTALLER_SUFFIX=".dmg"
    AC_DEFINE(XP_MACOSX)
    AC_DEFINE(XP_UNIX)
    AC_DEFINE(NO_X11)
  ;;
  linux|solaris)
    BIN_SUFFIX=
    OBJ_SUFFIX=".o"
    LIB_PREFIX="lib"
    LIB_SUFFIX=".a"
    DLL_SUFFIX=".so"
    INSTALLER_SUFFIX=".tar.gz"
    AC_DEFINE(XP_UNIX)
    AC_DEFINE(_REENTRANT)
  ;;
esac
AC_SUBST(BIN_SUFFIX)
AC_SUBST(OBJ_SUFFIX)
AC_SUBST(LIB_PREFIX)
AC_SUBST(LIB_SUFFIX)
AC_SUBST(DLL_SUFFIX)
AC_SUBST(INSTALLER_SUFFIX)

#
# Set platform-specific flags
#

case "$SB_PLATFORM" in
  windows)
    # We use MSVC8 on Windows
    _CC_SUPPORTED_MAJOR_VERSION=14

    changequote(,)
    _MSVC_VER_FILTER='s|.* \([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*|\1|p'
    changequote([,])

    # Determine compiler version
    CC_VERSION=`"${CC}" -v 2>&1 | sed -ne "$_MSVC_VER_FILTER"`
    _CC_MAJOR_VERSION=`echo ${CC_VERSION} | $AWK -F\. '{ print $1 }'`
    _CC_MINOR_VERSION=`echo ${CC_VERSION} | $AWK -F\. '{ print $2 }'`
    _CC_BUILD_VERSION=`echo ${CC_VERSION} | $AWK -F\. '{ print $4 }'`
    _MSC_VER=${_CC_MAJOR_VERSION}${_CC_MINOR_VERSION}

    CXX_VERSION=`"${CXX}" -v 2>&1 | sed -ne "$_MSVC_VER_FILTER"`
    _CXX_MAJOR_VERSION=`echo ${CXX_VERSION} | $AWK -F\. '{ print $1 }'`

    if test "$_CC_MAJOR_VERSION" != "$_CXX_MAJOR_VERSION"; then
        if test -z "$SB_ENABLE_COMPILER_ENVIRONMENT_CHECKS"; then
            AC_MSG_WARN([The major versions of \$CC and \$CXX do not match. Songbird Bugzilla reports should not be filed against builds produced using them.])
        else
            AC_MSG_ERROR([The major versions of \$CC and \$CXX do not match.])
        fi
    fi
    case "$_CC_MAJOR_VERSION" in
        14)
            _CC_SUITE=8
            SB_ARCH=$SB_ARCH-msvc8
            # We need at least SP1 (bug 21339)
            if test "$_CC_BUILD_VERSION" -lt 762 ; then
                # SP0 is 14.00.50727.42
                # SP1 is 14.00.50727.762
                if test -z "$SB_ENABLE_COMPILER_ENVIRONMENT_CHECKS"; then
                    AC_MSG_WARN([Visual Studio 8 (2005) without Service Pack 1 detected; your build may not complete.])
                else
                    AC_MSG_ERROR([Visual Studio 8 (2005) requires Service Pack 1 to build Songbird. Please install KB926601.])
                fi
            fi
            ;;
        15)
            _CC_SUITE=9
            ;;
        16)
            _CC_SUITE=10
            ;;
    esac
    if test "$_CC_MAJOR_VERSION" -lt "$_CC_SUPPORTED_MAJOR_VERSION"; then
        if test -z "$SB_ENABLE_COMPILER_ENVIRONMENT_CHECKS"; then
            AC_MSG_WARN([This version of the MSVC compiler, $CC_VERSION, is unsupported. Songbird Bugzilla reports should not be filed against builds produced using it.])
        else
            AC_MSG_ERROR([This version of the MSVC compiler, $CC_VERSION, is unsupported.])
        fi
    fi

    # mozilla bug #249782
    # ensure that mt.exe is Microsoft (R) Manifest Tool and not magnetic tape manipulation utility (or something else)
    if test "$_CC_SUITE" -ge "8"; then
            changequote(,)
            _MSMT_VER_FILTER='s|.* \([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*|\1|p'
            changequote([,])

            MSMT_TOOL=`mt 2>&1|grep 'Microsoft (R) Manifest Tool'`
            if test -n "$MSMT_TOOL"; then
                    MSMANIFEST_TOOL_VERSION=`echo ${MSMT_TOOL}|sed -ne "$_MSMT_VER_FILTER"`
                    if test -z "$MSMANIFEST_TOOL_VERSION"; then
                            AC_MSG_WARN([Unknown version of the Microsoft (R) Manifest Tool.])
                    fi
                    MSMANIFEST_TOOL="mt"
                    unset MSMT_TOOL
            else
                    AC_MSG_ERROR([Microsoft (R) Manifest Tool must be in your \$PATH.])
            fi
    fi

    # Check linker version
    _LD_FULL_VERSION=`"${LD}" -v 2>&1 | sed -ne "$_MSVC_VER_FILTER"`
    _LD_MAJOR_VERSION=`echo ${_LD_FULL_VERSION} | $AWK -F\. '{ print $1 }'`
    if test "$_LD_MAJOR_VERSION" != "$_CC_SUITE"; then
        AC_MSG_ERROR([The linker major version, $_LD_FULL_VERSION, does not match the compiler suite version, $_CC_SUITE.])
    fi

    MSVC_COMPILER_FLAGS="-nologo -W3 -WL -GS -EHsc -Zi -c"

    if test "$_CC_SUITE" -ge "8"; then
      # MSVC 10 gives linker errors (https://bugzilla.mozilla.org/show_bug.cgi?id=682259)
      # MSVC 8+ use native wchar_t, but Mozilla builds without it
      # MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -Zc:wchar_t"
      MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS"
    fi

    if test "$enable_debug" = "yes"; then
      MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -Od -RTC1 -RTCc"
    else
      MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -Ox"
    fi

    if test "$enable_debug" == "yes"; then
      CFLAGS_STATIC_LIBC="-MTd"
    else
      CFLAGS_STATIC_LIBC="-MT"
    fi
    if test "$SB_USE_JEMALLOC" != "1"; then
      MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS $CFLAGS_STATIC_LIBC"
    fi

    # We always want to include "mozilla-config.h"
    MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -FI \"mozilla-config.h\" -FI \"xpcom-config.h\""

    MSVC_LINKER_FLAGS="-NOLOGO -INCREMENTAL:NO -SUBSYSTEM:WINDOWS -MACHINE:X86 -DEBUG"
    if test "$enable_debug" != "yes"; then
      MSVC_LINKER_FLAGS="$MSVC_LINKER_FLAGS -OPT:REF -OPT:ICF"
    fi

    if test "$SB_USE_JEMALLOC" == "1"; then
      # if test "$enable_debug" == "yes"; then
      #   MOZ_CRT_DLL_NAME="mozcrt19d"
      # else
      #   MOZ_CRT_DLL_NAME="mozcrt19"
      # fi

      # MSVC_DLL_LINKER_FLAGS="-NODEFAULTLIB:MSVCRT -NODEFAULTLIB:MSVCRTD -NODEFAULTLIB:LIBCMT -NODEFAULTLIB:LIBCMTD -DEFAULTLIB:$MOZ_CRT_DLL_NAME"
      MSVC_DLL_LINKER_FLAGS=""
    fi

    CFLAGS="$CFLAGS $MSVC_COMPILER_FLAGS -TC"
    CXXFLAGS="$CXXFLAGS $MSVC_COMPILER_FLAGS -TP"
    CFLAGS_INCLUDE_PREFIX="-I"
    CFLAGS_INCLUDE_SUFFIX=""
    CFLAGS_PREPROCESS="-P"
    CFLAGS_ASSEMBLER="-FAs"
    # Always disable warning 4624, because the XPCOM elicits this warning.
    # Also disable warning 4800, it isn't useful, and using nsString with
    # std::map causes it
    CFLAGS_WARNING_IS_ERROR="-wd4624 -wd4800"
    LDFLAGS="$LDFLAGS $MSVC_LINKER_FLAGS"
    LDFLAGS_DLL="-DLL $MSVC_DLL_LINKER_FLAGS"
    LDFLAGS_BIN=""
    LDFLAGS_LIB=""
    LDFLAGS_OUT_PREFIX="-OUT:\""
    LDFLAGS_OUT_SUFFIX="\""
    LDFLAGS_PATH_PREFIX="-LIBPATH:\""
    LDFLAGS_PATH_SUFFIX="\""
    LDFLAGS_IMPORT_PREFIX=""
    LDFLAGS_IMPORT_SUFFIX="${LIB_SUFFIX}"
    DEFAULT_LIBS="kernel32 user32 gdi32 winmm wsock32 advapi32 comctl32 ole32 oleaut32"
    LNFLAGS="-f --symbolic"
    ARFLAGS="-NOLOGO"
    ARFLAGS_OUT_PREFIX="-OUT:\""
    ARFLAGS_OUT_SUFFIX="\""
    ARFLAGS_PATH_PREFIX=""
    ARFLAGS_PATH_SUFFIX=""
    ARFLAGS_LIB=""
    UNZIPFLAGS="-u -o -q -X"
    UNZIPFLAGS_EXTRACT="-d"
    XULRUNNERDIR="/xulrunner"
    COMPILER_GARBAGE="vc70.pdb vc71.pdb vc80.pdb vc90.pdb"
  ;;
  macosx)
    if ! test -e $SB_MACOSX_SDK/SDKSettings.plist; then
      AC_ERROR(Invalid Mac OS X SDK directory: $SB_MACOSX_SDK)
    fi

    MACOSX_APPBUNDLE="Nightingale.app"
    MACOSX_CONTENTS="/$MACOSX_APPBUNDLE/Contents"
    MACOSX_RESOURCES="$MACOSX_CONTENTS/Resources"
    MACOSX_FRAMEWORKS="$MACOSX_CONTENTS/Frameworks"
    XULRUNNERDIR="/XUL.framework"

    GCC_COMPILER_FLAGS="-c -fexceptions -fnon-call-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-common -fpascal-strings -no-cpp-precomp -Wall -Wconversion -Wpointer-arith -Wcast-align -Wno-long-long -fshort-wchar -pipe"
    GCC_CPP_COMPILER_FLAGS="-Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor -fno-rtti"

    if test "$enable_debug" = "yes"; then
      GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -g -ggdb3 -fno-inline"
    else
      # Use -O2 on OS X, better than -Os according to moz perf tests
      GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -gstabs+ -O2"
    fi

    # We always want to include "mozilla-config.h"
    GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -include \"mozilla-config.h\""

    GCC_LINKER_FLAGS="-lpthread -lobjc -shared-libgcc -lstdc++ -Wl,-executable_path,\$(topsrcdir)/$OBJDIRNAME/$DISTDIRNAME$MACOSX_FRAMEWORKS$XULRUNNERDIR"

    CFLAGS="$CFLAGS $GCC_COMPILER_FLAGS"
    CXXFLAGS="$CXXFLAGS $GCC_COMPILER_FLAGS $GCC_CPP_COMPILER_FLAGS"
    CMMFLAGS="$CMMFLAGS $GCC_COMPILER_FLAGS $GCC_CPP_COMPILER_FLAGS -fobjc-exceptions -DUSE_COCOA"
    CFLAGS_INCLUDE_PREFIX="-I"
    CFLAGS_INCLUDE_SUFFIX=""
    CFLAGS_PREPROCESS="-E"
    CFLAGS_ASSEMBLER="-S"
    CFLAGS_WARNING_IS_ERROR=""
    LDFLAGS="$LDFLAGS $GCC_LINKER_FLAGS"
    LDFLAGS_DLL=""
    LDFLAGS_BIN=""
    LDFLAGS_LIB="-static"
    LDFLAGS_OUT_PREFIX="-o "
    LDFLAGS_OUT_SUFFIX=""
    LDFLAGS_PATH_PREFIX="-L"
    LDFLAGS_PATH_SUFFIX=""
    LDFLAGS_IMPORT_PREFIX="-l"
    LDFLAGS_IMPORT_SUFFIX=""
    LNFLAGS="-f -s"
    ARFLAGS="cr"
    ARFLAGS_OUT_PREFIX=""
    ARFLAGS_OUT_SUFFIX=""
    ARFLAGS_PATH_PREFIX=""
    ARFLAGS_PATH_SUFFIX=""
    ARFLAGS_LIB=""
    UNZIPFLAGS="-u -o -q -X"
    UNZIPFLAGS_EXTRACT="-d"
    USING_RANLIB=1
    STRIP_FLAGS="-x -S"
    AC_SUBST(SB_MACOSX_SDK)
  ;;
  linux)
    XULRUNNERDIR="/xulrunner"

    _SAVE_LIBS=$LIBS
    LIBS=
    AC_SEARCH_LIBS([pthread_create], [pthread])
    AC_SUBST(PTHREAD_LIBS, $LIBS)
    LIBS=$_SAVE_LIBS

    PKG_CHECK_MODULES(GTK, gtk+-2.0)
    AC_SUBST(GTK_CFLAGS)
    AC_SUBST(GTK_LIBS)

    PKG_CHECK_MODULES(GLIB, glib-2.0)
    AC_SUBST(GLIB_CFLAGS)
    AC_SUBST(GLIB_LIBS)

    # The iPod support needs dbus.
    PKG_CHECK_MODULES(DBUS, dbus-glib-1)
    AC_SUBST(DBUS_CFLAGS)
    AC_SUBST(DBUS_LIBS)

    # Breakpad needs stabs debugging data, so always explicitly request it with
    GCC_COMPILER_FLAGS="-c -fPIC -fshort-wchar -fexceptions -fnon-call-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -fno-strict-aliasing -Wall -Wno-conversion -Wno-attributes -Wpointer-arith -Wcast-align -Wno-long-long -pipe -pthread"
    if test "$enable_debug" = "yes"; then
      GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -g -ggdb3 -fno-inline"
    else
      # Use -Os on linux, better than -O2 according to moz perf tests
      GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -gstabs+ -Os"
    fi

    # We always want to include "mozilla-config.h"
    GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -include \"mozilla-config.h\""

    GCC_LINKER_FLAGS="-Wl,-z,defs -Wl,-rpath-link,\$(topsrcdir)/$OBJDIRNAME/$DISTDIRNAME$XULRUNNERDIR"

    CFLAGS="$CFLAGS $GCC_COMPILER_FLAGS"
    CXXFLAGS="$CXXFLAGS $GCC_COMPILER_FLAGS -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor"
    CFLAGS_INCLUDE_PREFIX="-I"
    CFLAGS_INCLUDE_SUFFIX=""
    CFLAGS_PREPROCESS="-E"
    CFLAGS_ASSEMBLER="-S"
    CFLAGS_WARNING_IS_ERROR=""
    LDFLAGS="$LDFLAGS $GCC_LINKER_FLAGS"
    LDFLAGS_DLL="-shared"
    LDFLAGS_BIN=""
    LDFLAGS_LIB="-static"
    LDFLAGS_OUT_PREFIX="-o "
    LDFLAGS_OUT_SUFFIX=""
    LDFLAGS_PATH_PREFIX="-L"
    LDFLAGS_PATH_SUFFIX=""
    LDFLAGS_IMPORT_PREFIX="-l"
    LDFLAGS_IMPORT_SUFFIX=""
    LNFLAGS="-f --symbolic"
    ARFLAGS="cr"
    ARFLAGS_OUT_PREFIX=""
    ARFLAGS_OUT_SUFFIX=""
    ARFLAGS_PATH_PREFIX=""
    ARFLAGS_PATH_SUFFIX=""
    ARFLAGS_LIB=""
    UNZIPFLAGS="-u -o -q -X"
    UNZIPFLAGS_EXTRACT="-d"
    USING_RANLIB=1
    STRIP_FLAGS="-v"
  ;;
  solaris)
    XULRUNNERDIR="/xulrunner"

    _SAVE_LIBS=$LIBS
    LIBS=
    AC_SEARCH_LIBS([pthread_create], [pthread])
    AC_SUBST(PTHREAD_LIBS, $LIBS)
    LIBS=$_SAVE_LIBS

    GST_MAJORMINOR=0.10
    GST_REQS=0.10.1
    GSTPLUG_REQS=0.10.7

    PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.10 >= $GST_REQS gstreamer-plugins-base-0.10 >= $GSTPLUG_REQS)
    PKG_CHECK_MODULES(GTK, gtk+-2.0)
    AC_SUBST(GSTREAMER_CFLAGS)
    GSTREAMER_LIBS="$GSTREAMER_LIBS -lgstinterfaces-$GST_MAJORMINOR -lgstvideo-$GST_MAJORMINOR -lgstaudio-$GST_MAJORMINOR"
    AC_SUBST(GSTREAMER_LIBS)
    AC_SUBST(GTK_CFLAGS)
    AC_SUBST(GTK_LIBS)

    PKG_CHECK_MODULES(GLIB, glib-2.0)
    AC_SUBST(GLIB_CFLAGS)
    AC_SUBST(GLIB_LIBS)

    # The iPod support needs dbus.
    PKG_CHECK_MODULES(DBUS, dbus-glib-1)
    AC_SUBST(DBUS_CFLAGS)
    AC_SUBST(DBUS_LIBS)

    if test "$GNU_CC" = "yes" ; then
      AC_MSG_ERROR([GCC not yet supported])
      LDFLAGS_DLL="-shared"
      LDFLAGS_BIN=""
    else
      # Sun Studio compiler (cc/CC)
      SUN_COMPILER_FLAGS="-c -KPIC"
      if test "$enable_debug" = "yes"; then
        SUN_COMPILER_FLAGS="-g $SUN_COMPILER_FLAGS"
      # Disable stab section for now. Breakpad needs this.
      # else
      # SUN_COMPILER_FLAGS="-xs -xdebugformat=stabs $SUN_COMPILER_FLAGS"
      fi

      CFLAGS="$CFLAGS $SUN_COMPILER_FLAGS"
      CXXFLAGS="$CXXFLAGS $SUN_COMPILER_FLAGS"
      LDFLAGS_DLL="-G -z defs -library=Cstd,Crun -lc"
      LDFLAGS_BIN=""
      USING_RANLIB=1
    fi

    CFLAGS_INCLUDE_PREFIX="-I"
    CFLAGS_INCLUDE_SUFFIX=""
    CFLAGS_PREPROCESS="-E"
    CFLAGS_ASSEMBLER="-S"
    CFLAGS_WARNING_IS_ERROR=""
    LDFLAGS_LIB="-static"
    LDFLAGS_OUT_PREFIX="-o "
    LDFLAGS_OUT_SUFFIX=""
    LDFLAGS_PATH_PREFIX="-L"
    LDFLAGS_PATH_SUFFIX=""
    LDFLAGS_IMPORT_PREFIX="-l"
    LDFLAGS_IMPORT_SUFFIX=""
    LNFLAGS="-f -s"
    ARFLAGS="cr"
    ARFLAGS_OUT_PREFIX=""
    ARFLAGS_OUT_SUFFIX=""
    ARFLAGS_PATH_PREFIX=""
    ARFLAGS_PATH_SUFFIX=""
    ARFLAGS_LIB=""
    UNZIPFLAGS="-u -o -q -X"
    UNZIPFLAGS_EXTRACT="-d"
  ;;
esac

AC_SUBST(ARFLAGS)
AC_SUBST(ARFLAGS_OUT_PREFIX)
AC_SUBST(ARFLAGS_OUT_SUFFIX)
AC_SUBST(ARFLAGS_PATH_PREFIX)
AC_SUBST(ARFLAGS_PATH_SUFFIX)
AC_SUBST(ARFLAGS_LIB)
AC_SUBST(CFLAGS_INCLUDE_PREFIX)
AC_SUBST(CFLAGS_INCLUDE_SUFFIX)
AC_SUBST(_MSC_VER)
AC_SUBST(DEFAULT_MIDL_FLAGS)
AC_SUBST(MSMANIFEST_TOOL)
AC_SUBST(LDFLAGS_DLL)
AC_SUBST(LDFLAGS_BIN)
AC_SUBST(LDFLAGS_LIB)
AC_SUBST(LDFLAGS_OUT_PREFIX)
AC_SUBST(LDFLAGS_OUT_SUFFIX)
AC_SUBST(LDFLAGS_PATH_PREFIX)
AC_SUBST(LDFLAGS_PATH_SUFFIX)
AC_SUBST(LDFLAGS_IMPORT_PREFIX)
AC_SUBST(LDFLAGS_IMPORT_SUFFIX)
AC_SUBST(DEFAULT_LIBS)
AC_SUBST(LNFLAGS)
AC_SUBST(UNZIPFLAGS)
AC_SUBST(UNZIPFLAGS_EXTRACT)
AC_SUBST(USING_RANLIB)
AC_SUBST(MACOSX_APPBUNDLE)
AC_SUBST(MACOSX_CONTENTS)
AC_SUBST(MACOSX_FRAMEWORKS)
AC_SUBST(MACOSX_RESOURCES)
AC_SUBST(XULRUNNERDIR)
AC_SUBST(CMMFLAGS)
AC_SUBST(CFLAGS_STATIC_LIBC)
AC_SUBST(CFLAGS_PREPROCESS)
AC_SUBST(CFLAGS_ASSEMBLER)
AC_SUBST(CFLAGS_WARNING_IS_ERROR)
AC_SUBST(STRIP_FLAGS)
AC_SUBST(COMPILER_GARBAGE)

#
# Check for --with-license-file
#

AC_ARG_WITH(license-file,
            [  --with-license-file=PATH, path to the license file to use (default=installer/LICENSE.html],
            [ SB_LICENSE_FILE=$withval],
            [ SB_LICENSE_FILE="\$(topsrcdir)/$OBJDIRNAME/installer/LICENSE.html" ])
AC_SUBST(SB_LICENSE_FILE)

#
# Check for --with-user-eula-file
#

AC_ARG_WITH(user-eula-file,
            [  --with-user-eula-file=PATH           the location of the user eula file to use. This file
                                       is displayed when the application is first launched.
                                       The user *must* accept this EULA for the application
                                       to continue. The file *must* reside within the source
                                       tree. Using / means the root of the source tree.
                                       ],
            SB_USER_EULA_FILE=$withval)
if test -z "$SB_USER_EULA_FILE"; then
  SB_USER_EULA_FILE=eula.html
fi
AC_SUBST(SB_USER_EULA_FILE)

#
# Check for an alternate dependencies location
#

AC_ARG_WITH(dependencies-dir,
            [  --with-dependencies-dir=DIR          the location of the dependencies directory.],
            SB_DEPENDENCIES_DIR=$withval)
if test -z "$SB_DEPENDENCIES_DIR"; then
  DEPS_SUBDIR=$SB_PLATFORM-$SB_ARCH
  SB_DEPENDENCIES_DIR=dependencies/$DEPS_SUBDIR
fi
AC_SUBST(SB_DEPENDENCIES_DIR)


#
# Are we going to use system or packaged taglib?
#

SB_SYSTEM_TAGLIB=
AC_ARG_WITH(taglib-source,
			[  --with-taglib-source=system,packaged,default  Taglib to use],
			[ SB_TAGLIB=$withval ],
			[ SB_TAGLIB=default ])
for option in `echo $SB_TAGLIB | sed 's/,/ /g'`; do
	if test "$option" = "default"; then
		case "$SB_PLATFORM" in
		windows)
			SB_TAGLIB=packaged
		;;
		macosx)
			SB_TAGLIB=packaged
		;;
		solaris)
			SB_TAGLIB=packaged
		;;
		linux)
			SB_TAGLIB=system
		;;
		*)
			AC_MSG_ERROR([No platform default taglib found; please manually specify --with-taglib-source=])
		;;
		esac
	elif test "$option" = "system"; then
		SB_TAGLIB=system
	elif test "$option" = "packaged"; then
		SB_TAGLIB=packaged
	else
		AC_MSG_ERROR([Unknown --with-taglib-source option $option])
	fi
done
AC_SUBST(SB_TAGLIB)

#	
# If we're building with SYSTEM_TAGLIB, find it
#

if test "$SB_TAGLIB" = "system"; then
	PKG_CHECK_MODULES(TAGLIB, taglib >= 1.8.0)
	AC_SUBST(TAGLIB_LIBS)
	AC_SUBST(TAGLIB_CFLAGS)
fi

#
# Check for media cores to enable
#

MEDIA_CORE_WMP=
MEDIA_CORE_GST=
MEDIA_CORE_GST_SYSTEM=
MEDIA_CORE_QT=

AC_ARG_WITH(media-core,
            [  --with-media-core=windowsmedia,gstreamer,gstreamer-system,qt,default,none  Media cores to enable],
            [ SB_MEDIA_CORES=$withval ],
            [ SB_MEDIA_CORES=default ])
for option in `echo $SB_MEDIA_CORES | sed 's/,/ /g'`; do
    if test "$option" = "default"; then
         case "$SB_PLATFORM" in
            windows)
                MEDIA_CORE_GST=1
                MEDIA_CORE_WMP=1
                MEDIA_CORE_QT=1
            ;;
            macosx)
                # Enable GST only on x86 (we don't have PPC binaries)
                case "$SB_ARCH" in
                    *86)
                        MEDIA_CORE_GST=1
                    ;;
                esac
                MEDIA_CORE_QT=1
            ;;
            linux|solaris)
                MEDIA_CORE_GST=1
            ;;
            *)
                AC_MSG_ERROR([No platform default media cores found; please manually specify --with-media-core=])
            ;;
        esac
    elif test "$option" = "none"; then
        # Nothing needs to be set here
        true
    elif test "$option" = "windowsmedia"; then
        MEDIA_CORE_WMP=1
    elif test "$option" = "gstreamer"; then
        MEDIA_CORE_GST=1
    elif test "$option" = "gstreamer-system"; then
        MEDIA_CORE_GST_SYSTEM=1
    elif test "$option" = "qt"; then
        MEDIA_CORE_QT=1
    else
        AC_MSG_ERROR([Unknown --with-media-core option $option])
    fi

    if test "$MEDIA_CORE_GST" = "1" -a "$MEDIA_CORE_GST_SYSTEM" = "1"; then
        AC_MSG_ERROR([You can not build the gstreamer and gstreamer-system media cores at the same time])
    fi
done
AC_SUBST(MEDIA_CORE_WMP)
AC_SUBST(MEDIA_CORE_GST)
AC_SUBST(MEDIA_CORE_GST_SYSTEM)
AC_SUBST(MEDIA_CORE_QT)

FORCE_MEDIA_CORE_WMP=
FORCE_MEDIA_CORE_QT=

AC_ARG_WITH(force-media-core,
            [  --with-force-media-core=windowsmedia,gstreamer,qt,default,none  Media cores to force install and enable],
            [ SB_FORCED_MEDIA_CORES=$withval ],
            [ SB_FORCED_MEDIA_CORES=default ])
for option in `echo $SB_FORCED_MEDIA_CORES | sed 's/,/ /g'`; do
    if test "$option" = "default"; then
        # Nothing needs to be set here
        true
    elif test "$option" = "none"; then
        # Nothing needs to be set here
        true
    elif test "$option" = "windowsmedia"; then
        FORCE_MEDIA_CORE_WMP=1
    elif test "$option" = "qt"; then
        FORCE_MEDIA_CORE_QT=1
    else
        AC_MSG_ERROR([Unknown --with-force-media-core option $option])
    fi
done
AC_SUBST(FORCE_MEDIA_CORE_WMP)
AC_SUBST(FORCE_MEDIA_CORE_QT)

#
# If we're building with MEDIA_CORE_GST_SYSTEM, find our required packages
#

if test "$MEDIA_CORE_GST_SYSTEM" = "1"; then

  GST_MAJORMINOR=0.10
  GST_REQS=0.10.1
  GSTPLUG_REQS=0.10.7

  PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.10 >= $GST_REQS gstreamer-plugins-base-0.10 >= $GSTPLUG_REQS)
  AC_SUBST(GSTREAMER_CFLAGS)

  GSTREAMER_LIBS="$GSTREAMER_LIBS -lgstinterfaces-$GST_MAJORMINOR -lgstvideo-$GST_MAJORMINOR -lgstaudio-$GST_MAJORMINOR"
  AC_SUBST(GSTREAMER_LIBS)

fi

#
# Check for --with-searchplugins
#

SB_SEARCHPLUGINS=

AC_ARG_WITH(searchplugins,
            [  --with-searchplugins=<list of plugin files>,default   Searchplugins to include (from the locales)],
            [ SB_SEARCHPLUGINS=$withval ],
            [ SB_SEARCHPLUGINS=default ])
AC_SUBST(SB_SEARCHPLUGINS)

# If our media core is gstreamer, and we're on linux, we may or may not
# have this file. Warn if we expect it but don't find it.
if test "$MEDIA_CORE_GST" = "1" -a "$SB_PLATFORM" = "linux"; then
  AC_CHECK_FILE(${srcdir}/dependencies/$SB_PLATFORM-$SB_ARCH/private/fluendo-mp3/libgstflump3dec.so,
      [HAVE_FLUENDO_MP3=1],
      [AC_MSG_WARN([Linux MP3 decoder not present so build will not play mp3.])]
  )
fi
AC_SUBST(HAVE_FLUENDO_MP3)

#
# Checks for libraries.
#

AC_CHECK_LIB([nspr4], [PR_Cleanup])
AC_CHECK_LIB([xul], [_XRE_Main])

#
# Use the mozilla perl method for generating makefiles quickly. This script
#   only replaces @top_srcdir@ and @srcdir@ in makefiles. If any other vars
#   are used then it passes them on to config.status via the $MAKEFILES var.
#

. ${srcdir}/allmakefiles.sh
echo $MAKEFILES | ${PERL} $srcdir/$SB_DEPENDENCIES_DIR/mozilla-9.0.1/$SB_CONFIGURATION/scripts/acoutput-fast.pl --srcdir=${srcdir} > ./conftest.sh
. ./conftest.sh
rm -f ./conftest.sh

#
# Generate all remaining makefiles
#

AC_OUTPUT($MAKEFILES)
