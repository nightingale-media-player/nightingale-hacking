DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/extensions/7digital/components
VPATH = /media/disk/Linux/makepkg/songbird/extensions/7digital/components

include $(DEPTH)/build/autodefs.mk

IS_EXTENSION = 1

SONGBIRD_PP_RESOURCES = $(srcdir)/sb7Digital.js.in
SONGBIRD_PP_DIR = $(SONGBIRD_COMPONENTSDIR)
PPDEFINES = -DEXTENSION_UUID="$(EXTENSION_UUID)"

include $(topsrcdir)/build/rules.mk
