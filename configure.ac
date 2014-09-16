AC_PREREQ([2.60])
AC_INIT([edcltool], [0.1], [andrew@ncrmnt.org])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_HEADERS([config.h])
AC_C_INLINE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_AR

LT_INIT

AC_MSG_NOTICE([ *** MADE IN USSR WITH LOVE *** ])
sleep 1

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/ioctl.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero gettimeofday memset strchr strdup strncasecmp strtoul])

AC_CONFIG_FILES([Makefile
                 src/Makefile
		 libedcl-1.0.pc])




found_lua=no

PKG_CHECK_MODULES([lua], [lua],
  found_lua=yes
  have_lua=yes
  ,
  have_lua=no
)

PKG_CHECK_MODULES([lua52], [lua5.2],
  found_lua=yes
  have_lua52=yes
  ,
  have_lua52=no
)

PKG_CHECK_MODULES([lua51], [lua5.1],
  found_lua=yes;
  have_lua51=yes
  ,
  have_lua51=no
)


# On CentOS we have no pkg-config file in libelf-devel
# So we handle this case with care

libelf_pkgconfig_lies=no
PKG_CHECK_MODULES([libelf], [libelf],
  have_libelf=yes
  ,
    AC_CHECK_HEADER(libelf.h,
    [AC_MSG_NOTICE([PKG-CONFIG is a liar! We have libelf headers and will try to use them!]) 
    			       have_libelf=yes
			       libelf_pkgconfig_lies=yes
    ],
    [
    AC_MSG_WARN([No libelf found in system. ELF loading will not be supported!])   
    			   	have_libelf=no
    ]
    )
)


AC_ARG_ENABLE([edcltool],
    AS_HELP_STRING([--disable-edcltool], [Do not build edcltool]))
AM_CONDITIONAL([BUILD_EDCLTOOL], [test x$found_lua = xyes    -a x$enable_edcltool != xno ])

AC_ARG_ENABLE([elfload],
    AS_HELP_STRING([--disable-elfload], [Do not build elf loader functions (nmc library will not work)]))
AM_CONDITIONAL([BUILD_ELFLOAD], [test x$have_libelf = xyes -a x$enable_elfload  != xno ])


if test $found_lua = no -a x$enable_edcltool != xno; then
    AC_MSG_WARN([Couldn't find any lua of use. I won't be able to build edcltool])
    AC_MSG_ERROR([Either rerun configure with --disable-edcltool or install lua devel packages])	
fi

if test $have_libelf = no -a x$enable_elfload != xno; then
    AC_MSG_WARN([Couldn't find any useable libelf. edcltool will not be able to operate with elf files])
    AC_MSG_WARN([In other words - you won't be able to upload .abs files to nmc core])
    AC_MSG_ERROR([Either rerun configure with --disable-elfload or install libelf devel packages])		
fi



if test $have_libelf = yes; then
if test $libelf_pkgconfig_lies = yes; then
   LIBELF_LIBS=-lelf
else
   LIBELF_CFLAGS=$libelf_CFLAGS
   LIBELF_LIBS=$libelf_LIBS
fi
   HAVE_LIBELF=yes
   AC_MSG_NOTICE([Found a usable libelf, good])
fi

if test $have_lua = yes; then
   LUA_CFLAGS=$lua_CFLAGS
   LUA_LIBS=$lua_LIBS
   AC_MSG_NOTICE([Compiling against default lua installation])
else if test $have_lua52 = yes; then
   LUA_CFLAGS=$lua52_CFLAGS
   LUA_LIBS=$lua52_LIBS
   AC_MSG_NOTICE([Compiling against lua52])
else if test $have_lua51 = yes; then
   LUA_CFLAGS=$lua51_CFLAGS
   LUA_LIBS=$lua51_LIBS
   AC_MSG_NOTICE([Conmpiling against lua51])
fi
fi
fi


AC_SUBST([LUA_LIBS])
AC_SUBST([LUA_CFLAGS])
AC_SUBST([LIBELF_LIBS])
AC_SUBST([LIBELF_CFLAGS])

AC_OUTPUT
