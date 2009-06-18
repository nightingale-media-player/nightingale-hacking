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

# Right now this system is not compatible with parallel make.
.NOTPARALLEL: all clean libs export

###############################################################################

LOOP_OVER_SUBDIRS = \
   @$(EXIT_ON_ERROR) \
   $(foreach dir,$(SUBDIRS), $(MAKE) -C $(dir) $@; ) true

# MAKE_DIRS: List of directories to build while looping over directories.
ifneq (,$(OBJS)$(XPIDLSRCS)$(SDK_XPIDLSRCS)$(SIMPLE_PROGRAMS))
   MAKE_DIRS += $(MDDEPDIR)
   GARBAGE_DIRS += $(MDDEPDIR)
endif

ifdef DYNAMIC_LIB
   ifneq (,$(DISABLE_IMPLICIT_NAMING))
      OUR_DYNAMIC_LIB = $(DYNAMIC_LIB)
   else
      OUR_DYNAMIC_LIB = $(DYNAMIC_LIB)$(DEBUG:%=_d)$(DLL_SUFFIX)
   endif
endif

ifdef STATIC_LIB
   ifneq (,$(DISABLE_IMPLICIT_NAMING))
      OUR_STATIC_LIB = $(STATIC_LIB)
   else
      OUR_STATIC_LIB = $(STATIC_LIB)$(DEBUG:%=_d)$(LIB_SUFFIX)
   endif
endif

ifdef SIMPLE_PROGRAM
   ifneq (,$(DISABLE_IMPLICIT_NAMING))
      OUR_SIMPLE_PROGRAM = $(SIMPLE_PROGRAM)
   else
      OUR_SIMPLE_PROGRAM = $(SIMPLE_PROGRAM)$(DEBUG:%=_d)$(BIN_SUFFIX)
   endif
endif

# SUBMAKEFILES: List of Makefiles for next level down.
#   This is used to update or create the Makefiles before invoking them.
SUBMAKEFILES += $(addsuffix /Makefile, $(SUBDIRS))

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
   $(COMPILER_GARBAGE) \
   $(XPIDL_HEADERS) $(XPIDL_TYPELIBS) $(XPIDL_MODULE) \
   $(OUR_DYNAMIC_LIB_OBJS) $(OUR_DYNAMIC_LIB) \
   $(OUR_DYNAMIC_LIB_OBJS:$(OBJ_SUFFIX)=.s) \
   $(OUR_DYNAMIC_LIB_OBJS:$(OBJ_SUFFIX)=.i) \
   $(OUR_STATIC_LIB_OBJS) $(OUR_STATIC_LIB) \
   $(OUR_STATIC_LIB_OBJS:$(OBJ_SUFFIX)=.s) \
   $(OUR_STATIC_LIB_OBJS:$(OBJ_SUFFIX)=.i) \
   $(GENERATED_PP_DEPS) \
   $(SIMPLE_PROGRAM_OBJS) $(SIMPLE_PROGRAM) \
   LOGS TAGS a.out

ifeq (windows,$(SB_PLATFORM))
   ALL_TRASH += \
    $(OUR_DYNAMIC_LIB:$(DLL_SUFFIX)=.pdb) \
    $(OUR_DYNAMIC_LIB:$(DLL_SUFFIX)=.lib) \
    $(OUR_DYNAMIC_LIB:$(DLL_SUFFIX)=.exp) \
    $(OUR_DYNAMIC_LIB).manifest \
    $(OUR_SIMPLE_PROGRAM:$(BIN_SUFFIX)=.pdb) \
    $(OUR_SIMPLE_PROGRAM:$(BIN_SUFFIX)=.lib) \
    $(OUR_SIMPLE_PROGRAM:$(BIN_SUFFIX)=.exp) \
    $(OUR_SIMPLE_PROGRAM).manifest \
    $(NULL)
endif

clean:: $(SUBMAKEFILES)
	-$(RM) $(ALL_TRASH)
	+$(LOOP_OVER_SUBDIRS)

distclean:: FORCE
	$(RM) -r $(SONGBIRD_DISTDIR)

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

libs:: $(OUR_STATIC_LIB) $(OUR_DYNAMIC_LIB) $(OUR_SIMPLE_PROGRAM) $(SONGBIRD_COMPONENTS) $(JAR_MANIFEST) $(MAKE_JAR_DEP) copy_sb_tests make_xpi
ifndef NO_DIST_INSTALL
   ifdef SIMPLE_PROGRAM
	   $(INSTALL_PROG) $(OUR_SIMPLE_PROGRAM) $(FINAL_TARGET)
   endif
   ifndef EXTENSION_STAGE_DIR
   ifdef DYNAMIC_LIB
      ifdef IS_COMPONENT
	      $(INSTALL_PROG) $(OUR_DYNAMIC_LIB) $(SONGBIRD_COMPONENTSDIR)/
      else
	      $(INSTALL_PROG) $(OUR_DYNAMIC_LIB) $(SONGBIRD_LIBDIR)/
      endif
   endif
   endif
endif # !NO_DIST_INSTALL

include $(topsrcdir)/build/oldrules.mk

## oldrules.mk translations ##

#------------------------------------------------------------------------------
# Rules for Makefile generation
#------------------------------------------------------------------------------

makefiles: $(SUBMAKEFILES)
	+$(LOOP_OVER_SUBDIRS)

Makefile: $(srcdir)/Makefile.in
	$(PERL) $(MOZSDK_SCRIPTS_DIR)/make-makefile -t $(topsrcdir) -d $(DEPTH) $@

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
# Common compiler flags
#------------------------------------------------------------------------------

ifeq (windows,$(SB_PLATFORM))
   COMPILER_OUTPUT_FLAG = -Fo$@
else
   COMPILER_OUTPUT_FLAG = -o $@
endif

#------------------------------------------------------------------------------
# rules for C++ compilation
#------------------------------------------------------------------------------
#
# CPP_SRCS           - a list of .cpp files to be compiled
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
# CPP_DEFS           - a override of the default defines to pass to the 
#                      compiler with -D added
# CPP_EXTRA_DEFS     - a list of additional defines with -D to pass to the 
#                      compiler
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

%$(OBJ_SUFFIX): %.cpp
	$(CXX) $(COMPILER_OUTPUT_FLAG) $(OUR_CPP_FLAGS) $(OUR_CPP_DEFS) $(OUR_CPP_INCLUDES) $<

%.i: %.cpp
	$(CXX) $(COMPILER_OUTPUT_FLAG) $(CFLAGS_PREPROCESS) $(OUR_CPP_FLAGS) $(OUR_CPP_DEFS) $(OUR_CPP_INCLUDES) $<

%.s: %.cpp
	$(CXX) $(COMPILER_OUTPUT_FLAG) $(CFLAGS_ASSEMBLER) $(OUR_CPP_FLAGS) $(OUR_CPP_DEFS) $(OUR_CPP_INCLUDES) $<

#------------------------------------------------------------------------------
# rules for Objective C compilation
#------------------------------------------------------------------------------
#
# CMM_SRCS           - a list of .mm files to be compiled
#
# CMM_INCLUDES       - an override of the default include dirs to pass to 
#                      the compiler
# CMM_EXTRA_INCLUDES - a list of additional include dirs 
# CMM_RAW_INCLUDES   - a list of additional include dirs that don't get 
#                      processed designed to be the target for cflags vars 
#                      that are generated by pkg-config
#
# CMM_FLAGS          - an override of the default flags to pass to the compiler
# CMM_EXTRA_FLAGS    - a list of additional flags to pass to the compiler
#
# CMM_DEFS - a override of the default defines to pass to the compiler with 
#            -D added
# CMM_EXTRA_DEFS - a list of additional defines with -D to pass to the compiler
#

CMM_DEFAULT_INCLUDES = $(MOZSDK_INCLUDE_DIR) \
                       $(MOZSDK_INCLUDE_DIR)/nspr \
                       $(MOZSDK_INCLUDE_DIR)/xpcom \
                       $(MOZSDK_INCLUDE_DIR)/string \
                       $(NULL)

ifdef CMM_FLAGS
   OUR_CMM_FLAGS = $(CMM_FLAGS)
else
   OUR_CMM_FLAGS = $(CMMFLAGS) $(CMM_EXTRA_FLAGS)
   ifeq (macosx,$(SB_PLATFORM))
      OUR_CMM_FLAGS += -isysroot $(SB_MACOSX_SDK)
   endif
endif

ifdef CMM_DEFS
   OUR_CMM_DEFS = $(CMM_DEFS)
else
   OUR_CMM_DEFS = $(ACDEFINES) $(CMM_EXTRA_DEFS)
endif

ifdef CMM_INCLUDES
   OUR_CMM_INCLUDES = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX),$(addprefix $(CFLAGS_INCLUDE_PREFIX),$(CMM_INCLUDES)))
else
   OUR_CMM_INCLUDES = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX),$(addprefix $(CFLAGS_INCLUDE_PREFIX),$(CMM_EXTRA_INCLUDES) $(CMM_DEFAULT_INCLUDES)))
   OUR_CMM_INCLUDES += $(CMM_RAW_INCLUDES)
endif

%$(OBJ_SUFFIX): %.mm
	$(CXX) $(COMPILER_OUTPUT_FLAG) $(OUR_CMM_FLAGS) $(OUR_CMM_DEFS) $(OUR_CMM_INCLUDES) $<

%.i: %.mm
	$(CXX) $(COMPILER_OUTPUT_FLAG) $(CFLAGS_PREPROCESS) $(OUR_CMM_FLAGS) $(OUR_CMM_DEFS) $(OUR_CMM_INCLUDES) $<

%.s: %.mm
	$(CXX) $(COMPILER_OUTPUT_FLAG) $(CFLAGS_ASSEMBLER) $(OUR_CMM_FLAGS) $(OUR_CMM_DEFS) $(OUR_CMM_INCLUDES) $<


#------------------------------------------------------------------------------
# rules for C compilation
#------------------------------------------------------------------------------
#
# C_SRCS           - a list of .c files to be compiled
#
# C_INCLUDES       - an override of the default include dirs to pass to 
#                    the compiler
# C_EXTRA_INCLUDES - a list of additional include dirs 
# C_RAW_INCLUDES   - a list of additional include dirs that don't get 
#                    processed designed to be the target for cflags vars 
#                    that are generated by pkg-config
#
# C_FLAGS          - an override of the default flags to pass to the compiler
# C_EXTRA_FLAGS    - a list of additional flags to pass to the compiler
#
# C_DEFS           - a override of the default defines to pass to the 
#                    compiler with -D added
# C_EXTRA_DEFS     - a list of additional defines with -D to pass to the 
#                    compiler
#

C_DEFAULT_INCLUDES = $(NULL)

ifdef C_FLAGS
   OUR_C_FLAGS = $(C_FLAGS)
else
   OUR_C_FLAGS = $(CFLAGS) $(C_EXTRA_FLAGS)
   ifeq (macosx,$(SB_PLATFORM))
      OUR_C_FLAGS += -isysroot $(SB_MACOSX_SDK)
   endif
endif

ifdef C_DEFS
   OUR_C_DEFS = $(C_DEFS)
else
   OUR_C_DEFS = $(ACDEFINES) $(C_EXTRA_DEFS)
endif

ifdef C_INCLUDES
   OUR_C_INCLUDES = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX),$(addprefix $(CFLAGS_INCLUDE_PREFIX),$(C_INCLUDES)))
else
   OUR_C_INCLUDES = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX),$(addprefix $(CFLAGS_INCLUDE_PREFIX),$(C_EXTRA_INCLUDES) $(C_DEFAULT_INCLUDES)))
   OUR_C_INCLUDES += $(C_RAW_INCLUDES)
endif

%$(OBJ_SUFFIX): %.c
	$(CC) $(COMPILER_OUTPUT_FLAG) $(OUR_C_FLAGS) $(OUR_C_DEFS) $(OUR_C_INCLUDES) $<

%.i: %.c
	$(CC) $(COMPILER_OUTPUT_FLAG) $(CFLAGS_PREPROCESS) $(OUR_C_FLAGS) $(OUR_C_DEFS) $(OUR_C_INCLUDES) $<

%.s: %.c
	$(CC) $(COMPILER_OUTPUT_FLAG) $(CFLAGS_ASSEMBLER) $(OUR_C_FLAGS) $(OUR_C_DEFS) $(OUR_C_INCLUDES) $<

#------------------------------------------------------------------------------
# Rules for dynamic (.so/.dylib/.dll) library generation
#------------------------------------------------------------------------------
# DYNAMIC_LIB                - the name of a lib to generate
# DYNAMIC_LIB_OBJS           - an override to the defaut list of object files 
#                              to link into the shared lib
#
# DYNAMIC_LIB_EXTRA_OBJS     - a list of extra objects to add to the library
#
# DYNAMIC_LIB_IMPORT_PATHS   - an overide to the default list of libs to link
# DYNAMIC_LIB_IMPORT_EXTRA_PATHS   - a list of paths to search for libs
#
# DYNAMIC_LIB_IMPORTS        - an override to the default list of libs to link
# DYNAMIC_LIB_EXTRA_IMPORTS  - an additional list of libs to link in
# DYNAMIC_LIB_STATIC_IMPORTS - a list of static libs to link in
#
# DYNAMIC_LIB_FLAGS          - an override to the default linker flags
# DYNAMIC_LIB_EXTRA_FLAGS    - a list of additional flags to pass to the linker
#
# DISABLE_IMPLICIT_LIBNAME   - disables the automatic generation of the 
#                              library name; DYNAMIC_LIB is used directly

DEFAULT_DYNAMIC_LIBS_IMPORTS = xpcomglue_s \
                               nspr4 \
                               xpcom \
                               $(DEFAULT_LIBS) \
                               $(NULL)

DEFAULT_DYNAMIC_LIB_IMPORT_PATHS = $(MOZSDK_LIB_DIR)

ifdef DYNAMIC_LIB # {

ifdef DYNAMIC_LIB_OBJS
   OUR_DYNAMIC_LIB_OBJS = $(DYNAMIC_LIB_OBJS)
else
   OUR_DYNAMIC_LIB_OBJS = $(CPP_SRCS:.cpp=$(OBJ_SUFFIX)) \
                          $(C_SRCS:.c=$(OBJ_SUFFIX)) \
                          $(CMM_SRCS:.mm=$(OBJ_SUFFIX)) \
                          $(DYNAMIC_LIB_EXTRA_OBJS) \
                          $(NULL)
endif

ifdef DYNAMIC_LIB_FLAGS
   OUR_LD_FLAGS = $(DYNAMIC_LIB_FLAGS)
else
   OUR_LD_FLAGS = $(LDFLAGS) $(LDFLAGS_DLL) $(DYNAMIC_LIB_EXTRA_FLAGS)

   ifeq (macosx,$(SB_PLATFORM))
      OUR_LD_FLAGS += -isysroot $(SB_MACOSX_SDK) -Wl,-syslibroot,$(SB_MACOSX_SDK)
       ifdef IS_COMPONENT
          OUR_LD_FLAGS += -bundle
       else 
          OUR_LD_FLAGS += -dynamiclib -install_name @executable_path/$(OUR_DYNAMIC_LIB) -compatibility_version 1 -current_version 1
       endif
   endif
endif

ifdef DYNAMIC_LIB_IMPORT_PATHS
   OUR_LINKER_PATHS_LIST = $(DYNAMIC_LIB_IMPORT_PATHS)
else
   OUR_LINKER_PATHS_LIST = $(DEFAULT_DYNAMIC_LIB_IMPORT_PATHS) $(DYNAMIC_LIB_IMPORT_EXTRA_PATHS)
endif

ifneq (,$(OUR_LINKER_PATHS_LIST))
   OUR_LINKER_PATHS = $(addsuffix $(LDFLAGS_PATH_SUFFIX),\
                       $(addprefix $(LDFLAGS_PATH_PREFIX),\
                       $(foreach dir,$(OUR_LINKER_PATHS_LIST),\
                       $(call normalizepath,$(dir)))))
else
   OUR_LINKER_PATHS = $(NULL)
endif

ifdef DYNAMIC_LIB_IMPORTS
   OUR_LD_IMPORT_LIST = $(DYNAMIC_LIB_IMPORTS)
else
   OUR_LD_IMPORT_LIST = $(DEFAULT_DYNAMIC_LIBS_IMPORTS) \
    $(foreach i, \
    $(DYNAMIC_LIB_EXTRA_IMPORTS), \
    $(if $(filter sb%,$i), \
    $i$(DEBUG:%=_d), \
    $i)) 
endif

OUR_LD_STATIC_IMPORT_LIST = $(foreach import, \
                             $(DYNAMIC_LIB_STATIC_IMPORTS), \
                             $(if $(wildcard $(dir $(import))), \
                             $(import)$(LIB_SUFFIX), \
                             $(addprefix $(SONGBIRD_OBJDIR)/, \
                             $(import)$(DEBUG:%=_d)$(LIB_SUFFIX))))

OUR_LD_IMPORTS = $(OUR_LD_STATIC_IMPORT_LIST) \
                  $(addsuffix $(LDFLAGS_IMPORT_SUFFIX),\
                  $(addprefix $(LDFLAGS_IMPORT_PREFIX),\
                  $(OUR_LD_IMPORT_LIST)))

OUR_LINKER_OUTPUT = $(LDFLAGS_OUT_PREFIX)$@$(LDFLAGS_OUT_SUFFIX)

$(OUR_DYNAMIC_LIB): $(OUR_DYNAMIC_LIB_OBJS)
ifdef FORCE_RANLIB
	$(RANLIB) $(OUR_LINKER_OUTPUT) $(FORCE_RANLIB)
endif
	$(LD) $(OUR_LINKER_OUTPUT) $(OUR_LD_FLAGS) $(OUR_LINKER_PATHS) $(OUR_DYNAMIC_LIB_OBJS) $(OUR_LD_IMPORTS)
endif # } DYNAMIC_LIB

#------------------------------------------------------------------------------
# Rules for static (.a/.lib) library generation
#------------------------------------------------------------------------------
# STATIC_LIB                - the name of a lib to generate
# STATIC_LIB_OBJS           - an override to the defaut list of object files 
#                             to link into the lib
# STATIC_LIB_FLAGS          - an overide t the default list of flags to
#                             pass to the static linker
# STATIC_LIB_EXTRA_FLAGS    - additional flags to pass to the linker
#
# STATIC_LIB_EXTRA_OBJS     - a list of extra objects to add to the library
#

ifdef STATIC_LIB

ifdef STATIC_LIB_FLAGS
   OUR_LINKER_FLAGS = $(STATIC_LIB_FLAGS)
else
   OUR_LINKER_FLAGS = $(ARFLAGS) $(ARFLAGS_LIB) $(STATIC_LIB_EXTRA_FLAGS)
endif

OUR_LINKER_OUTPUT = $(ARFLAGS_OUT_PREFIX)$@$(ARFLAGS_OUT_SUFFIX)

ifdef STATIC_LIB_OBJS
   OUR_STATIC_LIB_OBJS = $(STATIC_LIB_OBJS)
else
   OUR_STATIC_LIB_OBJS = $(CPP_SRCS:.cpp=$(OBJ_SUFFIX)) \
                         $(C_SRCS:.c=$(OBJ_SUFFIX)) \
                         $(CMM_SRCS:.mm=$(OBJ_SUFFIX)) \
                         $(STATIC_LIB_EXTRA_OBJS) \
                         $(NULL)
endif

$(OUR_STATIC_LIB): $(OUR_STATIC_LIB_OBJS)
ifdef USING_RANLIB
	$(RM) $@
endif
	$(AR) $(OUR_LINKER_FLAGS) $(OUR_LINKER_OUTPUT) $(OUR_STATIC_LIB_OBJS)
ifdef USING_RANLIB
	$(RANLIB) $(OUR_LINKER_OUTPUT)
endif

endif


#------------------------------------------------------------------------------
# Rules for creating simple programs
#------------------------------------------------------------------------------
#
#  A target for creating a simple program, consisting of a list of object
#  to link into a program.
# SIMPLE_PROGRAM - the name of a dll to link
# SIMPLE_PROGRAM_OBJS - the object files to link into the dll
# SIMPLE_PROGRAM_IMPORT_PATHS - a list of paths to search for libs
# SIMPLE_PROGRAM_IMPORTS - an override to the default list of libs to link
# SIMPLE_PROGRAM_EXTRA_IMPORTS - an additional list of libs to link
# SIMPLE_PROGRAM_STATIC_IMPORTS - a list of static libs to link
# SIMPLE_PROGRAM_FLAGS - an override to the default linker flags
# SIMPLE_PROGRAM_EXTRA_FLAGS - a list of additional flags to pass to the linker

ifdef SIMPLE_PROGRAM

ifneq (,$(STATIC_LIB)$(DYNAMIC_LIB))
   $(error SIMPLE_PROGRAM cannot be specified together with DYNAMIC_LIB or STATIC_LIB)
endif # STATIC_LIB || DYNAMIC_LIB

CPP_EXTRA_FLAGS += $(CFLAGS_STATIC_LIBC)

ifdef SIMPLE_PROGRAM_FLAGS
   OUR_SIMPLE_PROGRAM_FLAGS = $(SIMPLE_PROGRAM_FLAGS)
else
   OUR_SIMPLE_PROGRAM_FLAGS = $(LDFLAGS) $(LDFLAGS_BIN) $(SIMPLE_PROGRAM_EXTRA_FLAGS)
endif

ifdef SIMPLE_PROGRAM_IMPORTS
   OUR_SIMPLE_PROGRAM_IMPORTS_LIST = $(SIMPLE_PROGRAM_IMPORTS)
else
   OUR_SIMPLE_PROGRAM_IMPORTS_LIST = $(SIMPLE_PROGRAM_DEFAULT_LIBS) $(SIMPLE_PROGRAM_EXTRA_IMPORTS)
endif

ifdef SIMPLE_PROG_OBJS
   OUR_SIMPLE_PROGRAM_OBJS = $(SIMPLE_PROGRAM_OBJS)
else
   OUR_SIMPLE_PROGRAM_OBJS = $(CPP_SRCS:.cpp=$(OBJ_SUFFIX)) \
                             $(C_SRCS:.c=$(OBJ_SUFFIX)) \
                             $(CMM_SRCS:.mm=$(OBJ_SUFFIX)) \
                             $(SIMPLE_PROGRAM_EXTRA_OBJS) \
                             $(NULL)
endif

ifdef SIMPLE_PROGRAM_IMPORTS
   OUR_SIMPLE_PROGRAM_IMPORTS_LIST = $(SIMPLE_PROGRAM_IMPORTS)
else
   OUR_SIMPLE_PROGRAM_IMPORTS_LIST = $(DEFAULT_LIBS) $(SIMPLE_PROGRAM_EXTRA_IMPORTS)
endif

ifdef SIMPLE_PROGRAM_STATIC_IMPORTS
   ifeq (windows,$(SB_PLATFORM))
      OUR_SIMPLE_PROGRAM_IMPORTS_LIST += $(SIMPLE_PROGRAM_STATIC_IMPORTS)
   else
      OUR_SIMPLE_PROGRAM_OBJS += $(addsuffix $(LIB_SUFFIX),$(SIMPLE_PROGRAM_STATIC_IMPORTS))
   endif
endif

OUR_LINKER_IMPORTS = $(addsuffix $(LDFLAGS_IMPORT_SUFFIX), \
                      $(addprefix $(LDFLAGS_IMPORT_PREFIX), \
                      $(OUR_SIMPLE_PROGRAM_IMPORTS_LIST)))

ifdef SIMPLE_PROGRAM_IMPORT_PATHS
   OUR_SIMPLE_PROGRAM_LINKER_PATHS = $(addsuffix $(LDFLAGS_PATH_SUFFIX), \
                                      $(addprefix $(LDFLAGS_PATH_PREFIX), \
                                      $(foreach dir, \
                                      $(SIMPLE_PROGRAM_IMPORT_PATHS),\
                                      $(call normalizepath,$(dir)))))
endif

OUR_SIMPLE_PROGRAM_OUT = $(LDFLAGS_OUT_PREFIX)$@$(LDFLAGS_OUT_SUFFIX)

$(OUR_SIMPLE_PROGRAM): $(OUR_SIMPLE_PROGRAM_OBJS)
	$(LD) $(OUR_SIMPLE_PROGRAM_OUT) $(OUR_SIMPLE_PROGRAM_FLAGS) $(OUR_SIMPLE_PROGRAM_LINKER_PATHS) $(OUR_SIMPLE_PROGRAM_OBJS) $(OUR_LINKER_IMPORTS)

endif

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

echo-variable-%:
	@echo $($*)

echo-tiers:
	@echo $(TIERS)

echo-subdirs:
	@echo $(SUBDIRS)

echo-module:
	@echo $(MODULE)

FORCE:

# Cancel these implicit rules

%: %,v

%: RCS/%,v

%: RCS/%

%: s.%

%: SCCS/s.%

# Re-define the list of default suffixes, so gmake won't have to churn through
# hundreds of built-in suffix rules for stuff we don't need.

#.SUFFIXES:

.PHONY: $(SUBDIRS) FORCE libs export

include $(topsrcdir)/build/file-autotargets.mk

#------------------------------------------------------------------------------
endif #RULES_MK_INCLUDED
#------------------------------------------------------------------------------
