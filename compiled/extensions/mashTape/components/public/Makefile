#preedTODO: add copyright
DEPTH = ../../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/extensions/mashTape/components/public
VPATH = /media/disk/Linux/makepkg/songbird/extensions/mashTape/components/public

include $(DEPTH)/build/autodefs.mk

IS_EXTENSION = 1

# compile these IDL files
XPIDL_SRCS = mashTape.idl \
             $(NULL)

# into this xpt (an xpt is like a compiled IDL read at runtime)
XPIDL_MODULE = mashTape.xpt

include $(topsrcdir)/build/rules.mk

