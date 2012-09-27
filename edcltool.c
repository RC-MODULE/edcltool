#include <stdio.h>
#include <getopt.h>
#include <lualib.h>
#include <lauxlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "edcl.h"

#define CHECK() printf("Current stack top:  %d (%s:%d)\n", lua_gettop(L),__FILE__, __LINE__);






void report_errors(lua_State *L, int status) {
        if ( status!=0 ) {
                printf("ooops: %s \n", lua_tostring(L, -1));
                lua_pop(L, 1); // remove error message
        }
}




void usage(char* app)
{
	printf(
		"EDCL scripting interpreter\n"
		"Usage %s [options]\n"
		"-i interface\t Use interface (default - eth0) \n"
		"-b address \t Specify board address (default - 192.168.0.0) \n"
		"-s address \t Specify self address (default - 192.168.0.1) \n"
		"-h\t This help \n"
		"See SCRIPTING.TXT for avaliable lua functions\n"
		"(c) Andrew Andrianov <andrianov@module.ru> @ RC Module 2012\n", app
		);
	exit(1);
}

static	char* default_iface = "eth0";
static  char* default_baddr = "192.168.0.0";
static 	char* default_saddr = "192.168.0.1";


static int l_edcl_init (lua_State *L) {
	int r = edcl_init(default_iface, default_baddr, default_saddr);
	if (-1==r)
		perror("edcl_init:"), exit(EXIT_FAILURE);
	return 0;  /* number of results */
}

static int l_edcl_write (lua_State *L) {
	int argc = lua_gettop(L);
	if (argc!=3)
		printf("FATAL: incorrect number of args to edlc_write\n"),exit(EXIT_FAILURE);
	int bytes = lua_tonumber(L, 1);
	unsigned int v32;
	unsigned short v16;
	unsigned char v8;
	unsigned int addr = lua_tonumber(L, 2);
	int ret;
	switch (bytes)
	{
	case 1:
		/* One byte write */
		v8 = (char) lua_tonumber(L,3);
		ret = edcl_write(addr, &v8, bytes);
		break;
	case 2:
		v16 = (unsigned short) lua_tonumber(L,3);
		ret = edcl_write(addr, &v16, bytes);
		break;
	case 4:
		v32 = (unsigned int) lua_tonumber(L,3);
		ret =edcl_write(addr, &v32, bytes);
		break;
	default:
		printf("unsupported write op for edcl_write\n");
		exit(EXIT_FAILURE);
	}
	printf("%d bytes to %x done with %d\n", bytes, addr, ret);
	int value = lua_tonumber(L, 3);
	return 0;  /* number of results */
}

static int l_edcl_wait (lua_State *L) {
	int argc = lua_gettop(L);
	if (argc!=3)
		printf("FATAL: incorrect number of args to edlc_wait\n"),exit(EXIT_FAILURE);
	int bytes = lua_tonumber(L, 1);
	unsigned int addr = lua_tonumber(L, 2);
	unsigned int expected = lua_tonumber(L, 3);
	unsigned int v32;
	unsigned short v16;
	unsigned char v8;
	int found = 0;
	while (!found)
	{
		switch (bytes)
		{
		case 1:
			edcl_read(addr, &v8, 1);
			if (v8 == (unsigned char) expected) found++;
			break;
		case 2:
			edcl_read(addr, &v16, 2);
			if (v16 == (unsigned short) expected) found++;
			break;
		case 4:
			edcl_read(addr, &v32, 4);
			if (v32 == (unsigned int) expected) found++;
			break;
		default:
			printf("FATAL: unexpected op to edcl_wait\b"); 
			exit(EXIT_FAILURE);
		}
		
		sleep(1);
	}
	return 0;
}


static int l_edcl_read (lua_State *L) {
	int argc = lua_gettop(L);
	if (argc!=2)
		printf("FATAL: incorrect number of args to edlc_read\n"),exit(EXIT_FAILURE);
	int bytes = lua_tonumber(L, 1);
	unsigned int addr = lua_tonumber(L, 2);
	char tmp[4];
	switch(bytes)
	{
	case 1:
		edcl_read(addr, tmp, 1);
		unsigned char r = tmp[0];
		lua_pushnumber(L,r);
		return 1;
	case 2:
		edcl_read(addr, tmp, 2);
		lua_pushnumber(L,* (unsigned short*) &tmp[0]);
		break;
	case 4:
		edcl_read(addr, tmp, 4);
		lua_pushnumber(L,* (unsigned int*) &tmp[0]);
		break;
	}
	return 1;
	
}
	

void bind_edcl_functions(lua_State* L){
	lua_pushcfunction(L, l_edcl_init);
	lua_setglobal(L, "edcl_init");	
	lua_pushcfunction(L, l_edcl_write);
	lua_setglobal(L, "edcl_write");	
	lua_pushcfunction(L, l_edcl_wait);
	lua_setglobal(L, "edcl_wait");	
	lua_pushcfunction(L, l_edcl_read);
	lua_setglobal(L, "edcl_read");		
}


#define PROGNAME "edcltool"
int main(int argc, char** argv) {
	
	size_t address;
        lua_State* L = lua_open();
        luaL_openlibs(L);
	bind_edcl_functions(L);
	int opt;
	if (argc<2){
		usage(argv[0]);
	}
	char* tmp = strdup(argv[1]);
	
	if ((0 != access(tmp,R_OK) || (0 == strcmp(PROGNAME, basename(tmp))))){
		/* Not running as interpreter */
		fprintf(stderr, "Please run this tool as interpreter for edcl script\n");
		fprintf(stderr, "See SCRIPTING.TXT for manual\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
	} 
	
	while ((opt = getopt(argc, argv, "hi:s:b:")) != -1) {
                switch (opt) {
		case 'i':
			default_iface = optarg;
			break;
		case 'b':
			default_baddr = optarg;
			break;
		case 's':
			default_saddr = optarg;
			break;
		case 'h':
		default:
			usage(argv[0]);
		}
	};
	
	printf("Loading edcl script: %s\n", tmp);
        int s = luaL_loadfile(L, tmp);
        printf("Done with result %d\n", s);
        if ( s==0 ) {
                s = lua_pcall(L, 0, LUA_MULTRET, 0);
		return s;
        }
        report_errors(L, s);
	return 1;
	



/* 
   if(argc < 3 || sscanf(argv[2], "%x", &address) != 1) {
   fprintf(stderr, "usage: %s interface address\n", argv[0]);
   return -1;
   }

   if(edcl_init(argv[1])) {
   perror("edcl_init");
   return -1;
   }

   for(;;) {
   char buf[256];
   size_t n = fread(buf, 1, sizeof(buf), stdin);

   if(n == 0) break;

   if(edcl_write(address, buf, n)) {
   perror("edcl_write");
   return -1;
   }

   address += n;
   }

*/
	return 0;
}
