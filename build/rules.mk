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

ifdef TIERS
   SUBDIRS += $(foreach tier,$(TIERS),$(tier_$(tier)_dirs))

default all alldep:: create_dirs $(SUBMAKEFILES)
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

export_tier_%:
	@echo "TIER PASS: $* export"
	$(EXIT_ON_ERROR) \
    $(foreach dir,$(tier_$*_dirs),$(MAKE) -C $(dir) export; ) true

libs_tier_%:
	@echo "TIER PASS: $* libs"
	$(EXIT_ON_ERROR) \
    $(foreach dir,$(tier_$*_dirs),$(MAKE) -C $(dir) libs; ) true

# This dependency listing is technically incorrect, in that it states that
# _all_ the tiers are dependent on the makefiles of _all_ the tiers, not just
# the tier you're actually building. We did this to avoid spawning a (99% of
# the time) useless make invocation just to check the makefiles of the specific
# tier we're building. Plus, re-generating makefiles is pretty cheap, even if
# it's all of them for all the tier_dirs.
$(foreach tier,$(TIERS),tier_$(tier)):: $(foreach tier,$(TIERS),$(if $(tier_$(tier)_dirs),$(addsuffix /Makefile,$(tier_$(tier)_dirs))))
	@echo "BUILDING $(patsubst tier_%,%,$@) TIER; directories: $($@_dirs)"
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

include $(topsrcdir)/build/oldrules.mk

## oldrules.mk translations ##

#------------------------------------------------------------------------------
# Rules for Makefile generation
#------------------------------------------------------------------------------

# SUBMAKEFILES: List of Makefiles for next level down.
#   This is used to update or create the Makefiles before invoking them.
SUBMAKEFILES += $(addsuffix /Makefile, $(SUBDIRS))

makefiles: $(SUBMAKEFILES)
	+$(LOOP_OVER_SUBDIRS)

$(SUBMAKEFILES): % : $(srcdir)/%.in
	$(PERL) $(MOZSDK_SCRIPTS_DIR)/make-makefile -t $(topsrcdir) -d $(DEPTH) $@

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

#------------------------------------------------------------------------------
# rules for C++ compilation
#------------------------------------------------------------------------------

# CPP_SRCS - a list of .cpp files to be compiled
#
# CPP_INCLUDES       - an override of the default include dirs to pass to 
#                      the compiler
# CPP_EXTRA_INCLUDES - a list of additional include dirs 
# CPP_RAW_INCLUDES   - a list of additional include dirs that don't get 
#                      processed designed to be the target for cflags vars 
#                      that are generated by pkg-config
#
# CPP_FLAGS          - an override of the default flags to pass to the compiler
# CPP_EXTRA_FLAGS    - a list of additional flags to pass to the compiler
#
# CPP_DEFS - a override of the default defines to pass to the compiler with 
#            -D added
# CPP_EXTRA_DEFS - a list of additional defines with -D to pass to the compiler
#

CPP_DEFAULT_INCLUDES = $(MOZSDK_INCLUDE_DIR) \
                       $(MOZSDK_INCLUDE_DIR)/nspr \
                       $(MOZSDK_INCLUDE_DIR)/xpcom \
                       $(MOZSDK_INCLUDE_DIR)/string \
                       $(NULL)

ifdef CPP_FLAGS
   OUR_CPP_FLAGS = $(CPP_FLAGS)
else
   OUR_CPP_FLAGS = $(CXXFLAGS) $(CPP_EXTRA_FLAGS)
   ifeq (macosx,$(SB_PLATFORM))
      OUR_CPP_FLAGS += -isysroot $(SB_MACOSX_SDK)
   endif
endif

ifdef CPP_DEFS
   OUR_CPP_DEFS = $(CPP_DEFS)
else
   OUR_CPP_DEFS = $(ACDEFINES) $(CPP_EXTRA_DEFS)
endif

ifdef CPP_INCLUDES
   OUR_CPP_INCLUDES = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX),$(addprefix $(CFLAGS_INCLUDE_PREFIX),$(CPP_INCLUDES)))
else
   OUR_CPP_INCLUDES = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX),$(addprefix $(CFLAGS_INCLUDE_PREFIX),$(CPP_EXTRA_INCLUDES) $(CPP_DEFAULT_INCLUDES)))
   OUR_CPP_INCLUDES += $(CPP_RAW_INCLUDES)
endif

ifeq (windows,$(SB_PLATFORM))
   COMPILER_OUTPUT_FLAG = -Fo$@
else
   COMPILER_OUTPUT_FLAG = -o $@
endif

%$(OBJ_SUFFIX): %.cpp
	$(CXX) $(COMPILER_OUTPUT_FLAG) $(OUR_CPP_FLAGS) $(OUR_CPP_DEFS) $(OUR_CPP_INCLUDES) $<

%.i: %.cpp
	$(CXX) $(COMPILER_OUTPUT_FLAG) $(CFLAGS_PREPROCESS) $(OUR_CPP_FLAGS) $(OUR_CPP_DEFS) $(OUR_CPP_INCLUDES) $<

%.s: %.cpp
	$(CXX) $(COMPILER_OUTPUT_FLAG) $(CFLAGS_ASSEMBLER) $(OUR_CPP_FLAGS) $(OUR_CPP_DEFS) $(OUR_CPP_INCLUDES) $<


#------------------------------------------------------------------------------
# Rules for pre-processed resources
#------------------------------------------------------------------------------
#
#  A target for pre-processing a list of files and a directory for those files
#  to end up at.
#
#  SONGBIRD_PP_RESOURCES - The list of files to preprocess, the target assumes
#                          that all the files in with ".in"
#
#  SONGBIRD_PP_DIR       - The target directory to put the pre-processed file
#                          list in $(SONGBIRD_PP_RESOURCES).
#
#  PP_RESOURCES_STRIP_SUFFIX  - The suffix of the files to be preprocessed; 
#                               defaults to ".in", but can be most anything,
#                               including empty.
#  
#  RESOURCES_PPFLAGS     - Command-line flags to pass to the preprocessor
#
#  PPDEFINES             - Extra definitions to pass to the preprocessor (in
#                          the form of -DFOO="bar")
#

ifeq (windows,$(SB_PLATFORM))
   RESOURCES_PPFLAGS += --line-endings=crlf
endif

ifndef PP_RESOURCES_STRIP_SUFFIX
   PP_RESOURCES_STRIP_SUFFIX = .in
endif

ifndef SONGBIRD_PP_DIR
   SONGBIRD_PP_DIR = $(CURDIR)
endif

GENERATED_PP_DEPS = $(addprefix $(SONGBIRD_PP_DIR)/,$(foreach f,$(SONGBIRD_PP_RESOURCES),$(patsubst %$(PP_RESOURCES_STRIP_SUFFIX),%,$(notdir $f))))

$(GENERATED_PP_DEPS): $(SONGBIRD_PP_RESOURCES)
   ifeq (,$(wildcard $(SONGBIRD_PP_DIR)))
	   $(MKDIR) $(SONGBIRD_PP_DIR)
   endif
	@for item in $(SONGBIRD_PP_RESOURCES); do \
      target=$(SONGBIRD_PP_DIR)/`basename $$item $(PP_RESOURCES_STRIP_SUFFIX)`; \
      echo Preprocessing $$item into $$target...; \
      $(RM) -f $$target; \
      $(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl \
       $(ACDEFINES) $(RESOURCES_PPFLAGS) \
       $(PPDEFINES) -- $$item > $$target; \
   done

export:: $(GENERATED_PP_DEPS)

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
