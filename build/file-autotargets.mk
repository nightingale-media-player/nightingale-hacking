#
# BEGIN NIGHTINGALE GPL
#
# This file is part of the Nightingale web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://www.getnightingale.com
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
# END NIGHTINGALE GPL
#

##############################################################################
# file-autotargets.mk
#
# Throughout the old build system, there are constructs where we create a list
# of files, and the variable represents their location to be placed in the
# dist directory, to be shipped (NIGHTINGALE_DIST, NIGHTINGALE_XULRUNNER, etc.)
#
# The old method of doing this took this list and just ran a cp on it. This
# is bad for a couple of reasons:
#
#   a. As that list grows (gst-plugins-*, especially), we run the risk of
#      running into shell/command MAX_PATH limits (on Win32, especially)
#
#   b. Doing it this way doesn't allow make to do proper dependency checking,
#      and not execute if the file already exists.
#
# Unfortunately, getting make to accept that there can be a dependency from a
# sourcefile in a set of random, different directories to the basename of
# those files in a common directory proves... annoying.
#
# We accomplish this by adding GST_FINAL_PLUGINS to the export target, then
# creates a dependency by going looking through all the directories provided
# in the original list, and then looking for a file with that name. We could
# have collisions in this list (foo/a.so and bar/a.so), but we don't really
# care because the original structure of this was used to copy them all to
# the same directory, so one of them will get clobbered anyway. We should
# assert this in the future (see the preedTODO below).
#
# Doing this correctly requires the use of gmake SECONDEXPANSION magic because
# % doesn't expand to anything normally; we need to get make to parse everything
# one more time, so we can pass it to our horrid list of functions, to generate
# the unique dependency.
#
# I'm betting there's a better way to do this, but... it works.
#
###############################################################################

#------------------------------------------------------------------------------
# Only include this file once
ifndef FILE_AUTOTARGETS_MK_INCLUDED
FILE_AUTOTARGETS_MK_INCLUDED=1
#------------------------------------------------------------------------------

# preedTODO: confirm there are no dups.; see page 70 of make book
GST_PLUGINS_TARGETS = $(addprefix $(NIGHTINGALE_GSTPLUGINSDIR)/,$(notdir $(NIGHTINGALE_GST_PLUGINS)))
libs:: $(GST_PLUGINS_TARGETS)

LIB_DIR_TARGETS = $(addprefix $(NIGHTINGALE_LIBDIR)/,$(notdir $(NIGHTINGALE_LIB)))
libs:: $(LIB_DIR_TARGETS)

XR_DIR_TARGETS = $(addprefix $(NIGHTINGALE_XULRUNNERDIR)/,$(notdir $(NIGHTINGALE_XULRUNNER)))
libs:: $(XR_DIR_TARGETS)

DIST_DIR_TARGETS = $(addprefix $(NIGHTINGALE_DISTDIR)/,$(notdir $(NIGHTINGALE_DIST)))
libs:: $(DIST_DIR_TARGETS)

CHROME_DIR_TARGETS = $(addprefix $(NIGHTINGALE_CHROMEDIR)/,$(notdir $(NIGHTINGALE_CHROME)))
libs:: $(CHROME_DIR_TARGETS)

SCRIPTS_DIR_TARGETS = $(addprefix $(NIGHTINGALE_SCRIPTSDIR)/,$(notdir $(NIGHTINGALE_SCRIPTS)))
libs:: $(SCRIPTS_DIR_TARGETS)

JSMODULES_DIR_TARGETS = $(addprefix $(NIGHTINGALE_JSMODULESDIR)/,$(notdir $(NIGHTINGALE_JSMODULES)))
libs:: $(JSMODULES_DIR_TARGETS)

COMPONENTS_DIR_TARGETS = $(addprefix $(NIGHTINGALE_COMPONENTSDIR)/,$(notdir $(NIGHTINGALE_COMPONENTS)))
libs:: $(COMPONENTS_DIR_TARGETS)

DOCUMENTATION_DIR_TARGETS = $(addprefix $(NIGHTINGALE_DOCUMENTATIONDIR)/,$(notdir $(NIGHTINGALE_DOCUMENTATION)))
libs:: $(DOCUMENTATION_DIR_TARGETS)

PREFS_DIR_TARGETS = $(addprefix $(NIGHTINGALE_PREFERENCESDIR)/,$(notdir $(NIGHTINGALE_PREFS)))
libs:: $(PREFS_DIR_TARGETS)

PLUGINS_DIR_TARGETS = $(addprefix $(NIGHTINGALE_PLUGINSDIR)/,$(notdir $(NIGHTINGALE_PLUGINS)))
libs:: $(PLUGINS_DIR_TARGETS)

SEARCHPLUGINS_DIR_TARGETS = $(addprefix $(NIGHTINGALE_SEARCHPLUGINSDIR)/,$(notdir $(NIGHTINGALE_SEARCHPLUGINS)))
libs:: $(SEARCHPLUGINS_DIR_TARGETS)

CONTENTS_DIR_TARGETS = $(addprefix $(NIGHTINGALE_CONTENTSDIR)/,$(notdir $(NIGHTINGALE_CONTENTS)))
libs:: $(CONTENTS_DIR_TARGETS)

PROFILE_DIR_TARGETS = $(addprefix $(NIGHTINGALE_PROFILEDIR)/,$(notdir $(NIGHTINGALE_PROFILE)))
libs:: $(PROFILE_DIR_TARGETS)

##
## THERE BE DRAGONS HERE!
##
## The ordering of these rules matters! More generic rules (NIGHTINGALE_DIST)
## MUST be further down in the list, or else they'll get matched with
## the core parts in the stem and none of the wildcard "magic" will work.
##
## Translation: best not to mess with this stuff, unless you really have to...
##

.SECONDEXPANSION:
$(NIGHTINGALE_JSMODULESDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_JSMODULES))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(NIGHTINGALE_JSMODULESDIR)/$(@F)

$(NIGHTINGALE_GSTPLUGINSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_GST_PLUGINS))),$(addprefix $$d,%)))
	$(INSTALL_PROG) $^ $(NIGHTINGALE_GSTPLUGINSDIR)/$(@F)

$(NIGHTINGALE_LIBDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_LIB))),$(addprefix $$d,%)))
	$(INSTALL_PROG) $^ $(NIGHTINGALE_LIBDIR)/$(@F)

ifdef XULRUNNERDIR_MODE
   XULRUNNERDIR_INSTALL = $(INSTALL) -m $(XULRUNNERDIR_MODE)
else
   XULRUNNERDIR_INSTALL = $(INSTALL_FILE)
endif

$(NIGHTINGALE_XULRUNNERDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_XULRUNNER))),$(addprefix $$d,%)))
	$(XULRUNNERDIR_INSTALL) $^ $(NIGHTINGALE_XULRUNNERDIR)/$(@F)

$(NIGHTINGALE_CHROMEDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_CHROME))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(NIGHTINGALE_CHROMEDIR)/$(@F)

$(NIGHTINGALE_SEARCHPLUGINSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_SEARCHPLUGINS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(NIGHTINGALE_SEARCHPLUGINSDIR)/$(@F)

$(NIGHTINGALE_SCRIPTSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_SCRIPTS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(NIGHTINGALE_SCRIPTSDIR)/$(@F)

$(NIGHTINGALE_DOCUMENTATIONDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_DOCUMENTATION))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(NIGHTINGALE_DOCUMENTATIONDIR)/$(@F)

$(NIGHTINGALE_PREFERENCESDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_PREFS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(NIGHTINGALE_PREFERENCESDIR)/$(@F)

$(NIGHTINGALE_PLUGINSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_PLUGINS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(NIGHTINGALE_PLUGINSDIR)/$(@F)

$(NIGHTINGALE_SEARCHPLUGINSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_SEARCHPLUGINS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(NIGHTINGALE_SEARCHPLUGINSDIR)/$(@F)

$(NIGHTINGALE_PROFILEDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_PROFILE))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(NIGHTINGALE_PROFILEDIR)/$(@F)

ifdef COMPONENTSDIR_MODE
   COMPONENTSDIR_INSTALL = $(INSTALL) -m $(COMPONENTSDIR_MODE)
else
   COMPONENTSDIR_INSTALL = $(INSTALL_FILE)
endif

$(NIGHTINGALE_COMPONENTSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_COMPONENTS))),$(addprefix $$d,%)))
	$(COMPONENTSDIR_INSTALL) $^ $(NIGHTINGALE_COMPONENTSDIR)/$(@F)

ifdef DISTDIR_MODE
   DISTDIR_INSTALL = $(INSTALL) -m $(DISTDIR_MODE)
else
   DISTDIR_INSTALL = $(INSTALL_FILE)
endif

$(NIGHTINGALE_DISTDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_DIST))),$(addprefix $$d,%)))
	$(DISTDIR_INSTALL) $^ $(NIGHTINGALE_DISTDIR)/$(@F)

$(NIGHTINGALE_CONTENTSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(NIGHTINGALE_CONTENTS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(NIGHTINGALE_CONTENTSDIR)/$(@F)

#------------------------------------------------------------------------------
endif #FILE_AUTOTARGETS_MK_INCLUDED
#------------------------------------------------------------------------------
