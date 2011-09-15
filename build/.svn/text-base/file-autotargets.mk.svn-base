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

##############################################################################
# file-autotargets.mk
#
# Throughout the old build system, there are constructs where we create a list
# of files, and the variable represents their location to be placed in the
# dist directory, to be shipped (SONGBIRD_DIST, SONGBIRD_XULRUNNER, etc.)
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
GST_PLUGINS_TARGETS = $(addprefix $(SONGBIRD_GSTPLUGINSDIR)/,$(notdir $(SONGBIRD_GST_PLUGINS)))
libs:: $(GST_PLUGINS_TARGETS)

LIB_DIR_TARGETS = $(addprefix $(SONGBIRD_LIBDIR)/,$(notdir $(SONGBIRD_LIB)))
libs:: $(LIB_DIR_TARGETS)

XR_DIR_TARGETS = $(addprefix $(SONGBIRD_XULRUNNERDIR)/,$(notdir $(SONGBIRD_XULRUNNER)))
libs:: $(XR_DIR_TARGETS)

DIST_DIR_TARGETS = $(addprefix $(SONGBIRD_DISTDIR)/,$(notdir $(SONGBIRD_DIST)))
libs:: $(DIST_DIR_TARGETS)

CHROME_DIR_TARGETS = $(addprefix $(SONGBIRD_CHROMEDIR)/,$(notdir $(SONGBIRD_CHROME)))
libs:: $(CHROME_DIR_TARGETS)

SCRIPTS_DIR_TARGETS = $(addprefix $(SONGBIRD_SCRIPTSDIR)/,$(notdir $(SONGBIRD_SCRIPTS)))
libs:: $(SCRIPTS_DIR_TARGETS)

JSMODULES_DIR_TARGETS = $(addprefix $(SONGBIRD_JSMODULESDIR)/,$(notdir $(SONGBIRD_JSMODULES)))
libs:: $(JSMODULES_DIR_TARGETS)

COMPONENTS_DIR_TARGETS = $(addprefix $(SONGBIRD_COMPONENTSDIR)/,$(notdir $(SONGBIRD_COMPONENTS)))
libs:: $(COMPONENTS_DIR_TARGETS)

DOCUMENTATION_DIR_TARGETS = $(addprefix $(SONGBIRD_DOCUMENTATIONDIR)/,$(notdir $(SONGBIRD_DOCUMENTATION)))
libs:: $(DOCUMENTATION_DIR_TARGETS)

PREFS_DIR_TARGETS = $(addprefix $(SONGBIRD_PREFERENCESDIR)/,$(notdir $(SONGBIRD_PREFS)))
libs:: $(PREFS_DIR_TARGETS)

PLUGINS_DIR_TARGETS = $(addprefix $(SONGBIRD_PLUGINSDIR)/,$(notdir $(SONGBIRD_PLUGINS)))
libs:: $(PLUGINS_DIR_TARGETS)

SEARCHPLUGINS_DIR_TARGETS = $(addprefix $(SONGBIRD_SEARCHPLUGINSDIR)/,$(notdir $(SONGBIRD_SEARCHPLUGINS)))
libs:: $(SEARCHPLUGINS_DIR_TARGETS)

CONTENTS_DIR_TARGETS = $(addprefix $(SONGBIRD_CONTENTSDIR)/,$(notdir $(SONGBIRD_CONTENTS)))
libs:: $(CONTENTS_DIR_TARGETS)

PROFILE_DIR_TARGETS = $(addprefix $(SONGBIRD_PROFILEDIR)/,$(notdir $(SONGBIRD_PROFILE)))
libs:: $(PROFILE_DIR_TARGETS)

##
## THERE BE DRAGONS HERE!
##
## The ordering of these rules matters! More generic rules (SONGBIRD_DIST)
## MUST be further down in the list, or else they'll get matched with
## the core parts in the stem and none of the wildcard "magic" will work.
##
## Translation: best not to mess with this stuff, unless you really have to...
##

.SECONDEXPANSION:
$(SONGBIRD_JSMODULESDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_JSMODULES))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(SONGBIRD_JSMODULESDIR)/$(@F)

$(SONGBIRD_GSTPLUGINSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_GST_PLUGINS))),$(addprefix $$d,%)))
	$(INSTALL_PROG) $^ $(SONGBIRD_GSTPLUGINSDIR)/$(@F)

$(SONGBIRD_LIBDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_LIB))),$(addprefix $$d,%)))
	$(INSTALL_PROG) $^ $(SONGBIRD_LIBDIR)/$(@F)

ifdef XULRUNNERDIR_MODE
   XULRUNNERDIR_INSTALL = $(INSTALL) -m $(XULRUNNERDIR_MODE)
else
   XULRUNNERDIR_INSTALL = $(INSTALL_FILE)
endif

$(SONGBIRD_XULRUNNERDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_XULRUNNER))),$(addprefix $$d,%)))
	$(XULRUNNERDIR_INSTALL) $^ $(SONGBIRD_XULRUNNERDIR)/$(@F)

$(SONGBIRD_CHROMEDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_CHROME))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(SONGBIRD_CHROMEDIR)/$(@F)

$(SONGBIRD_SEARCHPLUGINSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_SEARCHPLUGINS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(SONGBIRD_SEARCHPLUGINSDIR)/$(@F)

$(SONGBIRD_SCRIPTSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_SCRIPTS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(SONGBIRD_SCRIPTSDIR)/$(@F)

$(SONGBIRD_DOCUMENTATIONDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_DOCUMENTATION))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(SONGBIRD_DOCUMENTATIONDIR)/$(@F)

$(SONGBIRD_PREFERENCESDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_PREFS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(SONGBIRD_PREFERENCESDIR)/$(@F)

$(SONGBIRD_PLUGINSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_PLUGINS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(SONGBIRD_PLUGINSDIR)/$(@F)

$(SONGBIRD_SEARCHPLUGINSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_SEARCHPLUGINS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(SONGBIRD_SEARCHPLUGINSDIR)/$(@F)

$(SONGBIRD_PROFILEDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_PROFILE))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(SONGBIRD_PROFILEDIR)/$(@F)

ifdef COMPONENTSDIR_MODE
   COMPONENTSDIR_INSTALL = $(INSTALL) -m $(COMPONENTSDIR_MODE)
else
   COMPONENTSDIR_INSTALL = $(INSTALL_FILE)
endif

$(SONGBIRD_COMPONENTSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_COMPONENTS))),$(addprefix $$d,%)))
	$(COMPONENTSDIR_INSTALL) $^ $(SONGBIRD_COMPONENTSDIR)/$(@F)

ifdef DISTDIR_MODE
   DISTDIR_INSTALL = $(INSTALL) -m $(DISTDIR_MODE)
else
   DISTDIR_INSTALL = $(INSTALL_FILE)
endif

$(SONGBIRD_DISTDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_DIST))),$(addprefix $$d,%)))
	$(DISTDIR_INSTALL) $^ $(SONGBIRD_DISTDIR)/$(@F)

$(SONGBIRD_CONTENTSDIR)/%: $$(wildcard $$(foreach d, $$(sort $$(dir $$(SONGBIRD_CONTENTS))),$(addprefix $$d,%)))
	$(INSTALL_FILE) $^ $(SONGBIRD_CONTENTSDIR)/$(@F)

#------------------------------------------------------------------------------
endif #FILE_AUTOTARGETS_MK_INCLUDED
#------------------------------------------------------------------------------
