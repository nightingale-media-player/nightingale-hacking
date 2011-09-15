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

DEPTH = ../../../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/mediacore/gstreamer/src/metadata
VPATH = /media/disk/Linux/makepkg/songbird/components/mediacore/gstreamer/src/metadata

include $(DEPTH)/build/autodefs.mk

STATIC_LIB = sbMetadataGstreamer

CPP_SRCS = sbGStreamerMetadataHandler.cpp \
           $(NULL)

CPP_EXTRA_INCLUDES = $(topsrcdir)/components/include \
                     $(DEPTH)/components/mediacore/metadata/manager/public \
                     $(topsrcdir)/components/mediacore/base/src \
                     $(DEPTH)/components/property/public \
                     $(topsrcdir)/components/property/src \
                     $(DEPTH)/components/mediacore/base/public \
                     $(DEPTH)/components/mediacore/gstreamer/public \
                     $(topsrcdir)/components/mediacore/gstreamer/src \
                     $(MOZSDK_INCLUDE_DIR)/dom \
                     $(MOZSDK_INCLUDE_DIR)/intl \
                     $(MOZSDK_INCLUDE_DIR)/necko \
                     $(MOZSDK_INCLUDE_DIR)/unicharutil \
                     $(NULL)

ifdef MEDIA_CORE_GST_SYSTEM
   CPP_EXTRA_FLAGS = -DGST_SYSTEM=1 \
                     $(GSTREAMER_CFLAGS) \
                     $(NULL)

   CPP_RAW_INCLUDES = $(GSTREAMER_CFLAGS) \
                      $(GLIB_CFLAGS) \
                      $(NULL)
else
   CPP_EXTRA_INCLUDES += \
    $(DEPS_DIR)/gstreamer/$(SB_CONFIGURATION)/include/gstreamer-$(GST_VERSION) \
    $(DEPS_DIR)/gst-plugins-base/$(SB_CONFIGURATION)/include/gstreamer-$(GST_VERSION) \
    $(NULL)

   ifeq (,$(filter-out macosx windows,$(SB_PLATFORM)))
      # macosx or windows
      CPP_EXTRA_INCLUDES += \
       $(DEPS_DIR)/glib/$(SB_CONFIGURATION)/include/glib-$(GLIB_VERSION) \
       $(DEPS_DIR)/glib/$(SB_CONFIGURATION)/lib/glib-$(GLIB_VERSION)/include \
       $(NULL)
   else
      # everything else
      CPP_RAW_INCLUDES += $(GLIB_CFLAGS) \
                          $(NULL)
   endif
endif

# The GStreamer headers trigger this warning on MSVC; it's harmless
ifeq (windows,$(SB_PLATFORM))
   CPP_EXTRA_FLAGS += "-wd4244"
endif

include $(topsrcdir)/build/rules.mk
