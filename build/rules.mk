#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright 2006 POTI, Inc.
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

#
# Collect a list of rules to run. We use two variables so that 'make clean'
# does what you'd expect.
#

targets=
clean_targets=

ifdef CREATEDIRS
targets += create_dirs
endif

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

ifdef CPP_SRCS
targets += cpp_compile
clean_targets += cpp_clean
endif

ifdef DYNAMIC_LIB
targets += dll_link
clean_targets += dll_clean
endif

ifdef STATIC_LIB
targets += lib_link
clean_targets += lib_clean
endif

ifdef UNZIP_SRC
targets += unzip_file
endif

ifdef GUNZIP_SRC
targets += gunzip_file
endif

ifdef SONGBIRD_MAIN_APP
targets += move_sb_stub_executable
endif

ifdef EXECUTABLE
targets += chmod_add_executable
endif

ifdef CLONEDIR
targets += clone_dir
endif

ifdef SONGBIRD_DIST
targets += copy_sb_dist
endif

ifdef SONGBIRD_COMPONENTS
targets += copy_sb_components
endif

ifdef SONGBIRD_CHROME
targets += copy_sb_chrome
endif

ifdef SONGBIRD_DEFAULTS
targets += copy_sb_defaults
endif

ifdef SONGBIRD_PREFS
targets += copy_sb_prefs
endif

ifdef SONGBIRD_PLUGINS
targets += copy_sb_plugins
endif

ifdef SONGBIRD_SCRIPTS
targets += copy_sb_scripts
endif

ifdef SONGBIRD_VLCPLUGINS
targets += copy_sb_vlcplugins
endif

ifdef SONGBIRD_XULRUNNER
targets += copy_sb_xulrunner
endif

ifdef SONGBIRD_CONTENTS
targets += copy_sb_macoscontents
endif

ifdef JAR_MANIFEST
targets += make_jar
clean_targets += clean_jar_postprocess
endif

ifdef PREFERENCES
targets += preferences_preprocess
endif

ifdef APPINI
targets += appini_preprocess
clean_targets += clean_appini
endif

ifdef SHELL_EXECUTE
targets += shell_execute
endif

# Right now this system is not compatible with parallel make.
.NOTPARALLEL : all clean

all:   $(targets) \
       garbage \
       $(NULL)

clean: $(clean_targets) \
       create_dirs_clean \
       $(NULL)

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
ifneq ($(SUBDIRS),)
SUBMAKEFILES            := $(addsuffix /Makefile, $(SUBDIRS))
endif

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
# Rules for C++ compilation
#------------------------------------------------------------------------------

# CPP_SRCS - a list of .cpp files to be compiled
# CPP_INCLUDES - a list of include dirs
# CPP_FLAGS - an override of the default flags to pass to the compiler
# CPP_EXTRA_FLAGS - a list of additional flags to pass to the compiler
# CPP_DEFS - a override of the default defines to pass to the compiler with -D added
# CPP_EXTRA_DEFS - a list of additional defines with -D to pass to the compiler

ifdef CPP_SRCS

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
compile_defs = $(ACDEFINES)
ifdef CPP_EXTRA_DEFS
compile_defs += $(CPP_EXTRA_DEFS)
endif
endif

ifdef CPP_INCLUDES
compile_includes_temp = $(addprefix $(CFLAGS_INCLUDE_PREFIX), $(CPP_INCLUDES))
compile_includes = $(addsuffix $(CFLAGS_INCLUDE_SUFFIX), $(compile_includes_temp))
endif

compiler_objects = $(CPP_SRCS:.cpp=$(OBJ_SUFFIX))

$(compiler_objects) :%$(OBJ_SUFFIX): %.cpp
	$(CYGWIN_WRAPPER) $(CXX) $(compile_flags) $(compile_defs) $(compile_includes) $<

cpp_compile: $(compiler_objects)

cpp_clean:
	$(CYGWIN_WRAPPER) $(RM) -f $(compiler_objects) vc70.pdb vc71.pdb

.PHONY : cpp_compile cpp_clean

endif #CPP_SRCS

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

linker_objs = $(DYNAMIC_LIB_OBJS)

ifeq (win32,$(SB_PLATFORM))
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
linker_paths_temp = $(addprefix $(LDFLAGS_PATH_PREFIX), $(DYNAMIC_LIB_IMPORT_PATHS))
linker_paths = $(addsuffix $(LDFLAGS_PATH_SUFFIX), $(linker_paths_temp))
endif

linker_out = $(LDFLAGS_OUT_PREFIX)$(DYNAMIC_LIB)$(LDFLAGS_OUT_SUFFIX)

makelink_cmd = $(CYGWIN_WRAPPER) $(LN) $(LNFLAGS) $(DYNAMIC_LIB) $(addprefix lib,$(DYNAMIC_LIB))

dll_link: $(DYNAMIC_LIB_OBJS)
	$(CYGWIN_WRAPPER) $(LD) $(linker_out) $(linker_flags) $(linker_paths) $(linker_imports) $(linker_objs)
	$(CYGWIN_WRAPPER) $(CHMOD) +x $(DYNAMIC_LIB)
	$(makelink_cmd)

dll_clean:
	$(CYGWIN_WRAPPER) $(RM) -f $(DYNAMIC_LIB) \
	      $(DYNAMIC_LIB:$(DLL_SUFFIX)=.pdb) \
	      $(DYNAMIC_LIB:$(DLL_SUFFIX)=.lib) \
	      $(DYNAMIC_LIB:$(DLL_SUFFIX)=.exp) \
	      $(NULL)

.PHONY : dll_clean

endif #DYNAMIC_LIB

#-----------------------

# STATIC_LIB - the name of a lib to link
# STATIC_LIB_OBJS - the object files to link into the lib
# STATIC_LIB_FLAGS - an override to the default linker flags
# STATIC_LIB_EXTRA_FLAGS - a list of additional flags to pass to the linker

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

makelink_cmd = $(CYGWIN_WRAPPER) $(LN) $(LNFLAGS) $(STATIC_LIB) $(addprefix lib,$(STATIC_LIB))

lib_link: $(static_lib_deps)
	$(CYGWIN_WRAPPER) $(AR) $(linker_flags) $(linker_out) $(STATIC_LIB_OBJS)
	$(ranlib_cmd)
	$(makelink_cmd)

lib_clean:
	$(CYGWIN_WRAPPER) $(RM) -f $(STATIC_LIB)

.PHONY : lib_clean

endif #STATIC_LIB

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

ifdef XPIDL_HEADER_SRCS

xpidl_headers  = $(XPIDL_HEADER_SRCS:.idl=.h)

xpidl_includes_temp = $(MOZSDK_IDL_DIR) \
                      $(srcdir) \
                      $(XPIDL_INCLUDES) \
                      $(NULL)

xpidl_includes = $(addprefix $(XPIDLFLAGS_INCLUDE), $(xpidl_includes_temp))

xpidl_compile_headers: $(XPIDL_HEADER_SRCS) $(xpidl_headers)

$(xpidl_headers): %.h: %.idl
	$(CYGWIN_WRAPPER) $(XPIDL) -m header $(xpidl_includes) $(XPIDL_EXTRA_FLAGS) $<

xpidl_clean_headers:
	$(CYGWIN_WRAPPER) $(RM) -f $(xpidl_headers)

.PHONY : xpidl_compile_headers xpidl_clean_headers

endif #XPIDL_HEADER_SRCS

#-----------------------

ifdef XPIDL_TYPELIB_SRCS

xpidl_typelibs = $(XPIDL_TYPELIB_SRCS:.idl=.xpt)

xpidl_includes_temp = $(MOZSDK_IDL_DIR) \
                      $(srcdir) \
                      $(XPIDL_INCLUDES) \
                      $(NULL)

xpidl_includes = $(addprefix $(XPIDLFLAGS_INCLUDE), $(xpidl_includes_temp))

xpidl_compile_typelibs: $(XPIDL_TYPELIB_SRCS) $(xpidl_typelibs)

$(xpidl_typelibs): %.xpt: %.idl
	$(CYGWIN_WRAPPER) $(XPIDL) -m typelib $(xpidl_includes) $(XPIDL_EXTRA_FLAGS) $<

xpidl_clean_typelibs:
	$(CYGWIN_WRAPPER) $(RM) -f $(xpidl_typelibs)

.PHONY : xpidl_compile_typelibs xpidl_clean_typelibs

endif #XPIDL_TYPELIB_SRCS

#-----------------------

ifdef XPIDL_MODULE

xpidl_module_typelibs = $(XPIDL_MODULE_TYPELIBS)

xpidl_link: $(xpidl_module_typelibs)
	$(CYGWIN_WRAPPER) $(XPTLINK) $(XPIDL_MODULE) $(xpidl_module_typelibs)

xpidl_clean_link:
	$(CYGWIN_WRAPPER) $(RM) -f $(XPIDL_MODULE)

.PHONY : xpidl_link xpidl_clean_link

endif #XPIDL_MODULE

#------------------------------------------------------------------------------
# Rules for running make recursively
#------------------------------------------------------------------------------

# SUBDIRS - set to a list of subdirectories that contain makefiles
# SUBDIRDEPS - a list of dependencies of the form dir1:dir2 (dir1 depends on
#              dir2 having already been processed)
#

ifdef SUBDIRS

make_subdirs: $(SUBDIRS)

ifdef SUBDIRDEPS
$(SUBDIRDEPS)
$(SUBDIRDEPS2)
$(SUBDIRDEPS3)
$(SUBDIRDEPS4)
$(SUBDIRDEPS5)
$(SUBDIRDEPS6)
endif

$(SUBDIRS):
	$(CYGWIN_WRAPPER) $(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY : make_subdirs $(SUBDIRS)

endif #SUBDIRS

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

clone_dir :
	$(clone_dir_cmd)

.PHONY : clone_dir

endif #CLONEDIR

#-----------------------

ifdef SONGBIRD_DIST
copy_sb_dist:
	$(CYGWIN_WRAPPER) $(CP) -dfp $(SONGBIRD_DIST) $(SONGBIRD_DISTDIR)
.PHONY : copy_sb_dist
endif #SONGBIRD_DIST

#-----------------------

ifdef SONGBIRD_CHROME
copy_sb_chrome:
	$(CYGWIN_WRAPPER) $(CP) -dfp $(SONGBIRD_CHROME) $(SONGBIRD_CHROMEDIR)
.PHONY : copy_sb_chrome
endif #SONGBIRD_CHROME

#-----------------------

ifdef SONGBIRD_COMPONENTS
copy_sb_components:
	$(CYGWIN_WRAPPER) $(CP) -dfp $(SONGBIRD_COMPONENTS) $(SONGBIRD_COMPONENTSDIR)
.PHONY : copy_sb_components
endif #SONGBIRD_COMPONENTS

#-----------------------

ifdef SONGBIRD_DEFAULTS
copy_sb_defaults:
	$(CYGWIN_WRAPPER) $(CP) -dfp $(SONGBIRD_DEFAULTS) $(SONGBIRD_DEFAULTSDIR)
.PHONY : copy_sb_defaults
endif #SONGBIRD_DEFAULTS  

#-----------------------

ifdef SONGBIRD_PREFS
songbird_pref_files := $(addprefix $(srcdir)/,$(SONGBIRD_PREFS))
copy_sb_prefs:
	$(CYGWIN_WRAPPER) $(CP) -dfp $(songbird_pref_files) $(SONGBIRD_PREFERENCESDIR)
.PHONY : copy_sb_prefs
endif #SONGBIRD_PREFS

#-----------------------

ifdef SONGBIRD_PLUGINS
copy_sb_plugins:
	$(CYGWIN_WRAPPER) $(CP) -dfp $(SONGBIRD_PLUGINS) $(SONGBIRD_PLUGINSDIR)
.PHONY : copy_sb_plugins
endif #SONGBIRD_PLUGINS

#-----------------------

ifdef SONGBIRD_SCRIPTS
copy_sb_scripts:
	$(CYGWIN_WRAPPER) $(CP) -dfp $(SONGBIRD_SCRIPTS) $(SONGBIRD_SCRIPTSDIR)
.PHONY : copy_sb_scripts
endif #SONGBIRD_SCRIPTS

#-----------------------

ifdef SONGBIRD_VLCPLUGINS
copy_sb_vlcplugins:
	$(CYGWIN_WRAPPER) $(CP) -dfp $(SONGBIRD_VLCPLUGINS) $(SONGBIRD_VLCPLUGINSDIR)
.PHONY : copy_sb_vlcplugins
endif #SONGBIRD_VLCPLUGINS

#-----------------------

ifdef SONGBIRD_XULRUNNER
copy_sb_xulrunner:
	$(CYGWIN_WRAPPER) $(CP) -dfp $(SONGBIRD_XULRUNNER) $(SONGBIRD_XULRUNNERDIR)
.PHONY : copy_sb_xulrunner
endif #SONGBIRD_XULRUNNER

#-----------------------

ifdef SONGBIRD_CONTENTS
copy_sb_macoscontents:
	$(CYGWIN_WRAPPER) $(CP) -dfp $(SONGBIRD_CONTENTS) $(SONGBIRD_CONTENTSDIR)
.PHONY : copy_sb_macoscontents
endif #SONGBIRD_CONTENTS

#------------------------------------------------------------------------------
# Rules for preprocessing
#------------------------------------------------------------------------------

ifdef PREFERENCES

# on win32, pref files need CRLF line endings
ifeq (win32,$(SB_PLATFORM))
PREF_PPFLAGS = --line-endings=crlf
endif

ifndef PREFERENCES_STRIP_SUFFIXES
PREFERENCES_STRIP_SUFFIXES = .in
endif
                             
preferences_preprocess:
	@$(MKDIR) -p $(SONGBIRD_PREFERENCESDIR)
	for item in $(PREFERENCES); \
    do $(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl \
      $(PREF_PPFLAGS) $(ACDEFINES) $(PPDEFINES) -- $(srcdir)/$$item > \
      $(SONGBIRD_PREFERENCESDIR)/`basename $$item $(PREFERENCES_STRIP_SUFFIXES)`; \
    done
.PHONY : preferences_preprocess
endif #PREFERENCES

#-----------------------

ifdef APPINI

# Preprocesses the $(APPINI) file and turns it into 'application.ini'.

appini_preprocess: $(SONGBIRD_DISTDIR)/application.ini

$(SONGBIRD_DISTDIR)/application.ini: $(APPINI)
	@$(MKDIR) -p $(SONGBIRD_DISTDIR)
	$(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl \
    $(ACDEFINES) $(PPDEFINES) -- $(srcdir)/$(APPINI) > \
    $(SONGBIRD_DISTDIR)/application.ini; \
    $(NULL)

clean_appini:
	$(CYGWIN_WRAPPER) $(RM) -f $(SONGBIRD_DISTDIR)/application.ini

.PHONY : appini_preprocess clean_appini

endif #APPINI

#------------------------------------------------------------------------------
# Rules for packaging things nicely
#------------------------------------------------------------------------------

ifdef JAR_MANIFEST

# Extension jars need to go to the extensions subdirectory of the xulrunner
# folder. Otherwise everything goes into the chrome directory.

ifdef JAR_IS_EXTENSION
# Hack to make this work with directories with funny characters (e.g. '@')
TARGET_DIR = $(SONGBIRD_EXTENSIONSDIR)/.package
MANIFEST_MOVE_CMD = $(CYGWIN_WRAPPER) $(MV) -f \
                      $(SONGBIRD_EXTENSIONSDIR)/chrome.manifest \
                      $(SONGBIRD_EXTENSIONSDIR)/.package/ \
                      $(NULL)
EXTENSION_PACKAGING_CMD = $(CYGWIN_WRAPPER) $(RM) -rf \
                            $(SONGBIRD_EXTENSIONSDIR)/$(EXTENSION_UUID) && \
                            $(CYGWIN_WRAPPER) $(MV) -f \
                            $(SONGBIRD_EXTENSIONSDIR)/.package \
                            $(SONGBIRD_EXTENSIONSDIR)/$(EXTENSION_UUID) \
                            $(NULL)
else
TARGET_DIR = $(SONGBIRD_CHROMEDIR)
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
ifndef FORCE_JARS
USING_FLAT_JARS=1
endif
else # DEBUG
ifdef PREVENT_JARS
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

$(JAR_MANIFEST): $(jar_manifest_in)
	$(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl \
    $(ACDEFINES) $(PPDEFINES) -- $(srcdir)/$(jar_manifest_in) > \
    ./$(JAR_MANIFEST) \
    $(NULL)

make_jar: $(JAR_MANIFEST)
	@$(CYGWIN_WRAPPER) $(MKDIR) -p $(TARGET_DIR)
	@$(PERL) -I$(MOZSDK_SCRIPTS_DIR) \
           $(MOZSDK_SCRIPTS_DIR)/make-jars.pl \
           $(MAKE_JARS_FLAGS) -- $(ACDEFINES) $(PPDEFINES)\
           < $(jar_manifest_file) \
           $(NULL)
	@$(CYGWIN_WRAPPER) $(RM) -rf $(TARGET_DIR)/stage
	@$(MANIFEST_MOVE_CMD)
	@$(EXTENSION_PACKAGING_CMD)

clean_jar_postprocess:
	$(CYGWIN_WRAPPER) $(RM) -f ./$(JAR_MANIFEST)

# We want the preprocessor to run every time regrdless of whether or not
# $(jar_manifest_in) as changed because defines may change as well.

.PHONY : make_jar clean_jar_postprocess $(JAR_MANIFEST)
endif

#-----------------------

ifdef SONGBIRD_MAIN_APP

ifeq (macosx,$(SB_PLATFORM))

ifneq (,$(findstring *xulrunner*,$(SONGBIRD_MAIN_APP)))
$(error You must specify the xulrunner-stub file here)
else
sb_executable = $(SONGBIRD_MACOS)/$(SB_APPNAME)$(BIN_SUFFIX)
endif

else

ifneq (,$(findstring *xulrunner-stub*,$(SONGBIRD_MAIN_APP)))
$(error You must specify the xulrunner-stub file here)
else
sb_executable = $(SONGBIRD_DISTDIR)/$(SB_APPNAME)$(BIN_SUFFIX)
endif
endif

move_sb_stub_executable: $(SONGBIRD_MAIN_APP)
	$(CYGWIN_WRAPPER) $(MV) -f $(SONGBIRD_MAIN_APP) $(sb_executable)
	$(CYGWIN_WRAPPER) $(CHMOD) +x $(sb_executable)

#.PHONY : move_sb_stub_executable

endif

#------------------------------------------------------------------------------
# Rules for changing permissions
#------------------------------------------------------------------------------

ifdef EXECUTABLE
chmod_add_executable:
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

ifdef UNZIP_SRC

ifndef UNZIP_DEST_DIR
$(error UNZIP_DEST_DIR *must* be set when extracting from UNZIP_SRC = $(UNZIP_SRC))
endif

ifneq (1,$(words $(UNZIP_SRC)))
$(error You can only have one file specified by UNZIP_SRC)
endif

ifdef UNZIP_FLAGS
unzip_flags = $(UNZIP_FLAGS)
else
unzip_flags = $(UNZIPFLAGS)
ifdef UNZIP_EXTRA_FLAGS
unzip_flags += $(UNZIP_EXTRA_FLAGS)
endif
endif

unzip_file:
	$(CYGWIN_WRAPPER) $(UNZIP) $(unzip_flags) $(UNZIP_SRC) $(UNZIPFLAGS_EXTRACT) $(UNZIP_DEST_DIR)

.PHONY : unzip_file

endif # UNZIP_SRC

#------------------------------------------------------------------------------
# Rules for manipulating GZ archives
#------------------------------------------------------------------------------

ifdef GUNZIP_SRC

ifndef GUNZIP_DEST_DIR
$(error GUNZIP_DEST_DIR *must* be set when extracting from GUNZIP_SRC = $(GUNZIP_SRC))
endif

ifneq (1,$(words $(GUNZIP_SRC)))
$(error You can only have one file specified by GUNZIP_SRC)
endif

gunzip_file:
	$(CYGWIN_WRAPPER) $(TAR) -z -x -f $(GUNZIP_SRC) -C $(GUNZIP_DEST_DIR)

.PHONY : gunzip_file

endif # GUNZIP_SRC

#------------------------------------------------------------------------------
# Rules for executing something in a shell (sh)
#------------------------------------------------------------------------------

ifdef SHELL_EXECUTE

shell_execute:
	$(SHELL_EXECUTE)

.PHONY : shell_execute

endif #SHELL_EXECUTE

#------------------------------------------------------------------------------
# Rules for making directories
#------------------------------------------------------------------------------

# CREATEDIRS - set to a list of directories to create

ifdef CREATEDIRS

create_dirs:
	$(CYGWIN_WRAPPER) $(MKDIR) -p $(CREATEDIRS)

.PHONY : create_dirs

endif #CREATEDIRS

create_dirs_clean:
	$(CYGWIN_WRAPPER) $(RM) -rf $(CREATEDIRS)

.PHONY : create_dirs_clean

#------------------------------------------------------------------------------
# Rules for cleaning up
#------------------------------------------------------------------------------

# GARBAGE - a list of files to delete upon completion

ifdef GARBAGE

remove_cmd = $(CYGWIN_WRAPPER) $(RM) -f $(GARBAGE)

out:
	$(warning garbage string: $(GARBAGE))
.PHONY : out

endif #GARBAGE

garbage: 
	$(remove_cmd)

.PHONY : garbage

#------------------------------------------------------------------------------
endif #RULES_MK_INCLUDED
#------------------------------------------------------------------------------
