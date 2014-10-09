
PACKAGES:=lua libelf

lua_ver=5.2.3
lua_FILE=lua-$(lua_ver).tar.gz
lua_URL=http://www.lua.org/ftp/$(lua_FILE)

libelf_ver=0.8.9
libelf_FILE=libelf-$(libelf_ver).tar.gz
libelf_URL=http://www.mr511.de/software/$(libelf_FILE)


define package_fetch
.stmp-downloaded-$(1):
	wget -O$(2) $(3)
	touch $$(@)
endef

define package_extract 
.stmp-extracted-$(1):
	mkdir -p $(1)
	tar vxpf $(2) -C $(1) --strip-components=1
	touch $$(@)
endef

define package_clean 
clean-$(1):
	-rm -Rfv $(1)
endef


define lua/compile
.stmp-compiled-$(1): 
	cd lua/src/; \
	FILES="`ls *.c | grep -v lua.c |grep -v luac.c`"; \
	for f in $$$${FILES}; do \
		$(CROSS_COMPILE)gcc -o $$$$f.o -c $$$$f; \
		echo "Compiling $$$$f"; \
	done; \
	$(CROSS_COMPILE)ar rcs liblua.a *.o
#	touch $$(@)
endef

define libelf/compile
.stmp-compiled-$(1): 
	cd libelf && \
	sed -i 's/w32\/Makefile//' configure.in && \
	autoreconf --install --force && \
	./configure --prefix=`pwd`/root/ $(GNU_TARGET_NAME) && \
	make && \
	make install
endef

$(foreach p,$(PACKAGES),$(eval $(call package_fetch,$(p),$($(p)_FILE),$($(p)_URL))))
$(foreach p,$(PACKAGES),$(eval $(call package_extract,$(p),$($(p)_FILE))))
$(foreach p,$(PACKAGES),$(eval $(call $(p)/compile,$(p))))
$(foreach p,$(PACKAGES),$(eval $(call package_clean,$(p))))


all: $(foreach p,$(PACKAGES), \
	$(addprefix .stmp-downloaded-,$(p))\
	$(addprefix .stmp-extracted-,$(p))\
	$(addprefix .stmp-compiled-,$(p))\
	)

clean: $(foreach p,$(PACKAGES), $(addprefix clean-,$(p)))
	-rm .stmp-compiled*
	-rm .stmp-extracted*
