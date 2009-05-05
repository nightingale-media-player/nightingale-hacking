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

# include config.mk to pick up extra variables

ifneq (,$(SB_ENABLE_STATIC))
  ifneq (,$(LIBSONGBIRD_COMPONENT))
    STATIC_LIB:=$(DYNAMIC_LIB:$(DLL_SUFFIX)=$(LIB_SUFFIX))
    DYNAMIC_LIB=
    STATIC_LIB_OBJS=$(DYNAMIC_LIB_OBJS)
  else
    # if the component does not support --enable-static, pretend it's not set
    # so that the files end up in the right place
    SB_ENABLE_STATIC=
  endif
endif

ifeq (macosx,$(SB_PLATFORM))
  SB_DYLD_LIBRARY_PATH = $(DEPS_DIR)/libIDL/$(SB_CONFIGURATION)/lib:$(DEPS_DIR)/glib/$(SB_CONFIGURATION)/lib:$(DEPS_DIR)/gettext/$(SB_CONFIGURATION)/lib
  export DYLD_LIBRARY_PATH = $(SB_DYLD_LIBRARY_PATH)
endif

#
# Collect a list of rules to run. We use two variables so that 'make clean'
# does what you'd expect.
#

ifdef XPIDL_SRCS
XPIDL_HEADER_SRCS += $(XPIDL_SRCS)
XPIDL_TYPELIB_SRCS += $(XPIDL_SRCS)
endif

# Right now this system is not compatible with parallel make.
.NOTPARALLEL : all clean

#------------------------------------------------------------------------------
# Utilities
#------------------------------------------------------------------------------

# from mozilla/config/rules.mk (the Java rules section)
# note that an extra slash was added between root-path and non-root-path to
# account for non-standard mount points in msys
# (C:/ vs C:/foo with missing trailing slash)
# Cygwin and MSYS have their own special path form, but manifest tool expects
# them to be in the DOS form (i.e. e:/builds/...).  This function
# does the appropriate conversion on Windows, but is a noop on other systems.
ifeq (windows,$(SB_PLATFORM))
  ifneq (,$(CYGWIN_WRAPPER))
    normalizepath = $(foreach p,$(1),$(shell cygpath -m $(p)))
  else
    # assume MSYS
    #  We use 'pwd -W' to get DOS form of the path.  However, since the given path
    #  could be a file or a non-existent path, we cannot call 'pwd -W' directly
    #  on the path.  Instead, we extract the root path (i.e. "c:/"), call 'pwd -W'
    #  on it, then merge with the rest of the path.
    root-path = $(shell echo $(1) | sed -e "s|\(/[^/]*\)/\?\(.*\)|\1|")
    non-root-path = $(shell echo $(1) | sed -e "s|\(/[^/]*\)/\?\(.*\)|\2|")
    normalizepath = $(if $(filter /%,$(1)),$(shell cd $(call root-path,$(1)) && pwd -W)/$(call non-root-path,$(1)),$(1))
  endif
else
  normalizepath = $(1)
endif

#------------------------------------------------------------------------------
# Redefine these for extensions
#------------------------------------------------------------------------------

ifdef EXTENSION_STAGE_DIR
   SONGBIRD_CHROMEDIR = $(EXTENSION_STAGE_DIR)/chrome
   SONGBIRD_COMPONENTSDIR = $(EXTENSION_STAGE_DIR)/components
   SONGBIRD_DEFAULTSDIR = $(EXTENSION_STAGE_DIR)/defaults
   SONGBIRD_PREFERENCESDIR = $(EXTENSION_STAGE_DIR)/defaults/preferences
   SONGBIRD_PLUGINSDIR = $(EXTENSION_STAGE_DIR)/plugins
   SONGBIRD_SEARCHPLUGINSDIR = $(EXTENSION_STAGE_DIR)/searchplugins
   SONGBIRD_SCRIPTSDIR = $(EXTENSION_STAGE_DIR)/scripts
   SONGBIRD_JSMODULESDIR = $(EXTENSION_STAGE_DIR)/jsmodules
endif

#------------------------------------------------------------------------------
# Update Makefiles
#------------------------------------------------------------------------------

# In GNU make 3.80, makefiles must use the /cygdrive syntax, even if we're
# processing them with AS perl. See bmo 232003
ifdef AS_PERL
CYGWIN_TOPSRCDIR = -nowrap -p $(topsrcdir) -wrap
endif

# SUBMAKEFILES: List of Makefiles for next level down.
#   This is used to update or create the Makefiles before invoking them.
# XXX - defined in rules.mk now
#ifneq ($(SUBDIRS),)
#SUBMAKEFILES := $(addsuffix /Makefile, $(SUBDIRS))
#endif

$(SUBMAKEFILES): % : $(srcdir)/%.in
	$(PERL) $(MOZSDK_SCRIPTS_DIR)/make-makefile -t $(topsrcdir) -d $(DEPTH) $(CYGWIN_TOPSRCDIR) $@

Makefile: Makefile.in
	@$(PERL) $(MOZSDK_SCRIPTS_DIR)/make-makefile -t $(topsrcdir) -d $(DEPTH) $(CYGWIN_TOPSRCDIR)

makefiles: $(SUBMAKEFILES)
ifneq (,$(SUBDIRS))
	@for d in $(SUBDIRS); do \
    $(MAKE) -C $$d $@; \
	done
endif

#------------------------------------------------------------------------------
# rules for c++ compilation
#------------------------------------------------------------------------------

# cpp_srcs - a list of .cpp files to be compiled
# cpp_includes - a list of include dirs
# cpp_extra_includes - a list of additional include dirs that don't get processed
#         designed to be the target for cflags vars that are generated by pkg-config
# cpp_flags - an override of the default flags to pass to the compiler
# CPP_EXTRA_FLAGS - a list of additional flags to pass to the compiler
# CPP_DEFS - a override of the default defines to pass to the compiler with -D added
# CPP_EXTRA_DEFS - a list of additional defines with -D to pass to the compiler

ifdef CPP_SRCS

ifdef CPP_FLAGS
compile_flags = $(CPP_FLAGS)
else
compile_flags = $(CXXFLAGS) $(CPP_EXTRA_FLAGS)
endif

ifeq (macosx,$(SB_PLATFORM))
  compile_flags += -isysroot $(SB_MACOSX_SDK)
endif

ifdef CPP_DEFS
compile_defs = $(CPP_DEFS)
else
compile_defs = $(ACDEFINES)
ifdef CPP_EXTRA_DEFS
compile_defs += $(CPP_EXTRA_DEFS)
endif
endif

ifdef CPP_INCLUDES
compile_includes_temp = $(addprefix $(CFLAGS_INCLUDE_PREFIX), $(CPP_INCLUDES))
compile_includes = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX), $(compile_includes_temp))
ifdef CPP_EXTRA_INCLUDES
compile_includes += $(CPP_EXTRA_INCLUDES)
endif
endif

ifneq (,$(SB_ENABLE_STATIC))
compile_defs += -DXPCOM_TRANSLATE_NSGM_ENTRY_POINT
endif # libsongbird_component + --enable-static

#compiler_objects = $(notdir $(CPP_SRCS:.cpp=$(OBJ_SUFFIX)))

ifeq (windows,$(SB_PLATFORM))
   OUTPUT_FLAG = -Fo$@
else
   OUTPUT_FLAG = -o $@
endif


%$(OBJ_SUFFIX): %.cpp
	$(CYGWIN_WRAPPER) $(CXX) $(OUTPUT_FLAG) $(compile_flags) $(compile_defs) $(compile_includes) $<

%.i: %.cpp
	$(CYGWIN_WRAPPER) $(CXX) $(compile_flags) $(compile_defs) $(compile_includes) $(CFLAGS_PREPROCESS) $<

endif #CPP_SRCS


#------------------------------------------------------------------------------
# Rules for Objective C++ compilation
#------------------------------------------------------------------------------

# CMM_SRCS - a list of .mm files to be compiled
# CMM_INCLUDES - a list of include dirs
# CMM_EXTRA_INCLUDES - a list of additional include dirs that don't get processed
#         designed to be the target for CFLAGS vars that are generated by pkg-config
# CMM_FLAGS - an override of the default flags to pass to the compiler
# CMM_EXTRA_FLAGS - a list of additional flags to pass to the compiler
# CMM_DEFS - a override of the default defines to pass to the compiler with -D added
# CMM_EXTRA_DEFS - a list of additional defines with -D to pass to the compiler

ifdef CMM_SRCS

ifdef CMM_FLAGS
mm_compile_flags = $(CMM_FLAGS)
else
mm_compile_flags = $(CMMFLAGS)
ifdef CMM_EXTRA_FLAGS
mm_compile_flags += $(CMM_EXTRA_FLAGS)
endif
endif

ifeq (macosx,$(SB_PLATFORM))
  mm_compile_flags += -isysroot $(SB_MACOSX_SDK)
endif

ifdef CMM_DEFS
mm_compile_defs = $(CMM_DEFS)
else
mm_compile_defs = $(ACDEFINES)
ifdef CMM_EXTRA_DEFS
mm_compile_defs += $(CMM_EXTRA_DEFS)
endif
endif

ifdef CMM_INCLUDES
mm_compile_includes_temp = $(addprefix $(CFLAGS_INCLUDE_PREFIX), $(CMM_INCLUDES))
mm_compile_includes = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX), $(mm_compile_includes_temp))
ifdef CMM_EXTRA_INCLUDES
mm_compile_includes += $(CMM_EXTRA_INCLUDES)
endif
endif

ifneq (,$(SB_ENABLE_STATIC))
mm_compile_defs += -DXPCOM_TRANSLATE_NSGM_ENTRY_POINT
endif # libsongbird_component + --enable-static

mm_compiler_objects = $(CMM_SRCS:.mm=$(OBJ_SUFFIX))

%$(OBJ_SUFFIX): %.mm
	$(CYGWIN_WRAPPER) $(CXX) $(mm_compile_flags) $(mm_compile_defs) $(mm_compile_includes) $<

#mm_compile: $(mm_compiler_objects)

#mm_clean:
#	$(CYGWIN_WRAPPER) $(RM) -f $(mm_compiler_objects)

# .PHONY : mm_compile mm_clean

endif #CMM_SRCS

#-----------------------

# DYNAMIC_LIB - the name of a dll to link
# DYNAMIC_LIB_OBJS - the object files to link into the dll
# DYNAMIC_LIB_IMPORT_PATHS - a list of paths to search for libs
# DYNAMIC_LIB_IMPORTS - an override to the default list of libs to link
# DYNAMIC_LIB_EXTRA_IMPORTS - an additional list of libs to link
# DYNAMIC_LIB_STATIC_IMPORTS - a list of static libs to link
# DYNAMIC_LIB_FLAGS - an override to the default linker flags
# DYNAMIC_LIB_EXTRA_FLAGS - a list of additional flags to pass to the linker

ifdef DYNAMIC_LIB

# preedTODO: Remove definition of DYNAMIC_LIB_OBJS from all the Makefile.ins
ifneq (,$(DYNAMIC_LIB_OBJS))
   $(info WARNING: $(CURDIR) defines DYNAMIC_LIB_OBJS)
else
#preedTODO: define variable to inhibit the autogeneration of these
ifneq (,$(CPP_SRCS))
   DYNAMIC_LIB_OBJS += $(CPP_SRCS:.cpp=$(OBJ_SUFFIX))
endif

ifneq (,$(C_SRCS))
   DYNAMIC_LIB_OBJS += $(C_SRCS:.c=$(OBJ_SUFFIX))
endif

ifneq (,$(CMM_SRCS))
   DYNAMIC_LIB_OBJS += $(CMM_SRCS:.mm=$(OBJ_SUFFIX))
endif

endif # DYNAMIC_LIB_OBJ check

ifdef DYNAMIC_LIB_FLAGS
linker_flags = $(DYNAMIC_LIB_FLAGS)
else

ifeq (macosx,$(SB_PLATFORM))

   #DYNAMIC_LIB_OBJS += $(notdir $(CPP_SRCS:.mm=$(OBJ_SUFFIX)))

ifdef DYNAMIC_LIB_IS_NOT_COMPONENT
LDFLAGS_DLL += -install_name @executable_path/$(DYNAMIC_LIB) -compatibility_version 1 -current_version 1
else
# Override LDFLAGS_DLL entirely. The makefile can still specify flags with the
# LDFLAGS and DYNAMIC_LIB_EXTRA_FLAGS variables.
LDFLAGS_DLL = -bundle
endif
endif

linker_flags = $(LDFLAGS) $(LDFLAGS_DLL)

ifeq (macosx,$(SB_PLATFORM))
  linker_flags += -isysroot $(SB_MACOSX_SDK) -Wl,-syslibroot,$(SB_MACOSX_SDK)
endif

ifdef DYNAMIC_LIB_EXTRA_FLAGS
linker_flags += $(DYNAMIC_LIB_EXTRA_FLAGS)
endif
endif

ifdef DYNAMIC_LIB_IMPORTS
linker_imports_temp1 = $(DYNAMIC_LIB_IMPORTS)
else
linker_imports_temp1 = $(DEFAULT_LIBS)
ifdef DYNAMIC_LIB_EXTRA_IMPORTS
linker_imports_temp1 += $(DYNAMIC_LIB_EXTRA_IMPORTS)
endif
endif

linker_objs = $(DYNAMIC_LIB_OBJS)

ifeq (windows,$(SB_PLATFORM))
ifdef DYNAMIC_LIB_STATIC_IMPORTS
linker_imports_temp1 += $(DYNAMIC_LIB_STATIC_IMPORTS)
endif
else
ifdef DYNAMIC_LIB_STATIC_IMPORTS
static_objs = $(addsuffix $(LIB_SUFFIX),$(DYNAMIC_LIB_STATIC_IMPORTS))
linker_objs += $(static_objs)
endif
endif

linker_imports_temp2 = $(addprefix $(LDFLAGS_IMPORT_PREFIX), $(linker_imports_temp1))
linker_imports = $(addsuffix $(LDFLAGS_IMPORT_SUFFIX), $(linker_imports_temp2))

ifdef DYNAMIC_LIB_IMPORT_PATHS
linker_paths_temp = $(addprefix $(LDFLAGS_PATH_PREFIX), \
                      $(foreach dir,$(DYNAMIC_LIB_IMPORT_PATHS),$(call normalizepath,$(dir))))
linker_paths = $(addsuffix $(LDFLAGS_PATH_SUFFIX), $(linker_paths_temp))
endif

linker_out = $(LDFLAGS_OUT_PREFIX)$(DYNAMIC_LIB)$(LDFLAGS_OUT_SUFFIX)

ranlib_cmd =
ifdef FORCE_RANLIB
	ranlib_cmd = $(CYGWIN_WRAPPER) $(RANLIB) $(linker_out) $(FORCE_RANLIB)
endif

$(DYNAMIC_LIB): $(DYNAMIC_LIB_OBJS)
	$(ranlib_cmd)
	$(CYGWIN_WRAPPER) $(LD) $(linker_out) $(linker_flags) $(linker_paths) $(linker_objs) $(linker_imports)


endif #DYNAMIC_LIB

#-----------------------

# STATIC_LIB - the name of a lib to link
# STATIC_LIB_OBJS - the object files to link into the lib
# STATIC_LIB_FLAGS - an override to the default linker flags
# STATIC_LIB_EXTRA_FLAGS - a list of additional flags to pass to the linker
# LIBSONGBIRD_COMPONENT - indicates that this is a static component;
#                         the value must be the same as the first parameter used
#                         for NS_IMPL_NSGETMODULE

ifdef STATIC_LIB

ifdef STATIC_LIB_FLAGS
linker_flags = $(STATIC_LIB_FLAGS)
else
linker_flags = $(ARFLAGS) $(ARFLAGS_LIB)
ifdef STATIC_LIB_EXTRA_FLAGS
linker_flags += $(STATIC_LIB_EXTRA_FLAGS)
endif
endif

linker_out = $(ARFLAGS_OUT_PREFIX)$(STATIC_LIB)$(ARFLAGS_OUT_SUFFIX)

static_lib_deps = $(STATIC_LIB_OBJS)

ifdef USING_RANLIB
ranlib_cmd = $(CYGWIN_WRAPPER) $(RANLIB) $(linker_out)
static_lib_deps += lib_clean
else
ranlib_cmd = @echo Not using ranlib
endif

#lib_link: $(static_lib_deps)
$(STATIC_LIB): $(static_lib_deps)
	$(CYGWIN_WRAPPER) $(AR) $(linker_flags) $(linker_out) $(STATIC_LIB_OBJS)
	$(ranlib_cmd)

ifneq (,$(SB_ENABLE_STATIC))
  lib_link: lib_static_list

  ifdef DYNAMIC_LIB_IMPORT_PATHS
    linker_paths_temp2 = $(foreach dir,$(DYNAMIC_LIB_IMPORT_PATHS),$(call normalizepath,$(dir)))
  endif

lib_static_list:
	$(PERL) -I$(MOZSDK_SCRIPTS_DIR) $(MOZSDK_SCRIPTS_DIR)/build-list.pl \
	    $(SONGBIRD_OBJDIR)/components/static/link-names $(LIBSONGBIRD_COMPONENT)
	$(PERL) -I$(MOZSDK_SCRIPTS_DIR) $(MOZSDK_SCRIPTS_DIR)/build-list.pl \
	    $(SONGBIRD_OBJDIR)/components/static/link-libs $(CURDIR)/$(STATIC_LIB)
ifneq (,$(strip $(linker_paths_temp2)))
	for path in $(linker_paths_temp2) ; do \
	  $(PERL) -I$(MOZSDK_SCRIPTS_DIR) $(MOZSDK_SCRIPTS_DIR)/build-list.pl \
	    $(SONGBIRD_OBJDIR)/components/static/link-paths $${path} ;\
	done
endif #linker_paths_temp2
ifneq (,$(strip $(DYNAMIC_LIB_EXTRA_IMPORTS)))
	for import in $(DYNAMIC_LIB_EXTRA_IMPORTS) ; do \
	  $(PERL) -I$(MOZSDK_SCRIPTS_DIR) $(MOZSDK_SCRIPTS_DIR)/build-list.pl \
	    $(SONGBIRD_OBJDIR)/components/static/link-imports $${import} ;\
	done
endif #DYNAMIC_LIB_EXTRA_IMPORTS
ifneq (,$(strip $(DYNAMIC_LIB_STATIC_IMPORTS)))
	for import in $(DYNAMIC_LIB_STATIC_IMPORTS) ; do \
	  $(PERL) -I$(MOZSDK_SCRIPTS_DIR) $(MOZSDK_SCRIPTS_DIR)/build-list.pl \
	    $(SONGBIRD_OBJDIR)/components/static/link-static-imports $${import} ;\
	done
endif # DYNAMIC_LIB_STATIC_IMPORTS
ifneq (,$(strip $(DYNAMIC_LIB_EXTRA_FLAGS)))
	for flag in "$(DYNAMIC_LIB_EXTRA_FLAGS)" ; do \
	  $(PERL) -I$(MOZSDK_SCRIPTS_DIR) $(MOZSDK_SCRIPTS_DIR)/build-list.pl \
	    $(SONGBIRD_OBJDIR)/components/static/link-flags "$${flag}" ;\
	done
endif # DYNAMIC_LIB_EXTRA_FLAGS

endif # static component

lib_clean:
	$(CYGWIN_WRAPPER) $(RM) -f $(STATIC_LIB)

.PHONY : lib_clean lib_static_list

endif #STATIC_LIB

#-----------------------

# SIMPLE_PROGRAM - the name of a dll to link
# SIMPLE_PROGRAM_OBJS - the object files to link into the dll
# SIMPLE_PROGRAM_IMPORT_PATHS - a list of paths to search for libs
# SIMPLE_PROGRAM_IMPORTS - an override to the default list of libs to link
# SIMPLE_PROGRAM_EXTRA_IMPORTS - an additional list of libs to link
# SIMPLE_PROGRAM_STATIC_IMPORTS - a list of static libs to link
# SIMPLE_PROGRAM_FLAGS - an override to the default linker flags
# SIMPLE_PROGRAM_EXTRA_FLAGS - a list of additional flags to pass to the linker

ifdef SIMPLE_PROGRAM

ifneq ($(STATIC_LIB)$(DYNAMIC_LIB),)
$(error SIMPLE_PROGRAM cannot be specified together with DYNAMIC_LIB or STATIC_LIB)
endif # STATIC_LIB || DYNAMIC_LIB

CPP_EXTRA_FLAGS += $(CFLAGS_STATIC_LIBC)

ifdef SIMPLE_PROGRAM_FLAGS
linker_flags = $(SIMPLE_PROGRAM_FLAGS)
else

linker_flags = $(LDFLAGS) $(LDFLAGS_BIN)
ifdef SIMPLE_PROGRAM_EXTRA_FLAGS
linker_flags += $(SIMPLE_PROGRAM_EXTRA_FLAGS)
endif

endif # SIMPLE_PROGRAM_FLAGS

ifdef SIMPLE_PROGRAM_IMPORTS
linker_imports_temp1 = $(SIMPLE_PROGRAM_IMPORTS)
else
linker_imports_temp1 = $(DEFAULT_LIBS)
ifdef SIMPLE_PROGRAM_EXTRA_IMPORTS
linker_imports_temp1 += $(SIMPLE_PROGRAM_EXTRA_IMPORTS)
endif
endif

linker_objs = $(SIMPLE_PROGRAM_OBJS)

ifeq (windows,$(SB_PLATFORM))
ifdef SIMPLE_PROGRAM_STATIC_IMPORTS
linker_imports_temp1 += $(SIMPLE_PROGRAM_STATIC_IMPORTS)
endif
else
ifdef SIMPLE_PROGRAM_STATIC_IMPORTS
static_objs = $(addsuffix $(LIB_SUFFIX),$(SIMPLE_PROGRAM_STATIC_IMPORTS))
linker_objs += $(static_objs)
endif
endif

linker_imports_temp2 = $(addprefix $(LDFLAGS_IMPORT_PREFIX), $(linker_imports_temp1))
linker_imports = $(addsuffix $(LDFLAGS_IMPORT_SUFFIX), $(linker_imports_temp2))

ifdef SIMPLE_PROGRAM_IMPORT_PATHS
linker_paths_temp = $(addprefix $(LDFLAGS_PATH_PREFIX), \
                      $(foreach dir,$(SIMPLE_PROGRAM_IMPORT_PATHS),$(call normalizepath,$(dir))))
linker_paths = $(addsuffix $(LDFLAGS_PATH_SUFFIX), $(linker_paths_temp))
endif

linker_out = $(LDFLAGS_OUT_PREFIX)$(SIMPLE_PROGRAM)$(LDFLAGS_OUT_SUFFIX)

ranlib_cmd =
ifdef FORCE_RANLIB
	ranlib_cmd = $(CYGWIN_WRAPPER) $(RANLIB) $(FORCE_RANLIB)
endif

$(SIMPLE_PROGRAM): $(SIMPLE_PROGRAM_OBJS)
	$(ranlib_cmd)
	$(CYGWIN_WRAPPER) $(LD) $(linker_out) $(linker_flags) $(linker_paths) $(linker_objs) $(linker_imports)
	$(CYGWIN_WRAPPER) $(CHMOD) +x $(SIMPLE_PROGRAM)

exe_clean:
	$(CYGWIN_WRAPPER) $(RM) -f $(SIMPLE_PROGRAM) \
	      $(SIMPLE_PROGRAM:$(BIN_SUFFIX)=.pdb) \
	      $(SIMPLE_PROGRAM:$(BIN_SUFFIX)=.lib) \
	      $(SIMPLE_PROGRAM:$(BIN_SUFFIX)=.exp) \
              $(SIMPLE_PROGRAM).manifest \
	      $(NULL)

.PHONY : exe_clean

endif #SIMPLE_PROGRAM

#------------------------------------------------------------------------------
# Rules for C compilation
#------------------------------------------------------------------------------

# C_SRCS - a list of .cpp files to be compiled
# C_INCLUDES - a list of include dirs
# C_FLAGS - an override of the default flags to pass to the compiler
# C_EXTRA_FLAGS - a list of additional flags to pass to the compiler
# C_DEFS - a override of the default defines to pass to the compiler with -D added
# C_EXTRA_DEFS - a list of additional defines with -D to pass to the compiler

ifdef C_SRCS

ifdef C_FLAGS
c_compile_flags = $(C_FLAGS)
else
c_compile_flags = $(CFLAGS)
ifdef C_EXTRA_FLAGS
c_compile_flags += $(C_EXTRA_FLAGS)
endif
endif

ifeq (macosx,$(SB_PLATFORM))
  c_compile_flags += -isysroot $(SB_MACOSX_SDK)
endif

ifdef C_DEFS
c_compile_defs = $(C_DEFS)
else
c_compile_defs = $(ACDEFINES)
ifdef C_EXTRA_DEFS
c_compile_defs += $(C_EXTRA_DEFS)
endif
endif

ifdef C_INCLUDES
c_compile_includes_temp = $(addprefix $(CFLAGS_INCLUDE_PREFIX), $(C_INCLUDES))
c_compile_includes = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX), $(c_compile_includes_temp))
endif

c_compiler_objects = $(C_SRCS:.c=$(OBJ_SUFFIX))

$(c_compiler_objects) :%$(OBJ_SUFFIX): %.c
	$(CYGWIN_WRAPPER) $(CC) $(c_compile_flags) $(c_compile_defs) $(c_compile_includes) $<

c_compile: $(c_compiler_objects)

c_clean:
	$(CYGWIN_WRAPPER) $(RM) -f $(c_compiler_objects) $(COMPILER_GARBAGE)

.PHONY : c_compile c_clean

endif #C_SRCS

#------------------------------------------------------------------------------
# Rules for XPIDL compilation
#------------------------------------------------------------------------------

# XPIDL_SRCS - a list of idl files to turn into header and typelib files
# XPIDL_HEADER_SRCS - a list of idl files to turn into C++ header files
# XPIDL_TYPELIB_SRCS - a list of idl files to turn into xpt typelib files
# XPIDL_MODULE - the name of an xpt file that will created from linking several
#                other xpt typelibs
# XPIDL_MODULE_TYPELIBS - a list of xpt files to link into the module
# XPIDL_INCLUDES - a list of dirs to search when looking for included idls
# XPIDL_EXTRA_FLAGS - additional flags to send to XPIDL

#ifdef XPIDL_HEADER_SRCS

xpidl_headers = $(XPIDL_HEADER_SRCS:.idl=.h)

xpidl_includes_temp = $(MOZSDK_IDL_DIR) \
                      $(srcdir) \
                      $(XPIDL_INCLUDES) \
                      $(NULL)

xpidl_includes = $(addprefix $(XPIDLFLAGS_INCLUDE), $(xpidl_includes_temp))

xpidl_compile_headers: $(XPIDL_HEADER_SRCS) $(xpidl_headers)

$(xpidl_headers): %.h: %.idl
	$(XPIDL) -m header $(xpidl_includes) $(XPIDL_EXTRA_FLAGS) $<
	$(INSTALL_FILE) $@ $(SONGBIRD_SDKINCLUDEDIR)

xpidl_clean_headers:
	$(CYGWIN_WRAPPER) $(RM) -f $(xpidl_headers)

xpidl_typelibs = $(XPIDL_TYPELIB_SRCS:.idl=.xpt)
xpidl_compile_typelibs: $(XPIDL_TYPELIB_SRCS) $(xpidl_typelibs)

$(xpidl_typelibs): %.xpt: %.idl
	$(XPIDL) -m typelib $(xpidl_includes) $(XPIDL_EXTRA_FLAGS) -e $@ $<
	@echo might call $(INSTALL_FILE) $@ $(SONGBIRD_DISTDIR)/components

xpidl_clean_typelibs:
	$(CYGWIN_WRAPPER) $(RM) -f $(xpidl_typelibs)

#-----------------------

#ifdef XPIDL_MODULE

ifneq (,$(SB_ENABLE_STATIC))
    # in static builds, don't put the xpt in the dist directory
    xpidl_module := $(addprefix $(SONGBIRD_OBJDIR)/components/static/, $(XPIDL_MODULE))
    XPIDL_MODULE =
else
    xpidl_module = $(XPIDL_MODULE)
endif

xpidl_module_typelibs = $(XPIDL_MODULE_TYPELIBS)

# If the collected typelibs are the same (single file) as XPIDL_MODULE, 
# there's no reason to run xpt_link on them (in fact, this creates a circular
# make dependency that gets dropped, but xpt_link clobbers the file in the
# process of trying to link it, and fails anyway.

ifneq ($(strip $(XPIDL_MODULE)),$(strip $(xpidl_module_typelibs)))
$(XPIDL_MODULE): $(xpidl_module_typelibs)
	$(XPTLINK) $(xpidl_module) $(xpidl_module_typelibs)
endif

xpidl_clean_link:
	$(CYGWIN_WRAPPER) $(RM) -f $(xpidl_module)

#.PHONY : xpidl_link xpidl_clean_link

#endif #XPIDL_MODULE

#------------------------------------------------------------------------------
# Rules for moving files around
#------------------------------------------------------------------------------

# SONGBIRD_dir - indicates that the files should be copied into the
#                   $(SONGBIRD_dir) directory
# CLONEDIR - a directory which will hold all the files from the source dir
#             with some pattern matching
# CLONE_FIND_EXP - an expression to pass to 'find' that specifies the type of
#                  files that will be cloned
# CLONE_EXCLUDE_NAME - a list of files that will be excluded from the clone
#                      based on the filename only
# CLONE_EXCLUDE_DIR - a list of directories that will be excluded from the clone

ifdef CLONEDIR
$(warning WARNING: CLONEDIR is DEPRECATED and slated for REMOVAL; you have been warned...)
ifdef CLONE_FIND_EXP

  find_exp = $(CLONE_FIND_EXP)

else
  ifdef CLONE_EXCLUDE_NAME
    clone_exclude_name := $(addsuffix ",$(CLONE_EXCLUDE_NAME))
    clone_exclude_name := $(addprefix ! -name ",$(clone_exclude_name))
  endif
  ifdef CLONE_EXCLUDE_DIR
    clone_exclude_dir := $(addsuffix /*",$(CLONE_EXCLUDE_DIR))
    clone_exclude_dir := $(addprefix ! -wholename "*/,$(clone_exclude_dir))
  endif

  find_exp = -type f ! \
             -path "*.svn*" \
             $(clone_exclude_dir) \
             ! -name "Makefile.in"  \
             ! -name "jar.mn" \
             $(clone_exclude_name) \
             $(NULL)

endif

files_list = $(shell cd $(srcdir) && $(FIND) . $(find_exp))

ifdef files_list
clone_dir_cmd = cd $(srcdir) && \
                $(CYGWIN_WRAPPER) $(CP) -P -f -p --parents $(files_list) $(CLONEDIR) \
                $(NULL)
endif

export:: clone_dir

clone_dir:
	$(warning WARNING: $@ is DEPRECATED and slated for REMOVAL; you have been warned...)
	$(MKDIR) $(CLONEDIR)
	$(clone_dir_cmd)

.PHONY : clone_dir

endif #CLONEDIR

## preedTODO: remove this ifdef
ifndef PP_COMPONENTS_STRIP_SUFFIXES
PP_COMPONENTS_STRIP_SUFFIXES = .in
endif

sb_components_preprocess:
ifdef SONGBIRD_PP_COMPONENTS
ifeq (,$(wildcard $(SONGBIRD_COMPONENTSDIR)))
	$(CYGWIN_WRAPPER) $(MKDIR) -p $(SONGBIRD_COMPONENTSDIR)
endif
ifneq (,$(strip $(SONGBIRD_PP_COMPONENTS)))
	for item in $(SONGBIRD_PP_COMPONENTS); do \
	  target=$(SONGBIRD_COMPONENTSDIR)/`basename $$item $(PP_COMPONENTS_STRIP_SUFFIXES)`; \
	  $(CYGWIN_WRAPPER) $(RM) -f $$target; \
	  $(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl $(ACDEFINES) $(COMPONENTS_PPFLAGS) \
	    $(PPDEFINES) -- $$item > $$target; \
	done
endif
.PHONY : sb_components_preprocess
endif #SONGBIRD_PP_COMPONENTS

#-----------------------


#------------------------------------------------------------------------------
# Rules for Songbird Pre-processed resources
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

## preedTODO: remove this ifdef
ifdef SONGBIRD_PP_RESOURCES
ifeq (windows,$(SB_PLATFORM))
   RESOURCES_PPFLAGS += --line-endings=crlf
endif
ifndef PP_RESOURCES_STRIP_SUFFIXES
PP_RESOURCES_STRIP_SUFFIXES = .in
endif
endif

## preedTODO: Remove the ifdef around this target
sb_resources_preprocess:
ifdef SONGBIRD_PP_RESOURCES
ifndef SONGBIRD_PP_DIR
	$(error $(CURDIR)/Makefile: SONGBIRD_PP_DIR was not defined, use SONGBIRD_PP_DIR)
endif
ifeq (,$(wildcard $(SONGBIRD_PP_DIR)))
	$(CYGWIN_WRAPPER) $(MKDIR) -p $(SONGBIRD_PP_DIR)
endif
ifneq (,$(strip $(SONGBIRD_PP_RESOURCES)))
	for item in $(SONGBIRD_PP_RESOURCES); do \
	  target=$(SONGBIRD_PP_DIR)/`basename $$item $(PP_RESOURCES_STRIP_SUFFIXES)`; \
	  echo $$target; \
	  $(CYGWIN_WRAPPER) $(RM) -f $$target; \
	  $(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl $(ACDEFINES) $(RESOURCES_PPFLAGS) \
	    $(PPDEFINES) -- $$item > $$target; \
	done
endif
endif

clean_sb_resources_preprocess:
ifneq (,$(strip $(SONGBIRD_PP_RESOURCES)))
	for item in $(SONGBIRD_PP_RESOURCES); do \
	  target = $(SONGBIRD_PP_DIR)/`basename $$item`; \
	  $(CYGWIN_WRAPPER) $(RM) -f $$target; \
	done
endif

.PHONY : sb_resources_preprocess clean_sb_resources_preprocess

ifdef SONGBIRD_TEST_COMPONENT
SONGBIRD_TESTSDIR := $(SONGBIRD_TESTSDIR)/$(SONGBIRD_TEST_COMPONENT)
endif #SONGBIRD_TEST_COMPONENT
copy_sb_tests:
ifdef SONGBIRD_TESTS
ifneq (,$(SB_ENABLE_TESTS))
ifeq (,$(wildcard $(SONGBIRD_TESTSDIR)))
	$(CYGWIN_WRAPPER) $(MKDIR) -p $(SONGBIRD_TESTSDIR)
endif
	$(INSTALL_FILE) $(SONGBIRD_TESTS) $(SONGBIRD_TESTSDIR)
endif
.PHONY : copy_sb_tests
endif #SONGBIRD_TESTS

#------------------------------------------------------------------------------
# Rules for preprocessing
#------------------------------------------------------------------------------


# on windows pref files need CRLF line endings
ifeq (windows,$(SB_PLATFORM))
PREF_PPFLAGS = --line-endings=crlf
endif

ifndef PREFERENCES_STRIP_SUFFIXES
PREFERENCES_STRIP_SUFFIXES = .in
endif

preferences_preprocess:
ifdef PREFERENCES
	@$(MKDIR) -p $(SONGBIRD_PREFERENCESDIR)
	for item in $(PREFERENCES); do \
	  target=$(SONGBIRD_PREFERENCESDIR)/`basename $$item $(PREFERENCES_STRIP_SUFFIXES)`; \
	  $(CYGWIN_WRAPPER) $(RM) -f $$target; \
    $(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl $(PREF_PPFLAGS) $(ACDEFINES) \
      $(PPDEFINES) -- $(srcdir)/$$item > $$target; \
  done
endif #PREFERENCES
.PHONY : preferences_preprocess

#-----------------------

# Preprocesses the $(APPINI) file and turns it into 'application.ini'.

appini_preprocess: $(if $(APPINI), $(SONGBIRD_DISTDIR)/application.ini)

appini_target = $(SONGBIRD_DISTDIR)/application.ini

$(SONGBIRD_DISTDIR)/application.ini: $(APPINI)
	$(CYGWIN_WRAPPER) $(RM) -f $(appini_target)
	$(CYGWIN_WRAPPER) $(MKDIR) -p $(SONGBIRD_DISTDIR)
	$(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl $(ACDEFINES) $(PPDEFINES) $(SB_BRANDING_DEFINES) -- \
    $(srcdir)/$(APPINI) > $(appini_target)

clean_appini:
	$(CYGWIN_WRAPPER) $(RM) -f $(appini_target)

.PHONY : appini_preprocess clean_appini

#-----------------------

ifdef DOXYGEN_PREPROCESS

# Preprocesses the $(DOXYGEN_PREPROCESS) so that the proper input and output directories
# can be set.

ifeq (windows,$(SB_PLATFORM))
DOXYGEN_PPFLAGS = --line-endings=crlf
endif

run_doxygen_preprocess:
	for file in $(DOXYGEN_PREPROCESS); do \
    source=$(SONGBIRD_DOCUMENTATIONDIR)/$$file.in; \
    target=$(SONGBIRD_DOCUMENTATIONDIR)/$$file; \
    $(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl $(DOXYGEN_PPFLAGS) \
      $(ACDEFINES) $(PPDEFINES) -DDOXYGEN_INPUTDIRS="$(DOXYGEN_INPUTDIRS)" \
      -DDOXYGEN_OUTPUTDIR="$(DOXYGEN_OUTPUTDIR)" \
      -DDOXYGEN_STRIP_TOPSRCDIR="$(DOXYGEN_STRIP_TOPSRCDIR)" -- \
      $$source > $$target; \
  done

clean_doxygen_preprocess:
	for file in $(DOXYGEN_PREPROCESS); do \
    $(CYGWIN_WRAPPER) $(RM) -f $(SONGBIRD_DOCUMENTATIONDIR)/$$file; \
  done

.PHONY : run_doxygen_preprocess clean_doxygen_preprocess

endif #DOXYGEN_PREPROCESS

#-----------------------


# Preprocesses the $(INSTALLER_PREPROCESS) files and turns them into plain .nsi files.

ifeq (windows,$(SB_PLATFORM))
INSTALLER_PPFLAGS = --line-endings=crlf
endif

run_installer_preprocess:
ifdef INSTALLER_PREPROCESS
	for file in $(INSTALLER_PREPROCESS); do \
    source=$(srcdir)/$$file.in; \
    target=$(SONGBIRD_INSTALLERDIR)/$$file; \
    $(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl $(INSTALLER_PPFLAGS) \
      $(ACDEFINES) $(PPDEFINES) $(SB_BRANDING_DEFINES) -- $$source > $$target; \
  done

.PHONY : run_installer_preprocess

endif #INSTALLER_PREPROCESS

#------------------------------------------------------------------------------
# Rules for packaging things nicely
#------------------------------------------------------------------------------

#ifdef JAR_MANIFEST

ifdef EXTENSION_STAGE_DIR
   JAR_IS_EXTENSION = 1
endif

# Extension jars need to go to the extensions subdirectory of the xulrunner
# folder. Otherwise everything goes into the chrome directory.

# Allow this to be overridden
ifdef JAR_TARGET_DIR
  TARGET_DIR = $(JAR_TARGET_DIR)
else
ifdef JAR_IS_EXTENSION
  TARGET_DIR = $(SONGBIRD_EXTENSIONSDIR)/$(EXTENSION_UUID)/chrome
else
  TARGET_DIR = $(SONGBIRD_CHROMEDIR)
endif
endif

TARGET_DIR := $(strip $(TARGET_DIR))

MAKE_JARS_FLAGS = -s $(srcdir) \
                  -t $(topsrcdir) \
                  -j $(TARGET_DIR) \
                  -z $(ZIP) \
                  -p $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl \
                  -v \
                  $(NULL)

# We use flat jars (i.e. plain directories) if we have DEBUG defined and
# FORCE_JARS is _not_defined. Also use flat jars if in a release build and
# PREVENT_JARS is defined.

ifdef DEBUG
   ifneq (1,$(FORCE_JARS))
      USING_FLAT_JARS=1
   endif
else # DEBUG
   ifeq (1,$(PREVENT_JARS))
      USING_FLAT_JARS=1
   endif
endif # DEBUG

ifdef USING_FLAT_JARS
   PPDEFINES += -DUSING_FLAT_JARS=$(USING_FLAT_JARS)
   MAKE_JARS_FLAGS += -f flat -d $(TARGET_DIR)
else
   MAKE_JARS_FLAGS += -d $(TARGET_DIR)/stage
endif

ifdef JAR_IS_EXTENSION
   MAKE_JARS_FLAGS += -e
endif

# Rather than assuming that all jar manifests are named 'jar.mn' we allow the
# filename to be passed in via the JAR_MANIFEST variable. If no preprocessing
# is needed then the file is passed to the make_jars.pl perl program. If
# preprocessing is desired then a file with the '.in' extension should exist in
# the source directory of the form '$(JAR_MANIFEST).in'.

# Examples:
#   JAR_MANIFEST = jar.mn
#
#   If 'jar.mn' exists in the source directory then no preprocessing will occur
#     and that file will be passed to make_jars.pl.
#
#   If 'jar.mn.in' exists and 'jar.mn' does _not_ exist then the preprocessor
#     will generate 'jar.mn' in the object directory before passing it to
#     make_jars.pl.
#
#   If both 'jar.mn' _and_ 'jar.mn.in' exist in the source directory then no
#     preprocessing will occur.

# Check to see if the manifest file exists in the source dir. If not then we're
# going to assume it needs to be generated through preprocessing. The
# postprocessed file will be generated in the object directory.

jar_mn_exists := $(shell if test -f $(srcdir)/$(JAR_MANIFEST); then \
                           echo 1; \
                         fi;)

ifneq (,$(jar_mn_exists))
jar_manifest_file = $(srcdir)/$(JAR_MANIFEST)
else
jar_manifest_file = ./$(JAR_MANIFEST)
endif

# Now check to see if a '.in' file exists.

jar_mn_in_exists := $(shell if test -f $(srcdir)/$(JAR_MANIFEST).in; then \
                              echo 1; \
                            fi;)

ifneq (,$(jar_mn_in_exists))
jar_manifest_in = $(JAR_MANIFEST).in
endif

$(JAR_MANIFEST):
ifneq (,$(jar_mn_in_exists))
	$(CYGWIN_WRAPPER) $(RM) -f $(JAR_MANIFEST)
	$(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl $(ACDEFINES) $(PPDEFINES) -- \
    $(srcdir)/$(jar_manifest_in) | \
    $(PERL) $(SCRIPTS_DIR)/expand-jar-mn.pl $(srcdir) > $(JAR_MANIFEST)
endif

make_jar: $(JAR_MANIFEST)
	$(CYGWIN_WRAPPER) $(MKDIR) -p $(TARGET_DIR)
	$(PERL) -I$(MOZSDK_SCRIPTS_DIR) $(MOZSDK_SCRIPTS_DIR)/make-jars.pl \
      $(MAKE_JARS_FLAGS) -- $(ACDEFINES) $(PPDEFINES) < $(jar_manifest_file)
	@$(CYGWIN_WRAPPER) $(RM) -rf $(TARGET_DIR)/stage


clean_jar_postprocess:
	$(CYGWIN_WRAPPER) $(RM) -f ./$(JAR_MANIFEST)

# We want the preprocessor to run every time regrdless of whether or not
# $(jar_manifest_in) has changed because defines may change as well.

.PHONY : make_jar clean_jar_postprocess $(JAR_MANIFEST)
#endif

#------------------------------------------------------------------------------
# Rules for creating XPIs
#------------------------------------------------------------------------------

# XPI_NAME - The base name (no extension) of the XPI to create. To do this you
#            must also set the following variables:
#
#              EXTENSION_STAGE_DIR - dir where the XPIs contents reside
#              EXTENSION_NAME - name of the extension (coolthing)
#
#            You must have 'install.rdf' in your src directory OR you can use
#            the preprocessor to create one. To do that either name your input
#            file 'install.rdf.in' or specify its name with the following:
#
#              INSTALL_RDF -  the name of the input file that will be passed to
#                             the preprocessor to create 'install.rdf'
#
#            If you use the preprocessor you may want to also set the
#            following variables:
#
#              EXTENSION_UUID    - uuid of the extension
#                                  (e.g. "coolthing@songbirdnest.com")
#              EXTENSION_ARCH    - arch string describing the build machine
#                                  (e.g. "WINNT_x86-msvc" or "Darwin_x86-gcc4")
#              EXTENSION_VER     - extension version
#                                  (e.g. "1.2.3")
#              EXTENSION_MIN_VER - minimum version of application needed for 
#                                  extension (e.g. "0.7pre")
#              EXTENSION_MAX_VER - maximum version of application needed for 
#                                  extension (e.g. "0.7.*")
#
#            If you want to also install the contents of the XPI to the
#            extensions directory then you may set the following variable:
#
#              INSTALL_EXTENSION - whether or not to install the XPI
#
#            Note that INSTALL_EXTENSION requires that EXTENSION_UUID be set
#
#            You may override this variable if you want the output of the
#            extension build process to output your xpi to a different location
#            than standard. Defaults to OBJDIR/xpi-stage/EXTENSION_NAME. You
#            wouldn't normally want to do this.
#
#              EXTENSION_DIR - dir where the final XPI should be moved
#

make_ext_stage:
ifdef EXTENSION_STAGE_DIR
ifneq (clean,$(MAKECMDGOALS))
	$(MKDIR) -p $(EXTENSION_STAGE_DIR)
endif
endif

ifdef XPI_NAME

# set a specific location for the output if it doesn't already exist
ifndef EXTENSION_DIR
EXTENSION_DIR  = $(SONGBIRD_OBJDIR)/xpi-stage/$(EXTENSION_NAME)
endif

ifdef EXTENSION_VER
ifeq ($(SONGBIRD_OFFICIAL)_$(SONGBIRD_NIGHTLY),_)
  EXTENSION_VER := $(EXTENSION_VER)+dev
else
  EXTENSION_VER := $(EXTENSION_VER).$(SB_BUILD_NUMBER)
endif
endif # EXTENSION_VER

# Create install.rdf if it doesn't exist

install_rdf_file = $(srcdir)/install.rdf

ifeq (,$(wildcard $(install_rdf_file)))

# Try this simple substitution. Saves one line in Makefiles...
ifneq (,$(wildcard $(install_rdf_file).in))
  INSTALL_RDF = $(install_rdf_file).in
endif

# If neither install.rdf nor install.rdf.in exist then the user needs to tell
# us which file to use. Bail.
ifndef INSTALL_RDF
  current_dir = $(shell pwd)
  $(error $(current_dir)/Makefile: install.rdf not found, use INSTALL_RDF)
endif

# install.rdf doesn't exist, so generate it from the given file
install_rdf_file = ./install.rdf

$(install_rdf_file): $(srcdir)/$(INSTALL_RDF)
	$(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl            \
          $(ACDEFINES) $(PPDEFINES)                        \
          -DEXTENSION_ARCH=$(EXTENSION_ARCH)               \
          -DEXTENSION_UUID=$(EXTENSION_UUID)               \
          -DEXTENSION_VER=$(EXTENSION_VER)                 \
          -DEXTENSION_MIN_VER=$(EXTENSION_MIN_VER)         \
          -DEXTENSION_MAX_VER=$(EXTENSION_MAX_VER)         \
          -DEXTENSION_NAME=$(EXTENSION_NAME) --            \
          $(srcdir)/$(INSTALL_RDF) > $(install_rdf_file)

endif

# Check for an extension license; default file name is "LICENSE" in the root
# directory of the extension. You can override this by setting EXTENSION_LICENSE
# in the extension's Makefile
EXTENSION_LICENSE ?= $(wildcard $(srcdir)/LICENSE)

.PHONY: make_xpi
make_xpi: make_ext_stage $(install_rdf_file) $(SUBDIRS) $(JAR_MANIFEST)
	@echo packaging $(EXTENSION_DIR)/$(XPI_NAME).xpi
	$(RM) -f $(EXTENSION_DIR)/$(XPI_NAME).xpi
	$(CP) -f $(install_rdf_file) $(EXTENSION_STAGE_DIR)/install.rdf
 ifneq (,$(EXTENSION_LICENSE))
	$(CP) -f $(EXTENSION_LICENSE) $(EXTENSION_STAGE_DIR)
 endif
	cd $(EXTENSION_STAGE_DIR) && $(ZIP) -qr ../$(XPI_NAME).xpi.tmp *
	$(MKDIR) -p $(EXTENSION_DIR)
	$(MV) -f $(EXTENSION_STAGE_DIR)/../$(XPI_NAME).xpi.tmp \
        $(EXTENSION_DIR)/$(XPI_NAME).xpi
 ifdef INSTALL_EXTENSION
	$(MKDIR) -p $(SONGBIRD_EXTENSIONSDIR)
	$(RM) -rf $(SONGBIRD_EXTENSIONSDIR)/$(EXTENSION_UUID)
	$(CP) -rf $(EXTENSION_STAGE_DIR) $(SONGBIRD_EXTENSIONSDIR)/$(EXTENSION_UUID)
 endif

.PHONY: clean_xpi
clean_xpi:
	$(RM) -f $(EXTENSION_DIR)/$(XPI_NAME).xpi
	$(RM) -f ./install.rdf
	$(RM) -rf $(EXTENSION_STAGE_DIR)

else
make_xpi:
endif # XPI_NAME

#------------------------------------------------------------------------------
# Rules for changing permissions
#------------------------------------------------------------------------------

ifdef EXECUTABLE
chmod_add_executable:
	$(warning WARNING: $@ is DEPRECATED and slated for REMOVAL; you have been warned...)
	$(CYGWIN_WRAPPER) $(CHMOD) +x $(EXECUTABLE)
.PHONY : chmod_add_executable
endif

#------------------------------------------------------------------------------
# Rules for manipulating ZIP archives
#------------------------------------------------------------------------------

# UNZIP_SRC - The name of a zip file to decompress in its entirety
# UNZIP_DEST - The name of a directory to hold the archive's contents
# UNZIP_FLAGS - an override of UNZIPFLAGS to pass to the program
# UNZIP_EXTRA_FLAGS - an additional list of flags to pass to the program

ifdef UNZIP_FLAGS
unzip_flags = $(UNZIP_FLAGS)
else
unzip_flags = $(UNZIPFLAGS)
ifdef UNZIP_EXTRA_FLAGS
unzip_flags += $(UNZIP_EXTRA_FLAGS)
endif
endif

unzip_file:
ifdef UNZIP_SRC
   ifneq (1,$(words $(UNZIP_SRC)))
      $(error You can only have one file specified by UNZIP_SRC)
   endif
   ifndef UNZIP_DEST_DIR
      $(error UNZIP_DEST_DIR *must* be set when extracting from UNZIP_SRC = $(UNZIP_SRC))
   endif

	$(warning WARNING: $@ is DEPRECATED and slated for REMOVAL; you have been warned...)
	$(CYGWIN_WRAPPER) $(UNZIP) $(unzip_flags) $(UNZIP_SRC) $(UNZIPFLAGS_EXTRACT) $(UNZIP_DEST_DIR)
endif # UNZIP_SRC

.PHONY : unzip_file


#------------------------------------------------------------------------------
# Rules for manipulating GZ archives
#------------------------------------------------------------------------------

gunzip_file:
ifdef GUNZIP_SRC
   ifndef GUNZIP_DEST_DIR
      $(error GUNZIP_DEST_DIR *must* be set when extracting from GUNZIP_SRC = $(GUNZIP_SRC))
   endif
   ifneq (1,$(words $(GUNZIP_SRC)))
      $(error You can only have one file specified by GUNZIP_SRC)
   endif

	$(warning WARNING: $@ is DEPRECATED and slated for REMOVAL; you have been warned...)
	$(TAR) -z -x -f $(GUNZIP_SRC) -C $(GUNZIP_DEST_DIR)

.PHONY : gunzip_file

endif # GUNZIP_SRC

#------------------------------------------------------------------------------
# Rules for executing something in a shell (sh)
#------------------------------------------------------------------------------


shell_execute:
ifdef SHELL_EXECUTE
	$(error $@ is hard DEPRECATED; stop using it...)
	sh $(SHELL_EXECUTE)
endif #SHELL_EXECUTE

.PHONY : shell_execute


#------------------------------------------------------------------------------
# Rules for making directories
#------------------------------------------------------------------------------

# CREATEDIRS - set to a list of directories to create

create_dirs:
	$(warning WARNING: $@ is DEPRECATED and slated for REMOVAL; you have been warned...)
	$(foreach dir,$(CREATEDIRS),$(CYGWIN_WRAPPER) $(MKDIR) $(dir);)

.PHONY : create_dirs

create_dirs_clean:
	$(CYGWIN_WRAPPER) $(RM) -rf $(CREATEDIRS)

.PHONY : create_dirs_clean
