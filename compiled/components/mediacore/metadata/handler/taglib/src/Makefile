#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://www.songbirdnest.com
# 
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the "GPL").
# 
# Software distributed under the License is distributed 
# on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
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

DEPTH = ../../../../../../
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/mediacore/metadata/handler/taglib/src
VPATH = /media/disk/Linux/makepkg/songbird/components/mediacore/metadata/handler/taglib/src

include $(DEPTH)/build/autodefs.mk

DYNAMIC_LIB = sbMetadataHandlerTaglib

CPP_SRCS = MetadataHandlerTaglib.cpp \
           MetadataHandlerTaglibComponent.cpp \
           SeekableChannel.cpp \
           TaglibChannelFileIO.cpp \
           TaglibChannelFileIOManager.cpp \
           $(NULL)

CPP_EXTRA_INCLUDES = \
 $(DEPTH)/components/mediacore/metadata/manager/public \
 $(DEPTH)/components/mediacore/metadata/handler/taglib/public \
 $(DEPTH)/components/albumart/public \
 $(DEPTH)/components/library/base/public \
 $(DEPTH)/components/property/public \
 $(DEPTH)/components/moz/proxy/public \
 $(topsrcdir)/components/mediacore/metadata/manager/src \
 $(topsrcdir)/components/moz/strings/src \
 $(topsrcdir)/components/moz/threads/src \
 $(topsrcdir)/components/property/src \
 $(topsrcdir)/components/include \
 $(srcdir) \
 $(MOZSDK_INCLUDE_DIR)/crashreporter \
 $(MOZSDK_INCLUDE_DIR)/necko \
 $(MOZSDK_INCLUDE_DIR)/mimetype \
 $(MOZSDK_INCLUDE_DIR)/unicharutil \
 $(MOZSDK_INCLUDE_DIR)/chardet \
 $(MOZSDK_INCLUDE_DIR)/uconv \
 $(MOZSDK_IDL_DIR) \
 $(NULL)

CPP_EXTRA_DEFS = -DTAGLIB_STATIC \
                 $(NULL)

DYNAMIC_LIB_EXTRA_IMPORTS = unicharutil_external_s \
                            plds4 \
                            $(NULL)

DYNAMIC_LIB_STATIC_IMPORTS += \
 components/moz/strings/src/sbMozStringUtils \
 components/moz/threads/src/sbMozThreads \
 $(DEPS_DIR)/taglib/$(SB_CONFIGURATION)/lib/$(LIB_PREFIX)tag$(LIB_SUFFIX) \
 $(NULL)

ifeq (windows,$(SB_PLATFORM))
   DYNAMIC_LIB_STATIC_IMPORTS += \
    $(DEPS_DIR)/zlib/$(SB_CONFIGURATION)/lib/zlib$(LIB_SUFFIX) \
    $(NULL)
else 
   DYNAMIC_LIB_EXTRA_IMPORTS += z
endif

ifeq (linux,$(SB_PLATFORM))
   # Search in xulrunner directory at runtime for shared libraries.
   DYNAMIC_LIB_EXTRA_FLAGS += -Wl,-rpath,\$$ORIGIN/../xulrunner
endif

ifeq (solaris,$(SB_PLATFORM))
   # Search in xulrunner directory at runtime for shared libraries.
   DYNAMIC_LIB_EXTRA_FLAGS += -R'$$ORIGIN/../xulrunner'
endif

ifeq (,$(TAGLIB_CFLAGS))
CPP_EXTRA_INCLUDES += $(DEPS_DIR)/taglib/$(SB_CONFIGURATION)/include/taglib \
                      $(NULL)
else
CPP_EXTRA_INCLUDES += $(TAGLIB_CFLAGS)
endif
        
IS_COMPONENT = 1                     

include $(topsrcdir)/build/rules.mk
