pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libedcl-1.0.pc

SUBDIRS = src

lualibdir=@datadir@/edcltool
luafiles= \
	lib/fw.lua \
	lib/nmc.lua

EXTRA_DIST = $(luafiles)

install-data-local:
	for file in $(luafiles); do \
	$(INSTALL) -D $$file $(lualibdir)/$$file ;\
	done 