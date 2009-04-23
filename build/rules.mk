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
# Rules.mk
#
# This file takes care of lots of messy rules. Each one is explained below.
###############################################################################

#------------------------------------------------------------------------------
# Only include this file once
ifndef RULES_MK_INCLUDED
RULES_MK_INCLUDED=1
#------------------------------------------------------------------------------

# include config.mk to pick up extra variables

include $(topsrcdir)/build/config.mk
include $(topsrcdir)/build/tiers.mk

# Since public, src, and test are directories used throughout the tree
# we automatically add them to SUBDIRS _unless_ it's requested that we don't
ifeq (,$(DISABLE_IMPLICIT_SUBDIRS))
   SUBDIRS += $(if $(wildcard $(srcdir)/public), public)
   SUBDIRS += $(if $(wildcard $(srcdir)/src), src)
   ifdef SB_ENABLE_TESTS
      SUBDIRS += $(if $(wildcard $(srcdir)/test), test)
   endif
endif

#$(info SUBDIRS are $(SUBDIRS))
#$(info MAKECMDGOALS is $(MAKECMDGOALS))

################################################################################

# SUBMAKEFILES: List of Makefiles for next level down.
#   This is used to update or create the Makefiles before invoking them.
#SUBMAKEFILES += $(addsuffix /Makefile, $(DIRS) $(TOOL_DIRS))

# The root makefile doesn't want to do a plain export/libs, because
# of the tiers and because of libxul. Suppress the default rules in favor
# of something else. Makefiles which use this var *must* provide a sensible
# default rule before including rules.mk

LOOP_OVER_SUBDIRS = \
   @$(EXIT_ON_ERROR) \
   $(foreach dir,$(SUBDIRS), $(MAKE) -C $(dir) $@; ) true

LOOP_OVER_STATIC_DIRS = \
   @$(EXIT_ON_ERROR) \
   $(foreach dir,$(STATIC_DIRS), $(MAKE) -C $(dir) $@; ) true

LOOP_OVER_TOOL_DIRS = \
   @$(EXIT_ON_ERROR) \
   $(foreach dir,$(TOOL_DIRS), $(MAKE) -C $(dir) $@; ) true

# MAKE_DIRS: List of directories to build while looping over directories.
ifneq (,$(OBJS)$(XPIDLSRCS)$(SDK_XPIDLSRCS)$(SIMPLE_PROGRAMS))
   MAKE_DIRS += $(MDDEPDIR)
   GARBAGE_DIRS += $(MDDEPDIR)
endif


#all::   $(targets) \
#        garbage \
#        $(NULL)

#clean:: $(clean_targets) \
#        create_dirs_clean \
#        $(NULL)


################################################################################

# The root makefile doesn't want to do a plain export/libs, because
# of the tiers and because of libxul. Suppress the default rules in favor
# of something else. Makefiles which use this var *must* provide a sensible
# default rule before including rules.mk

# SUBMAKEFILES: List of Makefiles for next level down.
#   This is used to update or create the Makefiles before invoking them.
SUBMAKEFILES += $(addsuffix /Makefile, $(SUBDIRS) $(TOOL_DIRS))

ifdef TIERS

SUBDIRS += $(foreach tier,$(TIERS),$(tier_$(tier)_dirs))
STATIC_DIRS += $(foreach tier,$(TIERS),$(tier_$(tier)_staticdirs))

default all alldep:: create_dirs
	$(EXIT_ON_ERROR) \
	$(foreach tier,$(TIERS),$(MAKE) tier_$(tier); ) true

else

default all:: create_dirs
#	@$(EXIT_ON_ERROR) \
#    $(foreach dir,$(STATIC_DIRS),$(MAKE) -C $(dir); ) true
	$(MAKE) export
	$(MAKE) libs
#	$(MAKE) tools
endif

MAKE_TIER_SUBMAKEFILES = +$(if $(tier_$*_dirs),$(MAKE) $(addsuffix /Makefile,$(tier_$*_dirs)))

export_tier_%:
	@echo "export_tier: $*"
	$(MAKE_TIER_SUBMAKEFILES)
	$(EXIT_ON_ERROR) \
    $(foreach dir,$(tier_$*_dirs),$(MAKE) -C $(dir) export; ) true

libs_tier_%:
	@echo "libs_tier: $*"
	$(MAKE_TIER_SUBMAKEFILES)
	$(EXIT_ON_ERROR) \
    $(foreach dir,$(tier_$*_dirs),$(MAKE) -C $(dir) libs; ) true

#tools_tier_%:
#	@echo "tools_tier_$@"
#	@echo "$@"
#	@$(MAKE_TIER_SUBMAKEFILES)
#	@$(EXIT_ON_ERROR) \
#    $(foreach dir,$(tier_$*_dirs),$(MAKE) -C $(dir) tools; ) true

$(foreach tier,$(TIERS),tier_$(tier))::
	@echo "$@: $($@_staticdirs) $($@_dirs)"
	$(EXIT_ON_ERROR) \
    $(foreach dir,$($@_staticdirs),$(MAKE) -C $(dir); ) true
	$(MAKE) export_$@
	$(MAKE) libs_$@

##
## SUBDIRS handling for libs and export targets
##

libs:: $(SUBMAKEFILES) $(SUBDIRS)
	+$(LOOP_OVER_SUBDIRS)

export:: $(SUBMAKEFILES) $(SUBDIRS)
	+$(LOOP_OVER_SUBDIRS)

##
## Program handling for libs and export targets
##

## preedTODO: hacky; fix this.
MAKE_JAR_DEP = $(if $(JAR_MANIFEST),make_jar)

libs:: $(HOST_LIBRARY) $(LIBRARY) $(SHARED_LIBRARY) $(IMPORT_LIBRARY) $(PROGRAM) $(SIMPLE_PROGRAM) $(SONGBIRD_COMPONENTS) $(STATIC_LIB) $(DYNAMIC_LIB) $(JAR_MANIFEST) preferences_preprocess $(MAKE_JAR_DEP) copy_sb_tests shell_execute make_xpi run_installer_preprocess
ifndef NO_DIST_INSTALL
   ifdef LIBRARY
      ifdef EXPORT_LIBRARY # Stage libs that will be linked into a static build
         ifdef IS_COMPONENT
	         $(INSTALL) $(IFLAGS1) $(LIBRARY) $(DEPTH)/staticlib/components
         else
	         $(INSTALL) $(IFLAGS1) $(LIBRARY) $(DEPTH)/staticlib
         endif
      endif # EXPORT_LIBRARY
      ifdef DIST_INSTALL
         ifdef IS_COMPONENT
            $(error Shipping static component libs makes no sense.)
         else
	         $(INSTALL) $(IFLAGS1) $(LIBRARY) $(DIST)/lib
         endif
      endif # DIST_INSTALL
   endif # LIBRARY
   ifdef SHARED_LIBRARY
      ifdef IS_COMPONENT
	      $(INSTALL) $(IFLAGS2) $(SHARED_LIBRARY) $(FINAL_TARGET)/components
         $(ELF_DYNSTR_GC) $(FINAL_TARGET)/components/$(SHARED_LIBRARY)
      else # ! IS_COMPONENT
         ifneq (,$(filter OS2 WINNT WINCE,$(OS_ARCH)))
	         $(INSTALL) $(IFLAGS2) $(IMPORT_LIBRARY) $(DIST)/lib
         else
	         $(INSTALL) $(IFLAGS2) $(SHARED_LIBRARY) $(DIST)/lib
         endif
	      $(INSTALL) $(IFLAGS2) $(SHARED_LIBRARY) $(FINAL_TARGET)
      endif # IS_COMPONENT
   endif # SHARED_LIBRARY
   ifdef PROGRAM
	   $(INSTALL) $(IFLAGS2) $(PROGRAM) $(FINAL_TARGET)
   endif
   ifdef SIMPLE_PROGRAMS
	   $(INSTALL_PROG) $(SIMPLE_PROGRAM) $(FINAL_TARGET)
   endif
   ifndef EXTENSION_STAGE_DIR
   ifdef DYNAMIC_LIB
      ifdef IS_COMPONENT
	      $(INSTALL_PROG) $(DYNAMIC_LIB) $(SONGBIRD_COMPONENTSDIR)/
      else
	      $(INSTALL_PROG) $(DYNAMIC_LIB) $(SONGBIRD_LIBDIR)/
      endif
   endif
   endif
endif # !NO_DIST_INSTALL

ifdef XPIDL_SRCS
libs:: $(patsubst %.idl,%.xpt, $(XPIDL_SRCS)) $(XPIDL_MODULE)
   ifdef XPIDL_MODULE
   ifndef XPI_NAME
	   $(INSTALL_FILE) $(XPIDL_MODULE) $(SONGBIRD_COMPONENTSDIR)
   endif
   endif
endif

#export:: $(SUBMAKEFILES) $(MAKE_DIRS) $(SUBDIRS) $(xpidl_headers) $(if $(EXPORTS)$(XPIDL_SRCS)$(SDK_HEADERS)$(SDK_XPIDLSRCS),$(PUBLIC)) $(if $(SDK_HEADERS)$(SDK_XPIDLSRCS),$(SDK_PUBLIC)) $(if $(XPIDLSRCS),$(IDL_DIR)) $(if $(SDK_XPIDLSRCS),$(SDK_IDL_DIR))
#	+$(LOOP_OVER_DIRS)
#	+$(LOOP_OVER_TOOL_DIRS)

XPIDL_GEN_DIR = .

#export:: $(SUBMAKEFILES) $(MAKE_DIRS) $(SUBDIRS) $(XPIDL_SRCS)
#	+$(LOOP_OVER_SUBDIRS)
#	+$(LOOP_OVER_TOOL_DIRS)

export:: gunzip_file unzip_file appini_preprocess

export:: $(patsubst %.idl,$(XPIDL_GEN_DIR)/%.h, $(XPIDL_SRCS)) $(SONGBIRD_PP_COMPONENTS) sb_resources_preprocess sb_components_preprocess 
	@#echo in xpidl header target: $(xpidl_headers), $(XPIDL_HEADER_SRCS), $^
	@#echo command is: $(CP) $^ 

#export:: $(xpidl_headers)
#	@echo in xpidl header target: $(xpidl_headers), $(XPIDL_HEADER_SRCS), $^
#	@echo command is: $(CP) $^ 

include $(topsrcdir)/build/oldrules.mk

.PHONY: $(SUBDIRS) FORCE libs export

echo-variable-%:
	@echo $($*)

echo-tiers:
	@echo $(TIERS)

echo-dirs:
	@echo $(DIRS)

echo-module:
	@echo $(MODULE)

echo-requires:
	@echo $(REQUIRES)

FORCE:

# Cancel these implicit rules

%: %,v

%: RCS/%,v

%: s.%

%: SCCS/s.%

# TODO
#
# Re-define the list of default suffixes, so gmake won't have to churn through
# hundreds of built-in suffix rules for stuff we don't need.
#
### .SUFFIXES:

include $(topsrcdir)/build/file-autotargets.mk

#------------------------------------------------------------------------------
endif #RULES_MK_INCLUDED
#------------------------------------------------------------------------------
