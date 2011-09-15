#
# BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
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

DEPTH = ../../../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/mediaimport/library/itunes/src
VPATH = /media/disk/Linux/makepkg/songbird/components/mediaimport/library/itunes/src

include $(DEPTH)/build/autodefs.mk

DYNAMIC_LIB = sbiTunesMediaImport

CPP_SRCS = sbiTunesXMLParser.cpp \
           sbiTunesDatabaseServices.cpp \
           sbiTunesImporter.cpp \
           sbiTunesImporterAlbumArtListener.cpp \
           sbiTunesImporterJob.cpp \
           sbiTunesImporterStatus.cpp \
           sbiTunesSignature.cpp \
           sbiTunesImporterComponent.cpp \
           $(NULL)

           
CPP_EXTRA_INCLUDES = $(topsrcdir)/components/include \
                     $(DEPTH)/components/albumart/public \
                     $(DEPTH)/components/dbengine/public \
                     $(DEPTH)/components/dataremote/public \
                     $(DEPTH)/components/job/public \
                     $(DEPTH)/components/library/base/public \
                     $(DEPTH)/components/library/localdatabase/public \
                     $(DEPTH)/components/moz/strings/components/public \
                     $(DEPTH)/components/mediacore/base/public \
                     $(DEPTH)/components/mediaimport/library/base/public \
                     $(DEPTH)/components/mediaimport/library/itunes/public \
                     $(DEPTH)/components/property/public \
                     $(DEPTH)/components/testharness/public \
                     $(topsrcdir)/components/library/base/src/static \
                     $(topsrcdir)/components/moz/streams/src \
                     $(topsrcdir)/components/moz/strings/src \
                     $(topsrcdir)/components/moz/threads/src \
                     $(topsrcdir)/components/property/src \
                     $(MOZSDK_INCLUDE_DIR)/intl \
                     $(MOZSDK_INCLUDE_DIR)/necko \
                     $(MOZSDK_INCLUDE_DIR)/pref \
                     $(MOZSDK_INCLUDE_DIR)/uconv \
                     $(MOZSDK_INCLUDE_DIR)/xml \
                     $(MOZSDK_INCLUDE_DIR)/xulapp \
                     $(NULL)
               
IS_COMPONENT = 1

DYNAMIC_LIB_EXTRA_IMPORTS = plds4 \
                            $(NULL)

DYNAMIC_LIB_IMPORT_EXTRA_PATHS = \
 $(DEPS_DIR)/wpdsdk/Lib/x86/ \
 $(DEPTH)/components/moz/streams/src \
 $(DEPTH)/components/moz/strings/components/src \
 $(DEPTH)/components/moz/strings/src \
 $(DEPTH)/components/library/base/src/static \
 $(NULL)

DYNAMIC_LIB_STATIC_IMPORTS += \
 components/library/base/src/static/sbLibraryUtils \
 components/moz/streams/src/sbMozStreams \
 components/moz/strings/src/sbMozStringUtils \
 components/moz/threads/src/sbMozThreads \
 $(NULL)

include $(topsrcdir)/build/rules.mk
