lib_LTLIBRARIES = libedcl-1.0.la
libedcl_1_0_la_SOURCES = edcl.c
libedcl_1_0_la_includedir=$(includedir)/edcl-1.0/
libedcl_1_0_la_include_HEADERS = edcl.h

bin_PROGRAMS = edcltool
edcltool_CPPFLAGS = $(AMCFLAGS) $(LUA_CFLAGS)
edcltool_LDADD = $(LUA_LIBS) -lpthread libedcl-1.0.la
edcltool_SOURCES = edcltool.c
