# vim: ft=make ts=3 sw=3
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

# define the tiers of the application
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

###############################################################################

LOOP_OVER_SUBDIRS = \
   @$(EXIT_ON_ERROR) \
   $(foreach dir,$(SUBDIRS), $(MAKE) -C $(dir) $@; ) true

# MAKE_DIRS: List of directories to build while looping over directories.
ifneq (,$(OBJS)$(XPIDLSRCS)$(SDK_XPIDLSRCS)$(SIMPLE_PROGRAMS))
   MAKE_DIRS += $(MDDEPDIR)
   GARBAGE_DIRS += $(MDDEPDIR)
endif


###############################################################################

# SUBMAKEFILES: List of Makefiles for next level down.
#   This is used to update or create the Makefiles before invoking them.
SUBMAKEFILES += $(addsuffix /Makefile, $(SUBDIRS))

ifdef TIERS
   SUBDIRS += $(foreach tier,$(TIERS),$(tier_$(tier)_dirs))

default all alldep:: create_dirs
	$(EXIT_ON_ERROR) \
	$(foreach tier,$(TIERS),$(MAKE) tier_$(tier); ) true

else

default all:: create_dirs
	$(MAKE) export
	$(MAKE) libs
endif

ALL_TRASH = \
   $(GARBAGE) \
   $(XPIDL_HEADERS) $(XPIDL_TYPELIBS) $(XPIDL_MODULE) \
   $(DYNAMIC_LIB_OBJS) \
   $(OBJS:.$(OBJ_SUFFIX)=.s) $(OBJS:.$(OBJ_SUFFIX)=.ii) \
   $(OBJS:.$(OBJ_SUFFIX)=.i) \
   $(SIMPLE_PROGRAM) $(SIMPLE_PROGRAM_OBJS) \
   LOGS TAGS a.out

clean:: $(SUBMAKEFILES)
	-$(RM) -f $(ALL_TRASH)
	+$(LOOP_OVER_SUBDIRS)

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

$(foreach tier,$(TIERS),tier_$(tier))::
	@echo "$@: $($@_staticdirs) $($@_dirs)"
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

libs:: $(HOST_LIBRARY) $(LIBRARY) $(SHARED_LIBRARY) $(IMPORT_LIBRARY) $(PROGRAM) $(SIMPLE_PROGRAM) $(SONGBIRD_COMPONENTS) $(STATIC_LIB) $(DYNAMIC_LIB) $(JAR_MANIFEST) $(MAKE_JAR_DEP) copy_sb_tests make_xpi
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
   ifdef SIMPLE_PROGRAM
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

export:: $(SONGBIRD_PP_COMPONENTS) sb_resources_preprocess
	@#echo in xpidl header target: $(XPIDL_HEADERS), $(XPIDL_HEADER_SRCS), $^
	@#echo command is: $(CP) $^ 

include $(topsrcdir)/build/oldrules.mk

## oldrules.mk translations ##

#------------------------------------------------------------------------------
# Rules for XPIDL compilation
#------------------------------------------------------------------------------
#
# XPIDL_SRCS - a list of idl files to turn into header and typelib files
# XPIDL_HEADER_SRCS - a list of idl files to turn into C++ header files
# XPIDL_TYPELIB_SRCS - a list of idl files to turn into xpt typelib files
# XPIDL_MODULE - the name of an xpt file that will created from linking several
#                other xpt typelibs
# XPIDL_MODULE_TYPELIBS - a list of xpt files to link into the module
# XPIDL_INCLUDES - a list of dirs to search when looking for included idls
# XPIDL_EXTRA_FLAGS - additional flags to send to XPIDL
#

XPIDL_INCLUDES += $(MOZSDK_IDL_DIR) \
                  $(srcdir) \
                  $(NULL)

XPIDL_HEADERS = $(XPIDL_SRCS:.idl=.h)

$(XPIDL_HEADERS): %.h: %.idl
	$(XPIDL) -m header $(addprefix -I,$(XPIDL_INCLUDES)) $(XPIDL_EXTRA_FLAGS) $<

export:: $(XPIDL_HEADERS)

XPIDL_TYPELIBS = $(XPIDL_SRCS:.idl=.xpt)

$(XPIDL_TYPELIBS): %.xpt: %.idl
	$(XPIDL) -m typelib $(addprefix -I,$(XPIDL_INCLUDES)) $(XPIDL_EXTRA_FLAGS) -e $@ $<

# The ifneq() check is in here because if the collected typelibs are the same 
# (single file) as XPIDL_MODULE, there's no reason to run xpt_link on them.
# (In fact, this creates a circular make dependency that gets dropped, but 
# xpt_link clobbers the file in the process of trying to link it, and 
# fails anyway.

$(XPIDL_MODULE): $(XPIDL_MODULE_TYPELIBS)
ifneq ($(strip $(XPIDL_MODULE)),$(strip $(XPIDL_MODULE_TYPELIBS)))
	$(XPTLINK) $(XPIDL_MODULE) $(XPIDL_MODULE_TYPELIBS)
endif

libs:: $(XPIDL_TYPELIBS) $(XPIDL_MODULE)
ifneq (,$(XPIDL_MODULE))
ifndef XPI_NAME
	$(INSTALL_FILE) $(XPIDL_MODULE) $(SONGBIRD_COMPONENTSDIR)
endif
endif

#########################

.PHONY: $(SUBDIRS) FORCE libs export

echo-variable-%:
	@echo $($*)

echo-tiers:
	@echo $(TIERS)

echo-subdirs:
	@echo $(SUBDIRS)

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

# Re-define the list of default suffixes, so gmake won't have to churn through
# hundreds of built-in suffix rules for stuff we don't need.

.SUFFIXES:

include $(topsrcdir)/build/file-autotargets.mk

#------------------------------------------------------------------------------
endif #RULES_MK_INCLUDED
#------------------------------------------------------------------------------
