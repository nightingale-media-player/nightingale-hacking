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

DEPTH = ..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/documentation
VPATH = /media/disk/Linux/makepkg/songbird/documentation

include $(DEPTH)/build/autodefs.mk

# The DOXYGEN_ variables need to be relative, because doxygen.exe on Win32
# doesn't understand MSYS-style paths.
DOXYGEN_INPUTDIRS = $(DEPTH)/../components \
                    $(DEPTH)/../bindings \
                    $(NULL)
                    
DOXYGEN_OUTPUTDIR = sdkapi \
                    $(NULL)

DOXYGEN_STRIP_TOPSRCDIR = $(DEPTH)/.. \
                          $(NULL)

SONGBIRD_DOCUMENTATION = $(srcdir)/naturaldocsconfig/Docs.css \
                         $(srcdir)/doxygenconfig/header.html \
                         $(srcdir)/doxygenconfig/footer.html \
                         $(NULL)

SONGBIRD_PP_RESOURCES = $(srcdir)/songbird.dox.in \
                        $(NULL)

PPDEFINES += \
 -DDOXYGEN_INPUTDIRS="$(DOXYGEN_INPUTDIRS)" \
 -DDOXYGEN_OUTPUTDIR="$(DOXYGEN_OUTPUTDIR)" \
 -DDOXYGEN_STRIP_TOPSRCDIR="$(DOXYGEN_STRIP_TOPSRCDIR)" \
 $(NULL)

SONGBIRD_PP_DIR = $(SONGBIRD_DOCUMENTATIONDIR)

CREATEDIRS += $(CURDIR)/webpageapi \
              $(CURDIR)/sdkapi \
              $(NULL)

GARBAGE += Docs.css \
           header.html \
           footer.html \
           tmp \
           $(CREATEDIRS) \
           $(NULL)

include $(topsrcdir)/build/rules.mk

libs::
ifeq (,$(DOXYGEN))
	$(error Doxygen not found. Cannot build docs here. Install doxygen. Bailing.)
else
	$(DOXYGEN) songbird.dox
endif
	$(topsrcdir)/documentation/make-webpageapi-docs.sh $(topsrcdir)
	$(INSTALL_FILE) $(srcdir)/sb_logo_doc.gif $(CURDIR)/sdkapi/html/sb_logo_doc.gif
	$(INSTALL_FILE) $(srcdir)/sb_logo_doc.gif $(CURDIR)/webpageapi/sb_logo_doc.gif
