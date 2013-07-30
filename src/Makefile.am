lib_LTLIBRARIES = libedcl-1.0.la
libedcl_1_0_la_SOURCES = edcl.c
libedcl_1_0_la_includedir=$(includedir)/edcl-1.0/
libedcl_1_0_la_include_HEADERS = edcl.h

bin_cflags = $(AMCFLAGS) $(LUA_CFLAGS)
bin_ldadd = $(LUA_LIBS) -lpthread libedcl-1.0.la

bin_PROGRAMS=

bin_PROGRAMS += edcltool
edcltool_LDADD = $(bin_ldadd)
edcltool_CPPFLAGS = $(bin_cflags)
edcltool_SOURCES = edcltool.c

bin_PROGRAMS += edcl_write
edcl_write_LDADD = $(bin_ldadd)
edcl_write_CPPFLAGS = $(bin_cflags)
edcl_write_SOURCES = edcl_write.c

bin_PROGRAMS += edcl_dump
edcl_dump_LDADD = $(bin_ldadd)
edcl_dump_CPPFLAGS = $(bin_cflags)
edcl_dump_SOURCES = edcl_dump.c

bin_PROGRAMS += edcl_test
edcl_test_LDADD = $(bin_ldadd)
edcl_test_CPPFLAGS = $(bin_cflags)
edcl_test_SOURCES = edcl_test.c

