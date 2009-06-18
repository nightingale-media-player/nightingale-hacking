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
   # We use 'pwd -W' to get DOS form of the path.  However, since the given path
   # could be a file or a non-existent path, we cannot call 'pwd -W' directly
   # on the path.  Instead, we extract the root path (i.e. "c:/"), call 'pwd -W'
   # on it, then merge with the rest of the path.
   root-path = $(shell echo $(1) | sed -e "s|\(/[^/]*\)/\?\(.*\)|\1|")
   non-root-path = $(shell echo $(1) | sed -e "s|\(/[^/]*\)/\?\(.*\)|\2|")
   normalizepath = $(if $(filter /%,$(1)),$(shell cd $(call root-path,$(1)) && pwd -W)/$(call non-root-path,$(1)),$(1))
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
                $(CP) -P -f -p --parents $(files_list) $(CLONEDIR) \
                $(NULL)
endif

export:: clone_dir

clone_dir:
	$(warning WARNING: $@ is DEPRECATED and slated for REMOVAL; you have been warned...)
	$(MKDIR) $(CLONEDIR)
	$(clone_dir_cmd)

.PHONY : clone_dir

endif #CLONEDIR

ifdef SONGBIRD_TEST_COMPONENT
SONGBIRD_TESTSDIR := $(SONGBIRD_TESTSDIR)/$(SONGBIRD_TEST_COMPONENT)
endif #SONGBIRD_TEST_COMPONENT
copy_sb_tests:
ifdef SONGBIRD_TESTS
ifneq (,$(SB_ENABLE_TESTS))
ifeq (,$(wildcard $(SONGBIRD_TESTSDIR)))
	$(MKDIR) $(SONGBIRD_TESTSDIR)
endif
	$(INSTALL_FILE) $(SONGBIRD_TESTS) $(SONGBIRD_TESTSDIR)
endif
.PHONY : copy_sb_tests
endif #SONGBIRD_TESTS

#------------------------------------------------------------------------------
# Rules for preprocessing
#------------------------------------------------------------------------------

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
    $(RM) -f $(SONGBIRD_DOCUMENTATIONDIR)/$$file; \
  done

.PHONY : run_doxygen_preprocess clean_doxygen_preprocess

endif #DOXYGEN_PREPROCESS

#-----------------------


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
	$(RM) -f $(JAR_MANIFEST)
	$(PERL) $(MOZSDK_SCRIPTS_DIR)/preprocessor.pl $(ACDEFINES) $(PPDEFINES) -- \
    $(srcdir)/$(jar_manifest_in) | \
    $(PERL) $(SCRIPTS_DIR)/expand-jar-mn.pl $(srcdir) > $(JAR_MANIFEST)
endif

make_jar: $(JAR_MANIFEST)
	$(MKDIR) $(TARGET_DIR)
	$(PERL) -I$(MOZSDK_SCRIPTS_DIR) $(MOZSDK_SCRIPTS_DIR)/make-jars.pl \
      $(MAKE_JARS_FLAGS) -- $(ACDEFINES) $(PPDEFINES) < $(jar_manifest_file)
	@$(RM) -rf $(TARGET_DIR)/stage


clean_jar_postprocess:
	$(RM) -f ./$(JAR_MANIFEST)

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
	$(MKDIR) $(EXTENSION_STAGE_DIR)
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
	$(INSTALL_FILE) $(install_rdf_file) $(EXTENSION_STAGE_DIR)/install.rdf
 ifneq (,$(EXTENSION_LICENSE))
	$(INSTALL_FILE) $(EXTENSION_LICENSE) $(EXTENSION_STAGE_DIR)
 endif
	cd $(EXTENSION_STAGE_DIR) && $(ZIP) -qr ../$(XPI_NAME).xpi.tmp *
	$(MKDIR) $(EXTENSION_DIR)
	$(MV) -f $(EXTENSION_STAGE_DIR)/../$(XPI_NAME).xpi.tmp \
        $(EXTENSION_DIR)/$(XPI_NAME).xpi
 ifdef INSTALL_EXTENSION
	$(MKDIR) $(SONGBIRD_EXTENSIONSDIR)
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
# Rules for making directories
#------------------------------------------------------------------------------

# CREATEDIRS - set to a list of directories to create

create_dirs:
	$(warning WARNING: $@ is DEPRECATED and slated for REMOVAL; you have been warned...)
	$(foreach dir,$(CREATEDIRS),$(MKDIR) $(dir);)

.PHONY : create_dirs

create_dirs_clean:
	$(RM) -rf $(CREATEDIRS)

.PHONY : create_dirs_clean
