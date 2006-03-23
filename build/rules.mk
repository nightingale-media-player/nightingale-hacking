#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright© 2006 Pioneers of the Inevitable LLC
# http://www.songbirdnest.com
# 
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the “GPL”).
# 
# Software distributed under the License is distributed 
# on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

targets=
clean_targets=

ifdef SUBDIRS
targets += make_subdirs
clean_targets += make_subdirs
endif

ifdef XPIDL_SRCS
ifndef XPIDL_HEADER_SRCS
XPIDL_HEADER_SRCS = $(XPIDL_SRCS)
endif
ifndef XPIDL_TYPELIB_SRCS
XPIDL_TYPELIB_SRCS = $(XPIDL_SRCS)
endif
endif

ifdef XPIDL_HEADER_SRCS
targets += xpidl_compile_headers
clean_targets += xpidl_clean_headers
endif

ifdef XPIDL_TYPELIB_SRCS
targets += xpidl_compile_typelibs
clean_targets += xpidl_clean_typelibs
endif

ifdef XPIDL_MODULE
targets += xpidl_link
clean_targets += xpidl_clean_link
endif

ifdef CPP_SOURCES
targets += cpp_compile
clean_targets += cpp_clean
endif

ifdef DYNAMIC_LIB
targets += dll_link
clean_targets += dll_clean
endif

all:   $(targets) \
       garbage \
       $(NULL)

clean: $(clean_targets) \
       $(NULL)

#------------------------------------------------------------------------------
# Rules for C++ compilation
#------------------------------------------------------------------------------

# CPP_SOURCES - a list of .cpp files to be compiled
# CPP_INCLUDES - a list of include dirs
# CPP_FLAGS - an override of the default flags to pass to the compiler
# CPP_EXTRA_FLAGS - a list of additional flags to pass to the compiler
# CPP_DEFS - a override of the default defines to pass to the compiler with -D added
# CPP_EXTRA_DEFS - a list of additional defines with -D to pass to the compiler

ifdef CPP_SOURCES

ifdef CPP_FLAGS
compile_flags = $(CPP_FLAGS)
else
compile_flags = $(CXXFLAGS)
ifdef CPP_EXTRA_FLAGS
compile_flags += $(CPP_EXTRA_FLAGS)
endif
endif

ifdef CPP_DEFS
compile_defs = $(CPP_DEFS)
else
compile_defs = $(DEFS)
ifdef CPP_EXTRA_DEFS
compile_defs += $(CPP_EXTRA_DEFS)
endif
endif

ifdef CPP_INCLUDES
compile_includes_temp = $(addprefix $(CFLAGS_INCLUDE_PREFIX), $(CPP_INCLUDES))
compile_includes = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX), $(compile_includes_temp))
endif

compiler_objects = $(CPP_SOURCES:.cpp=$(OBJ_SUFFIX))

$(compiler_objects) :%$(OBJ_SUFFIX): %.cpp
	$(CXX) $(compile_flags) $(compile_defs) $(compile_includes) $<

cpp_compile: $(compiler_objects)

cpp_clean:
	@rm -f $(compiler_objects) vc70.pdb

.PHONY : cpp_compile cpp_clean

endif #CPP_SOURCES

#-----------------------

# DYNAMIC_LIB - the name of a dll to link
# DYNAMIC_LIB_OBJS - the object files to link into the dll
# DYNAMIC_LIB_IMPORT_PATHS - a list of paths to search for libs
# DYNAMIC_LIB_IMPORTS - an override to the default list of libs to link
# DYNAMIC_LIB_EXTRA_IMPORTS - an additional list of libs to link
# DYNAMIC_LIB_FLAGS - an override to the default linker flags
# DYNAMIC_LIB_EXTRA_FLAGS - a list of additional flags to pass to the linker

ifdef DYNAMIC_LIB

ifdef DYNAMIC_LIB_FLAGS
linker_flags = $(DYNAMIC_LIB_FLAGS)
else
linker_flags = $(LDFLAGS) $(LDFLAGS_DLL)
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
linker_imports_temp2 = $(addprefix $(LDFLAGS_IMPORT_PREFIX), $(linker_imports_temp1))
linker_imports = $(addsuffix $(LDFLAGS_IMPORT_SUFFIX), $(linker_imports_temp2))

ifdef DYNAMIC_LIB_IMPORT_DIRS
linker_paths_temp = $(addprefix $(LDFLAGS_PATH_PREFIX), $(DYNAMIC_LIB_IMPORT_DIRS))
linker_paths = $(addsuffix $(LDFLAGS_PATH_SUFFIX), $(linker_paths_temp))
endif

linker_out = $(LDFLAGS_OUT_PREFIX)$(DYNAMIC_LIB)$(LDFLAGS_OUT_SUFFIX)

dll_link: $(DYNAMIC_LIB_OBJS)
	$(LD) $(linker_out) $(linker_flags) $(linker_paths) $(linker_imports) $(DYNAMIC_LIB_OBJS)

dll_clean:
	@rm -rf $(DYNAMIC_LIB) \
	        $(DYNAMIC_LIB:$(DLL_SUFFIX)=.pdb) \
	        $(DYNAMIC_LIB:$(DLL_SUFFIX)=.lib) \
	        $(DYNAMIC_LIB:$(DLL_SUFFIX)=.exp) \
	        $(NULL)

.PHONY : dll_link dll_clean

endif #DYNAMIC_LIB

#------------------------------------------------------------------------------
# Rules for XPIDL compilation
#------------------------------------------------------------------------------

# XPIDL_SRCS - a list of idl files to turn into header and typelib files
# XPIDL_HEADER_SRCS - a list of idl files to turn into C++ header files
# XPIDL_TYPELIB_SRCS - a list of idl files to turn into xpt typelib files
# XPIDL_MODULE - the name of an xpt file that will created from linking several
#                other xpt typelibs
# XPIDL_MODULE_TYPELIBS - a list of xpt files to link into the module

ifdef XPIDL_HEADER_SRCS

xpidl_headers  = $(XPIDL_HEADER_SRCS:.idl=.h)

xpidl_compile_headers: $(XPIDL_HEADER_SRCS) $(xpidl_headers)
$(xpidl_headers): %.h: %.idl
	$(XPIDL) -m header -I $(MOZSDK_IDL_DIR) -I $(srcdir) $<

xpidl_clean_headers:
	@rm -f $(xpidl_headers)

.PHONY : xpidl_compile_headers xpidl_clean_headers

endif #XPIDL_HEADER_SRCS

#-----------------------

ifdef XPIDL_TYPELIB_SRCS

xpidl_typelibs = $(XPIDL_TYPELIB_SRCS:.idl=.xpt)

xpidl_compile_typelibs: $(XPIDL_TYPELIB_SRCS) $(xpidl_typelibs)
$(xpidl_typelibs): %.xpt: %.idl
	$(XPIDL) -m typelib -I $(MOZSDK_IDL_DIR) -I $(srcdir) $<

xpidl_clean_typelibs:
	@rm -f $(xpidl_typelibs)

.PHONY : xpidl_compile_typelibs xpidl_clean_typelibs

endif #XPIDL_TYPELIB_SRCS

#-----------------------

ifdef XPIDL_MODULE

xpidl_module_typelibs = $(XPIDL_MODULE_TYPELIBS)

xpidl_link: $(xpidl_module_typelibs)
	$(XPTLINK) $(XPIDL_MODULE) $(xpidl_module_typelibs)

xpidl_clean_link:
	@rm -f $(XPIDL_MODULE)

.PHONY : xpidl_link xpidl_clean_link
endif #XPIDL_MODULE

#------------------------------------------------------------------------------
# Rules for running make recursively
#------------------------------------------------------------------------------

# SUBDIRS - set to a list of subdirectories that contain makefiles
# SUBDIRDEPS - a list of dependencies of the form dir1:dir2 (dir1 depends on
#              dir2 having already been processed)
#
# rules: subdirs - runs make in the list of subdirectories

ifdef SUBDIRS

make_subdirs: $(SUBDIRS)

ifdef SUBDIRDEPS
$(SUBDIRDEPS)
endif

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY : make_subdirs $(SUBDIRS)

endif #SUBDIRS

#------------------------------------------------------------------------------
# Rules for cleaning up
#------------------------------------------------------------------------------

# GARBAGE - a list of files to delete upon completion

ifdef GARBAGE
remove_cmd = rm -f $(GARBAGE)
out:
	$(warning garbage string: $(GARBAGE))
.PHONY : out
endif #GARBAGE

garbage:
	@$(remove_cmd)

.PHONY : garbage

#------------------------------------------------------------------------------
endif #RULES_MK_INCLUDED
#------------------------------------------------------------------------------
