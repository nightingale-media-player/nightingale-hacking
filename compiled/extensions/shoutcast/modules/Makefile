DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/extensions/shoutcast/modules
VPATH = /media/disk/Linux/makepkg/songbird/extensions/shoutcast/modules

include $(DEPTH)/build/autodefs.mk

IS_EXTENSION = 1

MODULES_DIR = $(EXTENSION_STAGE_DIR)/modules

CREATEDIRS = $(MODULES_DIR)

include $(topsrcdir)/build/rules.mk

export::
	$(INSTALL_FILE) $(srcdir)/Utils.jsm $(MODULES_DIR)
