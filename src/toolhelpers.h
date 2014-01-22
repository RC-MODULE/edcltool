#ifndef TOOLHELPERS_H
#define TOOLHELPERS_H


#define L_REG(_fname)				\
	lua_pushcfunction(L, l_ ## _fname);	\
	lua_setglobal(L, ## _fname);

#define ERROR(fmt, ...) {						\
		fprintf(stderr,  fmt, ##__VA_ARGS__);					\
		return 0;						\
	}



#define CHECK_ARGCOUNT(lfunc, nargs)					\
	int argc = lua_gettop(L);					\
	if ( argc!=nargs ) {						\
		fprintf(stderr, lfunc ": incorrect number of arguments. Expected: %d got: %d\n", nargs, argc); \
		return 0;						\
	}
#endif
