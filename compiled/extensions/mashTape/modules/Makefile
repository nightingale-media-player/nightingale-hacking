# preedTODO: add copyright.
DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/extensions/mashTape/modules
VPATH = /media/disk/Linux/makepkg/songbird/extensions/mashTape/modules

include $(DEPTH)/build/autodefs.mk

IS_EXTENSION = 1

MT_MODULES_DIR = $(EXTENSION_STAGE_DIR)/modules

CREATEDIRS = $(MT_MODULES_DIR)

include $(topsrcdir)/build/rules.mk

export::
	$(INSTALL_FILE) $(srcdir)/mtUtils.jsm $(MT_MODULES_DIR)
