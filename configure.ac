#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright 2006 Pioneers of the Inevitable LLC
# http://www.songbirdnest.com
# 
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the GPL).
# 
# Software distributed under the License is distributed 
# on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
# express or implied. See the GPL for the specific language 
# governing rights and limitations.
#
# You should have received a copy of the GPL along with this 
# program. If not, go to http://www.gnu.org/licenses/gpl.html
# or write to the Free Software Foundation, Inc., 
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# 
# END SONGBIRD GPL
#

#
# Process this file with autoconf to produce a configure script.
#

AC_PREREQ(2.59)
AC_INIT(build/rules.mk)
AC_CONFIG_AUX_DIR(${srcdir}/build/autoconf)

#
# Check platform
#

AC_CANONICAL_SYSTEM
case "$target" in
  *-cygwin*|*-mingw*|*-msvc*|*-mks*)
    SB_PLATFORM="win32"
    # Add the Cygwin wrapper if necessary
    case "$target" in
      *-mingw*)
        # no cygwin wrapper for mingw
      ;;  
      *)
        CYGWIN_WRAPPER="${srcdir}/build/cygwin-wrapper"
        if test "`echo ${srcdir} | grep -c ^/ 2>/dev/null`" = 0; then
          _pwd=`pwd`
          CYGWIN_WRAPPER="${_pwd}/${srcdir}/build/cygwin-wrapper"
        fi
        CYGDRIVE_MOUNT=`mount -p | awk '{ if (/^\//) { print $1; exit } }'`
      ;;
    esac
  ;;
  *darwin*)
    SB_PLATFORM="macosx"
  ;;
  *linux*)
    SB_PLATFORM="linux"
  ;;
esac
AC_SUBST(SB_PLATFORM)
AC_SUBST(CYGWIN_WRAPPER)
AC_SUBST(CYGDRIVE_MOUNT)

#
# Checks for programs.
#

AC_PROG_CC
AC_PROG_CXX

AC_CHECK_PROG(PERL, perl, perl)
if test -z "$PERL"; then AC_MSG_ERROR(Perl not found); fi

AC_CHECK_PROG(ZIP, zip, zip)
if test -z "$ZIP"; then AC_MSG_ERROR(Zip not found); fi

AC_CHECK_PROG(UNZIP, unzip, unzip)
if test -z "$UNZIP"; then AC_MSG_ERROR(Unzip not found); fi

AC_CHECK_PROG(LN, ln, ln)
if test -z "$LN"; then AC_MSG_ERROR(Ln not found); fi

case "$SB_PLATFORM" in
  win32)
    AC_CHECK_PROG(LD, link, link)
    if test -z "$LD"; then AC_MSG_ERROR(Linker not found); fi
    AC_CHECK_PROG(AS, ml, ml)
    if test -z "$AS"; then AC_MSG_ERROR(Assembler not found); fi
    AC_CHECK_PROG(AR, lib, lib)
    if test -z "$AR"; then AC_MSG_ERROR(Librarian not found); fi
  ;;
  *)
    AC_CHECK_PROG(LD, ld, ld)
    if test -z "$LD"; then AC_MSG_ERROR(Linker not found); fi
    AC_CHECK_PROG(AS, as, as)
    if test -z "$AS"; then AC_MSG_ERROR(Assembler not found); fi
    AC_CHECK_PROG(AR, ar, ar)
		if test -z "$AR"; then AC_MSG_ERROR(Archiver not found); fi
		AC_CHECK_PROG(RANLIB, ranlib, ranlib)
		if test -z "$RANLIB"; then AC_MSG_ERROR(Ranlib not found); fi
		USING_RANLIB=1
  ;;
esac
AC_SUBST(USING_RANLIB)

#
# Checks for libraries.
#

AC_CHECK_LIB([nspr4], [PR_Cleanup])
AC_CHECK_LIB([xul], [_XRE_Main])

#
# Checks for header files.
#

#AC_PATH_X
#AC_HEADER_DIRENT
#AC_HEADER_STDC
#AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h malloc.h memory.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h wchar.h])

#
# Checks for typedefs, structures, and compiler characteristics.
#

#AC_HEADER_STAT
#AC_HEADER_STDBOOL
#AC_C_CONST
#AC_TYPE_UID_T
#AC_C_INLINE
#AC_TYPE_MODE_T
#AC_TYPE_OFF_T
#AC_TYPE_PID_T
#AC_TYPE_SIZE_T
#AC_CHECK_MEMBERS([struct stat.st_blksize])
#AC_STRUCT_ST_BLOCKS
#AC_CHECK_MEMBERS([struct stat.st_rdev])
#AC_HEADER_TIME
#AC_STRUCT_TM
#AC_C_VOLATILE
#AC_CHECK_TYPES([ptrdiff_t])

#
# Checks for library functions.
#

#AC_FUNC_ERROR_AT_LINE
#AC_PROG_GCC_TRADITIONAL
#AC_FUNC_LSTAT
#AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
#AC_FUNC_MALLOC
#AC_FUNC_MEMCMP
#AC_FUNC_MKTIME
#AC_FUNC_MMAP
#AC_FUNC_REALLOC
#AC_FUNC_SELECT_ARGTYPES
#AC_FUNC_SETVBUF_REVERSED
#AC_TYPE_SIGNAL
#AC_FUNC_STAT
#AC_FUNC_STRCOLL
#AC_FUNC_STRFTIME
#AC_FUNC_VPRINTF
#AC_CHECK_FUNCS([ftruncate getcwd gethostbyaddr gethostname getpagesize gettimeofday memchr memmove memset munmap putenv select socket strcasecmp strdup strncasecmp strndup strstr strtol])

#
# Check for --enable-debug
#

AC_ARG_ENABLE(debug,
              [  --enable-debug          whether or not to compile a debug version (disabled by default)])
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
# Generic defines
#

AC_DEFINE(MOZILLA_INTERNAL_API)

#
# Set platform-specific defines
#

case "$SB_PLATFORM" in
  win32)
    BIN_SUFFIX=".exe"
    OBJ_SUFFIX=".obj"
    LIB_SUFFIX=".lib"
    DLL_SUFFIX=".dll"
    AC_DEFINE(UNICODE)
    AC_DEFINE(_UNICODE)
    AC_DEFINE(_WINDOWS)
    AC_DEFINE(_WIN32)
    AC_DEFINE(WIN32)
    AC_DEFINE(XP_WIN)
    AC_DEFINE(XP_WIN32)
    AC_DEFINE(WIN32_LEAN_AND_MEAN)
  ;;
  macosx)
    BIN_SUFFIX=
    OBJ_SUFFIX=".o"
    LIB_SUFFIX=".a"
    DLL_SUFFIX=".dylib"
    AC_DEFINE(XP_MACOSX)
    AC_DEFINE(XP_UNIX)
  ;;
  linux)
    BIN_SUFFIX=
    OBJ_SUFFIX=".o"
    LIB_SUFFIX=".a"
    DLL_SUFFIX=".so"
    AC_DEFINE(XP_UNIX)
  ;;
esac
AC_SUBST(BIN_SUFFIX)
AC_SUBST(OBJ_SUFFIX)
AC_SUBST(LIB_SUFFIX)
AC_SUBST(DLL_SUFFIX)

#
# Set platform-specific flags
#

case "$SB_PLATFORM" in
  win32)
    # We use MSVC on Windows
    
    MSVC_COMPILER_FLAGS="-nologo -W3 -WL -GS -Wp64 -EHsc -Zi -c"
    if test "$enable_debug" = "yes"; then
      MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -MDd -Od"
    else
      MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -MD -Ox"
    fi

    # We always want to include "mozilla-config.h"
    MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -FI \"mozilla-config.h\""
   
    MSVC_LINKER_FLAGS="-NOLOGO -INCREMENTAL:NO -SUBSYSTEM:WINDOWS -MACHINE:X86 -DEBUG"
    if test "$enable_debug" != "yes"; then
      MSVC_LINKER_FLAGS="$MSVC_LINKER_FLAGS -OPT:REF -OPT:ICF"
    fi

    CFLAGS="$CFLAGS $MSVC_COMPILER_FLAGS -TC"
    CXXFLAGS="$CXXFLAGS $MSVC_COMPILER_FLAGS -TP"
    CFLAGS_INCLUDE_PREFIX="/I"
    CFLAGS_INCLUDE_SUFFIX=""
    LDFLAGS="$LDFLAGS $MSVC_LINKER_FLAGS"
    LDFLAGS_DLL="/DLL"
    LDFLAGS_LIB=""
    LDFLAGS_OUT_PREFIX="/OUT:\""
    LDFLAGS_OUT_SUFFIX="\""
    LDFLAGS_PATH_PREFIX="/LIBPATH:\""
    LDFLAGS_PATH_SUFFIX="\""
    LDFLAGS_IMPORT_PREFIX=""
    LDFLAGS_IMPORT_SUFFIX="${LIB_SUFFIX}"
    ARFLAGS="/NOLOGO"
    ARFLAGS_OUT_PREFIX="/OUT:\""
    ARFLAGS_OUT_SUFFIX="\""
    ARFLAGS_PATH_PREFIX=""
    ARFLAGS_PATH_SUFFIX=""
    ARFLAGS_LIB=""
    DEFAULT_LIBS="kernel32 user32 gdi32 winmm wsock32 advapi32 comctl32"
  ;;
  macosx)
  ;;
  linux)
    GCC_COMPILER_FLAGS="-c -fPIC -Wall -Wconversion -Wpointer-arith -Wcast-align -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor -Wno-long-long -pedantic -fshort-wchar -pthread -pipe"
    if test "$enable_debug" = "yes"; then
      GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -g -fno-inline"
    else
      GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -O"
    fi

    # We always want to include "mozilla-config.h"
    GCC_COMPILE_FLAGS="$GCC_COMPILE_FLAGS -include \"mozilla-config.h\""

    GCC_LINKER_FLAGS=""

    CFLAGS="$CFLAGS $GCC_COMPILER_FLAGS"
    CXXFLAGS="$CXXFLAGS $GCC_COMPILER_FLAGS"
    CFLAGS_INCLUDE_PREFIX="-I"
    CFLAGS_INCLUDE_SUFFIX=""
    LDFLAGS="$LDFLAGS $GCC_LINKER_FLAGS"
    LDFLAGS_DLL="-shared"
    LDFLAGS_LIB="-static"
    LDFLAGS_OUT_PREFIX="-o"
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
AC_SUBST(LDFLAGS_DLL)
AC_SUBST(LDFLAGS_LIB)
AC_SUBST(LDFLAGS_OUT_PREFIX)
AC_SUBST(LDFLAGS_OUT_SUFFIX)
AC_SUBST(LDFLAGS_PATH_PREFIX)
AC_SUBST(LDFLAGS_PATH_SUFFIX)
AC_SUBST(LDFLAGS_IMPORT_PREFIX)
AC_SUBST(LDFLAGS_IMPORT_SUFFIX)
AC_SUBST(LNFLAGS)
AC_SUBST(DEFAULT_LIBS)

#
# We're going to hardcode our obj and dist directory names for the moment.
# These MUST match the vars in songbird.mk or bad things will happen!
#

OBJDIRNAME="compiled"
DISTDIRNAME="dist"

AC_SUBST(OBJDIRNAME)
AC_SUBST(DISTDIRNAME)

#
# Use the mozilla perl method for generating makefiles quickly. This script
#   only replaces @top_srcdir@ and @srcdir@ in makefiles. If any other vars
#   are used then it passes them on to config.status via the $MAKEFILES var.
#

. ${srcdir}/allmakefiles.sh
echo $MAKEFILES | ${PERL} $srcdir/build/autoconf/acoutput-fast.pl > ./conftest.sh
. ./conftest.sh
rm -f ./conftest.sh

#
# Generate all remaining makefiles
#

AC_OUTPUT($MAKEFILES)

