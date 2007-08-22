#
# BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2007 POTI, Inc.
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

AC_PREREQ(2.56)
AC_INIT(build/rules.mk)
AC_CONFIG_AUX_DIR(${srcdir}/build/autoconf)


#
# We're going to hardcode our obj and dist directory names for the moment.
# These MUST match the vars in songbird.mk or bad things will happen!
#

OBJDIRNAME="compiled"
DISTDIRNAME="dist"

AC_SUBST(OBJDIRNAME)
AC_SUBST(DISTDIRNAME)

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
# Check for --enable-jars
#

AC_ARG_ENABLE(jars,
              [  --enable-jars           whether or not to create jar files (enabled by default)])
if test "$enable_jars" == "yes"; then
  FORCE_JARS=1
fi
if test "$enable_jars" == "no"; then
  PREVENT_JARS=1
fi
AC_SUBST(FORCE_JARS)
AC_SUBST(PREVENT_JARS)

#
#  Check for --enable-installer
#

AC_ARG_ENABLE(installer,
              [  --enable-installer      whether or not to create installer (disabled by default)])
if test "$enable_installer" == "yes"; then
  MAKE_INSTALLER=1
fi
AC_SUBST(MAKE_INSTALLER)

#
#  Check for --enable-official
#

AC_ARG_ENABLE(official,
              [  --enable-official       whether or not to create an official build (disabled by default)])
if test "$enable_official" == "yes"; then
  SONGBIRD_OFFICIAL=1
fi
AC_SUBST(SONGBIRD_OFFICIAL)

#
#  Check for --enable-tests
#

AC_ARG_ENABLE(tests,
              [  --enable-tests      whether or not to build tests (defaults: debug:enabled rel:disabled)])
if test "$enable_tests" == "yes"; then
  SB_ENABLE_TESTS=1
fi
AC_SUBST(SB_ENABLE_TESTS)

#
# Check for --enable-update-channel
#

AC_ARG_ENABLE(update-channel,
              [  --enable-update-channel=CHANNEL      which update channel to use ("default")],
              SB_UPDATE_CHANNEL=`echo $enableval | tr A-Z a-z`)
if test -z "$SB_UPDATE_CHANNEL"; then
  SB_UPDATE_CHANNEL=default
fi
AC_SUBST(SB_UPDATE_CHANNEL)

#
# Check platform
#

AC_CANONICAL_SYSTEM
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
    # Add the Cygwin wrapper if necessary
    case "$target" in
      *-mingw*)
        # no cygwin wrapper for mingw
      ;;
      *)
        CYGWIN_WRAPPER="${srcdir}/dependencies/$SB_PLATFORM-$SB_ARCH/mozilla/$SB_CONFIGURATION/scripts/cygwin-wrapper"
        if test "`echo ${srcdir} | grep -c ^/ 2>/dev/null`" = 0; then
          _pwd=`pwd`
          CYGWIN_WRAPPER="${_pwd}/${srcdir}/dependencies/$SB_PLATFORM-$SB_ARCH/mozilla/$SB_CONFIGURATION/scripts/cygwin-wrapper"
        fi
        CYGDRIVE_MOUNT=`mount -p | awk '{ if (/^\//) { print $1; exit } }'`
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
      *)
        AC_MSG_ERROR(Unsupported architecture)
      ;;
    esac
  ;;
esac
AC_SUBST(SB_PLATFORM)
AC_SUBST(SB_ARCH)
AC_SUBST(CYGWIN_WRAPPER)
AC_SUBST(CYGDRIVE_MOUNT)

#
# Breakpad support
#

AC_ARG_ENABLE(breakpad,
              [  --enable-breakpad       whether or not to enable breakpad (disabled by default in non-official builds)])
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
# Checks for programs.
#

# Don't let autoconf hijack our flags
_SAVE_CFLAGS=$CFLAGS
_SAVE_CXXFLAGS=$CXXFLAGS

case "$SB_PLATFORM" in
  windows)
  AC_CHECK_PROG(CC, cl, cl)
  AC_CHECK_PROG(CXX, cl, cl)
  ;;
  *)
  AC_PROG_CC
  AC_PROG_CXX
  ;;
esac

# Restore saved flags
CFLAGS=$_SAVE_CFLAGS
CXXFLAGS=$_SAVE_CXXFLAGS

AC_CHECK_PROG(PERL, perl, perl)
if test -z "$PERL"; then AC_MSG_ERROR(Perl not found); fi

AC_CHECK_PROG(PYTHON, python, python)
if test -z "$PYTHON"; then AC_MSG_ERROR(Python not found); fi

AC_CHECK_PROG(ZIP, zip, zip)
if test -z "$ZIP"; then AC_MSG_ERROR(Zip not found); fi

AC_CHECK_PROG(UNZIP, unzip, unzip)
if test -z "$UNZIP"; then AC_MSG_ERROR(Unzip not found); fi

AC_CHECK_PROG(LN, ln, ln)
if test -z "$LN"; then AC_MSG_ERROR(Ln not found); fi

AC_CHECK_PROG(FIND, find, find)
if test -z "$FIND"; then AC_MSG_ERROR(Find not found); fi

AC_CHECK_PROG(MV, mv, mv)
if test -z "$MV"; then AC_MSG_ERROR(Mv not found); fi

AC_CHECK_PROG(CHMOD, chmod, chmod)
if test -z "$CHMOD"; then AC_MSG_ERROR(Chmod not found); fi

AC_CHECK_PROG(RM, rm, rm)
if test -z "$RM"; then AC_MSG_ERROR(Rm not found); fi

AC_CHECK_PROG(MKDIR, mkdir, mkdir)
if test -z "$MKDIR"; then AC_MSG_ERROR(Mkdir not found); fi

AC_CHECK_PROG(AWK, awk, awk)
if test -z "$AWK"; then AC_MSG_ERROR(Awk not found); fi

AC_CHECK_PROGS(CP, ${CP} gnucp gcp cp, "")
if test -z "$CP"; then AC_MSG_ERROR(Cp not found); fi

AC_CHECK_PROG(GZIP, gzip, gzip)
AC_CHECK_PROG(GUNZIP, gunzip, gunzip)
AC_CHECK_PROG(TAR, tar, tar)

AC_CHECK_PROGS(SHA1SUM, ${SHA1SUM} gsha1sum sha1sum, "")

case "$SB_PLATFORM" in
  windows)
    AC_CHECK_PROG(LD, link, link)
    if test -z "$LD"; then AC_MSG_ERROR(Linker not found); fi
    AC_CHECK_PROG(AS, ml, ml)
    if test -z "$AS"; then AC_MSG_ERROR(Assembler not found); fi
    AC_CHECK_PROG(AR, lib, lib)
    if test -z "$AR"; then AC_MSG_ERROR(Librarian not found); fi
    AC_CHECK_PROG(RC, rc, rc)
    if test -z "$RC"; then AC_MSG_ERROR(Rc not found); fi
  ;;
  *)
    AC_CHECK_PROG(LD, ${CC}, ${CC})
    if test -z "$LD"; then AC_MSG_ERROR(Linker not found); fi
    AC_CHECK_PROG(AS, as, as)
    if test -z "$AS"; then AC_MSG_ERROR(Assembler not found); fi
    AC_CHECK_PROG(AR, ar, ar)
    if test -z "$AR"; then AC_MSG_ERROR(Archiver not found); fi
    AC_CHECK_PROG(RANLIB, ranlib, ranlib)
    if test -z "$RANLIB"; then AC_MSG_ERROR(Ranlib not found); fi
    if test -z "$GZIP"; then AC_MSG_ERROR(Gzip not found); fi
    if test -z "$GUNZIP"; then AC_MSG_ERROR(Gunzip not found); fi
    if test -z "$TAR"; then AC_MSG_ERROR(Tar not found); fi
  ;;
esac
#
# Set platform-specific defines
#

case "$SB_PLATFORM" in
  windows)
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
    AC_DEFINE(NO_X11)
  ;;
  linux)
    BIN_SUFFIX=
    OBJ_SUFFIX=".o"
    LIB_SUFFIX=".a"
    DLL_SUFFIX=".so"
    AC_DEFINE(XP_UNIX)
    AC_DEFINE(_REENTRANT)
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
  windows)
    # We use MSVC on Windows

    changequote(,)
    _MSVC_VER_FILTER='s|.* \([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*|\1|p'
    changequote([,])

    # Determine compiler version
    CC_VERSION=`"${CC}" -v 2>&1 | sed -ne "$_MSVC_VER_FILTER"`
    _CC_MAJOR_VERSION=`echo ${CC_VERSION} | $AWK -F\. '{ print $1 }'`
    _CC_MINOR_VERSION=`echo ${CC_VERSION} | $AWK -F\. '{ print $2 }'`
    _MSC_VER=${_CC_MAJOR_VERSION}${_CC_MINOR_VERSION}

    CXX_VERSION=`"${CXX}" -v 2>&1 | sed -ne "$_MSVC_VER_FILTER"`
    _CXX_MAJOR_VERSION=`echo ${CXX_VERSION} | $AWK -F\. '{ print $1 }'`

    if test "$_CC_MAJOR_VERSION" != "$_CXX_MAJOR_VERSION"; then
        AC_MSG_ERROR([The major versions of \$CC and \$CXX do not match.])
    fi
    if test "$_CC_MAJOR_VERSION" = "12"; then
        _CC_SUITE=6
    elif test "$_CC_MAJOR_VERSION" = "13"; then
        _CC_SUITE=7
    elif test "$_CC_MAJOR_VERSION" = "14"; then
        _CC_SUITE=8
    else
        AC_MSG_ERROR([This version of the MSVC compiler, $CC_VERSION , is unsupported.])
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
                    MSMANIFEST_TOOL=1
                    unset MSMT_TOOL
            else
                    AC_MSG_ERROR([Microsoft (R) Manifest Tool must be in your \$PATH.])
            fi
    fi

    # Check linker version
    _LD_FULL_VERSION=`"${LD}" -v 2>&1 | sed -ne "$_MSVC_VER_FILTER"`
    _LD_MAJOR_VERSION=`echo ${_LD_FULL_VERSION} | $AWK -F\. '{ print $1 }'`
    if test "$_LD_MAJOR_VERSION" != "$_CC_SUITE"; then
        AC_MSG_ERROR([The linker major version, $_LD_FULL_VERSION,  does not match the compiler suite version, $_CC_SUITE.])
    fi

    MSVC_COMPILER_FLAGS="-nologo -W3 -WL -GS -Wp64 -EHsc -Zi -c"

    if test "$_CC_SUITE" = "8"; then
      # MSVC 8+ use native wchar_t, but Mozilla builds without it
      MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -Zc:wchar_t-"
    fi

    if test "$enable_debug" = "yes"; then
      MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -MTd -Od -RTC1 -RTCc"
    else
      MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -MT -Ox"
    fi

    # We always want to include "mozilla-config.h"
    MSVC_COMPILER_FLAGS="$MSVC_COMPILER_FLAGS -FI \"mozilla-config.h\" -FI \"xpcom/xpcom-config.h\""

    MSVC_LINKER_FLAGS="-NOLOGO -INCREMENTAL:NO -SUBSYSTEM:WINDOWS -MACHINE:X86 -DEBUG"
    if test "$enable_debug" != "yes"; then
      MSVC_LINKER_FLAGS="$MSVC_LINKER_FLAGS -OPT:REF -OPT:ICF"
    fi

    CFLAGS="$CFLAGS $MSVC_COMPILER_FLAGS -TC"
    CXXFLAGS="$CXXFLAGS $MSVC_COMPILER_FLAGS -TP"
    CFLAGS_INCLUDE_PREFIX="-I"
    CFLAGS_INCLUDE_SUFFIX=""
    LDFLAGS="$LDFLAGS $MSVC_LINKER_FLAGS"
    LDFLAGS_DLL="-DLL"
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
  ;;
  macosx)
    MACOSX_APPBUNDLE="Songbird.app"
    MACOSX_CONTENTS="/$MACOSX_APPBUNDLE/Contents"
    MACOSX_RESOURCES="$MACOSX_CONTENTS/Resources"
    MACOSX_FRAMEWORKS="$MACOSX_CONTENTS/Frameworks"
    XULRUNNERDIR="/XUL.framework"

    GCC_COMPILER_FLAGS="-gstabs+ -c -fexceptions -fnon-call-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-common -fpascal-strings -fno-rtti -no-cpp-precomp -Wall -Wconversion -Wpointer-arith -Wcast-align -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor -Wno-long-long -fshort-wchar -pipe"
    if test "$enable_debug" = "yes"; then
      GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -fno-inline -O1"
    else
      # Use -O2 on OS X, better than -Os according to moz perf tests
      GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -O2"
    fi

    # We always want to include "mozilla-config.h"
    GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -include \"mozilla-config.h\""

    GCC_LINKER_FLAGS="-lpthread -lobjc -shared-libgcc -lstdc++ -Wl,-executable_path,\$(topsrcdir)/$OBJDIRNAME/$DISTDIRNAME$MACOSX_FRAMEWORKS$XULRUNNERDIR"

    CFLAGS="$CFLAGS $GCC_COMPILER_FLAGS"
    CXXFLAGS="$CXXFLAGS $GCC_COMPILER_FLAGS"
    CFLAGS_INCLUDE_PREFIX="-I"
    CFLAGS_INCLUDE_SUFFIX=""
    LDFLAGS="$LDFLAGS $GCC_LINKER_FLAGS"
    LDFLAGS_DLL="-dynamiclib"
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
  ;;
  linux)
    XULRUNNERDIR="/xulrunner"

    # Our GStreamer component needs GTK headers (we don't link, so we only
    #   take the flags). We only check for gtk2 right now, if we _really_
    #    need gtk1 support we can add it later but we should resist at all costs.
    PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.10)
    PKG_CHECK_MODULES(GTK, gtk+-2.0)
    AC_SUBST(GSTREAMER_CFLAGS)
    AC_SUBST(GTK_CFLAGS)

    # Breakpad needs stabs debugging data, so always explicitly request it with
    # -gstabs+
    GCC_COMPILER_FLAGS="-c -gstabs+ -fPIC -fshort-wchar -fexceptions -fnon-call-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -Wall -Wconversion -Wpointer-arith -Wcast-align -Wno-long-long -pipe -pthread"
    if test "$enable_debug" = "yes"; then
      GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -fno-inline -O1"
    else
      # Use -Os on linux, better than -O2 according to moz perf tests
      GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -Os"
    fi

    # We always want to include "mozilla-config.h"
    GCC_COMPILER_FLAGS="$GCC_COMPILER_FLAGS -include \"mozilla-config.h\""

    GCC_LINKER_FLAGS="-Wl,-rpath-link,\$(topsrcdir)/$OBJDIRNAME/$DISTDIRNAME$XULRUNNERDIR"

    CFLAGS="$CFLAGS $GCC_COMPILER_FLAGS"
    CXXFLAGS="$CXXFLAGS $GCC_COMPILER_FLAGS -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor"
    CFLAGS_INCLUDE_PREFIX="-I"
    CFLAGS_INCLUDE_SUFFIX=""
    LDFLAGS="$LDFLAGS $GCC_LINKER_FLAGS"
    LDFLAGS_DLL="-shared"
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
AC_SUBST(MSMANIFEST_TOOL)
AC_SUBST(LDFLAGS_DLL)
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


#
# Check for an alternate dependencies location
#

AC_ARG_WITH(dependencies-dir,
            [  --with-dependencies-dir=DIR          the location of the dependencies directory],
            SB_DEPENDENCIES_DIR=$withval)
if test -z "$SB_DEPENDENCIES_DIR"; then
  DEPS_SUBDIR=$SB_PLATFORM-$SB_ARCH
  if test "$SB_PLATFORM" = "windows" -a "$_CC_SUITE" = "8" ; then
    # since the default dependencies for Windows are for MSVC 7.1,
    # we need to special-case for MSVC 8 to use its (incompatible) version instead
    DEPS_SUBDIR=$DEPS_SUBDIR-msvc8
  fi
  SB_DEPENDENCIES_DIR=dependencies/$DEPS_SUBDIR
fi
AC_SUBST(SB_DEPENDENCIES_DIR)

#
# Check for media cores to enable
#
AC_ARG_WITH(media-core,
            [  --with-media-core=vlc,windowsmedia,gstreamer,qt,default,none  Media cores to enable],
            [ SB_MEDIA_CORES=$withval ],
            [ SB_MEDIA_CORES=default ])
for option in `echo $SB_MEDIA_CORES | sed 's/,/ /g'`; do
    if test "$option" = "default"; then
        MEDIA_CORE_VLC=1
        MEDIA_CORE_WMP=1
        MEDIA_CORE_GST=1
        MEDIA_CORE_QT=1
    elif test "$option" = "none"; then
        MEDIA_CORE_VLC=
        MEDIA_CORE_WMP=
        MEDIA_CORE_GST=
        MEDIA_CORE_QT=
    elif test "$option" = "vlc"; then
        MEDIA_CORE_VLC=1
    elif test "$option" = "windowsmedia"; then
        MEDIA_CORE_WMP=1
    elif test "$option" = "gstreamer"; then
        MEDIA_CORE_GST=1
    elif test "$option" = "qt"; then
        MEDIA_CORE_QT=1
    else
        AC_MSG_ERROR([Unknown --with-core-wrappers option $option])
    fi
done
AC_SUBST(MEDIA_CORE_VLC)
AC_SUBST(MEDIA_CORE_WMP)
AC_SUBST(MEDIA_CORE_GST)
AC_SUBST(MEDIA_CORE_QT)

#
# Check for extras.
#

if test -d ${srcdir}/extras; then
  HAS_EXTRAS=1
fi
AC_SUBST(HAS_EXTRAS)

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
echo $MAKEFILES | ${PERL} $srcdir/$SB_DEPENDENCIES_DIR/mozilla/$SB_CONFIGURATION/scripts/acoutput-fast.pl --srcdir=${srcdir} > ./conftest.sh
. ./conftest.sh
rm -f ./conftest.sh

#
# Generate all remaining makefiles
#

AC_OUTPUT($MAKEFILES)
