#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <lualib.h>
#include <sys/stat.h>
#include <lauxlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <edcl.h>

#ifndef EDCL_WINDOWS
#include <sys/ioctl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <ifaddrs.h>
#endif

#define CHECK() printf("Current stack top:  %d (%s:%d)\n", lua_gettop(L),__FILE__, __LINE__);

void report_errors(lua_State *L, int status) {
        if ( status!=0 ) {
                printf("Looks like an error in lua code: %s \n", lua_tostring(L, -1));
                lua_pop(L, 1); // remove error message
        }
}

void usage(char* app)
{
	printf(
		"EDCL scripting interpreter\n"
		"Usage %s [options]\n"
#ifndef EDCL_WINDOWS 
		"-i interface      Use this interface (default - eth0) \n"
#else
		"-i interface      Use this interface (Number). Use -l to list them\n"
#endif
		"-b address        Specify board address (default - 192.168.0.0) \n"
		"-s address        Specify self address (default - 192.168.0.1) \n"
		"-t                Enter interactive terminal mode \n"
		"-l                List available network interfaces"
		"-h                Show this useless help message \n"
		"See SCRIPTING.TXT for avaliable lua functions\n"
		"(c) Andrew Andrianov <andrianov@module.ru> @ RC Module 2012\n", app
		);
	exit(1);
}

#ifndef EDCL_WINDOWS 
static	char* default_iface = "eth0";
#else
static	char* default_iface = "0";
#endif


static int l_edcl_init (lua_State *L) {
	int r = edcl_init(default_iface);
	if (-1==r)
		perror("edcl_init:"), exit(EXIT_FAILURE);
	return 0;  /* number of results */
}


static int l_edcl_writestring (lua_State *L) {
	int argc = lua_gettop(L);
	if (argc!=2)
		printf("FATAL: incorrect number of args to edlc_write\n"),exit(EXIT_FAILURE);
	unsigned int addr = lua_tonumber(L, 1);
	const char* hexstr = lua_tostring(L, 2);
	edcl_write(addr, hexstr, strlen(hexstr)+1);	
	return 0;
}


static int l_edcl_filesize(lua_State *L) {
	struct stat st;
	uint64_t size; 
	int ret;
	int argc = lua_gettop(L);
	if (argc!=1)
		printf("FATAL: incorrect number of args to edlc_fsize\n"),exit(EXIT_FAILURE);
	const char* filename = lua_tostring(L, 1);
	printf("%s\n", filename);
	ret = stat(filename, &st);
	if (ret == 0) { 
		size = (uint64_t) st.st_size;
		lua_pushnumber(L,size);
	} else {
		lua_pushnumber(L,-1);
		perror("stat");
	}
	return 1;
}

static int l_edcl_write (lua_State *L) {
	
	int argc = lua_gettop(L);
	if (argc!=3)
		printf("FATAL: incorrect number of args to edlc_write\n"),exit(EXIT_FAILURE);
	int bytes = lua_tonumber(L, 1);
	uint64_t v64;
	uint32_t v32;
	uint16_t v16;
	uint8_t v8;
	unsigned int addr = lua_tonumber(L, 2);
	int ret,i;
	const char* hexstr;
	char* buffer;
	char tmp[3];
	tmp[2]=0x00; /* strncpy will not do this for us */
	switch (bytes)
	{
	case 0:
		hexstr = lua_tostring(L, 3);
		printf("%s\n", hexstr);
		ret = strlen(hexstr);
		if (ret % 2) printf("FATAL: for hex string format you need one more character\n"), exit(EXIT_FAILURE);
		buffer = malloc(ret / 2 );
		/* this impl. sucks. But who f*cking cares? */
		for (i=0; i<ret/2; i++) {
			int j = i*2;
			strncpy(tmp,&hexstr[j],2);
			sscanf(tmp, "%hhx", &buffer[i]);
		}
		
		ret = edcl_write(addr, buffer, ret/2);
		free(buffer);
		break;
	case 1:
		/* One byte write */
		v8 = (uint8_t) lua_tonumber(L,3);
		ret = edcl_write(addr, &v8, bytes);
		break;
	case 2:
		v16 = (uint16_t) lua_tonumber(L,3);
		ret = edcl_write(addr, &v16, bytes);
		break;
	case 4:
		v32 = (uint32_t) lua_tonumber(L,3);
		ret =edcl_write(addr, &v32, bytes);
		break;
	case 8:
		v64 = (uint64_t) lua_tonumber(L,3);
		ret =edcl_write(addr, &v32, bytes);
		break;
	default:
		printf("unsupported write op for edcl_write\n");
		exit(EXIT_FAILURE);
	}

//	printf("%d bytes to %x done with %d\n", bytes, addr, ret);
	if (ret == -1) {
		printf("Ooops: edcl_write returned -1, restarting edcl\n");
		edcl_init(default_iface);
		return l_edcl_write(L);;
	}
	
	return 0;  /* number of results */
}



static int poll_interval = 100000;

static int l_edcl_setwait(lua_State *L) {
       int argc = lua_gettop(L);
       if (argc!=1)
               printf("FATAL: incorrect number of args to edlc_poll_interval\n"),exit(EXIT_FAILURE);
       poll_interval = lua_tonumber(L, 1);
       return 0;
}


static int l_edcl_nwait (lua_State *L) {
	int argc = lua_gettop(L);
	if (argc!=3)
		printf("FATAL: incorrect number of args to edlc_wait\n"),exit(EXIT_FAILURE);
	int bytes = lua_tonumber(L, 1);
	unsigned int addr = lua_tonumber(L, 2);
	unsigned int expected = lua_tonumber(L, 3);
	unsigned int v32;
	unsigned short v16;
	unsigned char v8;
	int ret = 0;
	while (1)
	{
		switch (bytes)
		{
		case 1:
			ret = edcl_read(addr, &v8, 1);
			if (v8 != (unsigned char) expected) return 0;
			break;
		case 2:
			ret = edcl_read(addr, &v16, 2);
			if (v16 != (unsigned short) expected) return 0;
			break;
		case 4:
			ret = edcl_read(addr, &v32, 4);
			if (v32 != (unsigned int) expected) return 0;
			break;
		default:
			printf("FATAL: unexpected op to edcl_wait\b"); 
			exit(EXIT_FAILURE);
		}
		if (ret == -1)
		{
			edcl_init(default_iface);
		}
		usleep(poll_interval);
	}
	return 0;
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
	int ret = 0;
	while (1)
	{
		switch (bytes)
		{
		case 1:
			ret = edcl_read(addr, &v8, 1);
			if (v8 == (unsigned char) expected) return 0;
			break;
		case 2:
			ret = edcl_read(addr, &v16, 2);
			if (v16 == (unsigned short) expected) return 0;
			break;
		case 4:
			ret = edcl_read(addr, &v32, 4);
			if (v32 == (unsigned int) expected) return 0;
			break;
		default:
			printf("FATAL: unexpected op to edcl_wait\b"); 
			exit(EXIT_FAILURE);
		}
		if (ret == -1)
		{
			//	printf("oops, restarting edcl..\n");
			edcl_init(default_iface);
		}
		usleep(poll_interval);
	}
	return 0;
}



static int l_edcl_read (lua_State *L) {
	int argc = lua_gettop(L);
	if (argc!=2)
		printf("FATAL: incorrect number of args to edlc_read\n"),exit(EXIT_FAILURE);
	int bytes = lua_tonumber(L, 1);
	unsigned int addr = lua_tonumber(L, 2);
	char tmp[8];
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
	case 8:
		edcl_read(addr, tmp, 8);
		lua_pushnumber(L,* (unsigned int*) &tmp[0]);
		break;
	}

	return 1;
	
}



static void display_progressbar(int max, int value)
{
	float percent = 100.0 - (float) value * 100.0 / (float) max; 

#ifndef EDCL_WINDOWS
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	
	int txt = printf("\r %.02f %% done [", percent);
	int max_bars = w.ws_col - txt - 7;
	int bars = max_bars - (int)(((float) value * max_bars) / (float) max);

	if (max_bars > 0) {
		int i;		
		for (i=0; i<bars; i++)
			printf("#");
		for (i=bars; i<max_bars; i++)
			printf(" ");
		printf("]");
	}
#else
	printf("\r%.02f %% complete");
#endif
	fflush(stdout);

}



static int l_edcl_upload_chunk (lua_State *L) {
	int argc = lua_gettop(L);

	if (argc!=4 && argc!=5)
		printf("FATAL: incorrect number of args to edlc_upload\n"),exit(EXIT_FAILURE);	

	unsigned int addr = lua_tonumber(L, 1);
	const char* filename = lua_tostring(L, 2);
	unsigned int offset = lua_tonumber(L, 3);
	unsigned int len = lua_tonumber(L, 4);
	int silent = 0;
	if (argc>4)
		silent = lua_tonumber(L, 5);
		
	if (0!=access(filename, R_OK)) {
		lua_pushnumber(L, -1);
		return 1;
	}

	struct stat buf;
	stat(filename, &buf);
	unsigned int sz = (unsigned int) buf.st_size;
	FILE* fd = fopen(filename, "r");
	char tmp[2048];
	int n, k; 
	int maxsz = sz;
	int written=0;
	fseek(fd, offset, SEEK_SET);
	sz -= offset;
	if (sz > len)
		sz = len;
	while (sz)
	{
		n = fread(tmp, 1, (sz > edcl_get_max_payload()) ? edcl_get_max_payload() : sz, fd);
		if (n<=0)
			break;
	retry:
		k = edcl_write(addr, tmp, n);
		//printf("write %x %d %d\n", addr, n, sz);
		if (k<0) 
		{
			edcl_init(default_iface);
			goto retry;
		}

		addr+=n;
		sz-=n;
		written += n;
		if (!silent)
			display_progressbar(maxsz, maxsz - offset);
		//	sleep(1);
	}

	if (offset + written == maxsz) { 
		if (!silent) {
			display_progressbar(maxsz, 0);
			printf(" done\n");
		}
	}
	fclose(fd);
	lua_pushnumber(L, written);
	return 1;
}


static int l_edcl_upload (lua_State *L) {
	int argc = lua_gettop(L);
	if (argc!=2)
		printf("FATAL: incorrect number of args to edlc_upload\n"),exit(EXIT_FAILURE);	
	unsigned int addr = lua_tonumber(L, 1);
	const char* filename = lua_tostring(L, 2);
	printf("Uploading %s to 0x%X\n", filename, addr);
	if (0!=access(filename, R_OK)) {
		lua_pushnumber(L,-1);
		return 1;
	}
	struct stat buf;
	stat(filename, &buf);
	unsigned int sz = (unsigned int) buf.st_size;
	printf("Filesize: %d bytes\n", sz);
	FILE* fd = fopen(filename, "r");
	char tmp[2048];
	int n, k; 
	int maxsz = sz;
	fflush(stdout);
	while (sz)
	{
		n = fread(tmp, 1, edcl_get_max_payload(), fd);
	retry:
		k = edcl_write(addr, tmp, n);
		if (k<0) 
		{
			edcl_init(default_iface);
			goto retry;
		}

		addr+=n;
		sz-=n;
		display_progressbar(maxsz,sz);
	}
	display_progressbar(maxsz, 0);
	printf(" done\n");
	lua_pushnumber(L,0);
	fclose(fd);
	return 1;
}


static int l_edcl_download (lua_State *L) {
	int argc = lua_gettop(L);
	if (argc!=3)
		printf("FATAL: incorrect number of args to edlc_download\n"),exit(EXIT_FAILURE);
	unsigned int addr = lua_tonumber(L, 1);
	const char* filename = lua_tostring(L, 2);
	unsigned int sz = lua_tonumber(L, 3);
	printf("Downloading 0x%X-0x%X to %s\n", addr, addr+sz, filename);
	printf("Filesize: %d bytes\n", sz);
	FILE* fd = fopen(filename, "w+");
	char tmp[2048];
	int n,i=0; 
	fflush(stdout);
	int maxsz = sz;
	while (sz)
	{
		n = (sz >= edcl_get_max_payload()) ? edcl_get_max_payload() : sz;
		int k = edcl_read(addr, tmp, n);
		fwrite(tmp, 1, n, fd);
		addr+=n;
		sz-=n;
		display_progressbar(maxsz,sz); 
	}
	printf("\n Download complete \n");
	lua_pushnumber(L,0);
	fclose(fd);
	return 1;
}


static int l_edcl_hexdump (lua_State *L) {
	int argc = lua_gettop(L);
	if (argc!=3)
		printf("FATAL: incorrect number of args to edlc_download\n"),exit(EXIT_FAILURE);
	unsigned int addr = lua_tonumber(L, 1);
	const char* filename = lua_tostring(L, 2);
	unsigned int sz = lua_tonumber(L, 3);
	printf("Downloading 0x%X-0x%X to %s\n", addr, addr+sz, filename);
	printf("Filesize: %d bytes\n", sz);
	FILE* fd = fopen(filename, "w+");
	char tmp[128];
	int n, i;
	i=0;
	printf("Running file download\n");
	fflush(stdout);
	while (sz)
	{
		n = (sz >= 128) ? 128 : sz;
		int k = edcl_read(addr, tmp, n);
		fwrite(tmp, 1, n, fd);
		addr+=n;
		sz-=n;
		i++;
		
		if (i % 64 == 0)
			printf("\n %d bytes | ", i*128 );
		printf("#");
		fflush(stdout);
	}
	printf("\n Download complete");
	lua_pushnumber(L,0);
	return 1;
}


void bind_edcl_functions(lua_State* L){
	lua_pushcfunction(L, l_edcl_init);
	lua_setglobal(L, "edcl_init");	
	lua_pushcfunction(L, l_edcl_write);
	lua_setglobal(L, "edcl_write");	
	lua_pushcfunction(L, l_edcl_wait);
	lua_setglobal(L, "edcl_wait");	
	lua_pushcfunction(L, l_edcl_nwait);
	lua_setglobal(L, "edcl_nwait");	
	lua_pushcfunction(L, l_edcl_read);
	lua_setglobal(L, "edcl_read");		
	lua_pushcfunction(L, l_edcl_setwait);
	lua_setglobal(L, "edcl_poll_interval");	
	lua_pushcfunction(L, l_edcl_upload);
	lua_setglobal(L, "edcl_upload");	
	lua_pushcfunction(L, l_edcl_download);
	lua_setglobal(L, "edcl_download");	
	lua_pushcfunction(L, l_edcl_hexdump);
	lua_setglobal(L, "edcl_hexdump");
	lua_pushcfunction(L, l_edcl_writestring);
	lua_setglobal(L, "edcl_writestring");
	lua_pushcfunction(L, l_edcl_upload_chunk);
	lua_setglobal(L, "edcl_upload_chunk");
	lua_pushcfunction(L, l_edcl_filesize);
	lua_setglobal(L, "edcl_filesize");
	//l_elf_register(L);
}


#ifdef HAVE_INTERACTIVE
void interactive_loop(lua_State* L) {

	char* input, shell_prompt[100];
	for(;;)
	{
		snprintf(shell_prompt, sizeof(shell_prompt), "edcl # " );
		input = readline(shell_prompt);
		if (input==NULL)
			break;
		rl_bind_key('\t', rl_complete);
		add_history(input);
		int error = luaL_loadbuffer(L, input, strlen(input), "shell") ||
			lua_pcall(L, 0, 0, 0);
		report_errors(L,error);
 
		free(input);
	}
}
#else
void interactive_loop(lua_State* L) {
	printf("Interactive mode disabled at compile-time or not supported on this platform\n");
}

#endif


#ifndef EDCL_WINDOWS
const char setup_paths[] =						\
	"package.path = '" LUAEXT_PREFIX "/?.lua;'..package.path\n" ;
#else
const char setup_paths[] =						\
	"package.path = 'lib/?.lua;'..package.path\n" ;
#endif


#define PROGNAME "edcltool"
int main(int argc, char** argv) {
	
	size_t address;
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
	bind_edcl_functions(L);
	int opt;
	if (argc<2){
		usage(argv[0]);
	}
	const char* file = NULL;
	int term = 0;
	while ((opt = getopt(argc, argv, "thi:s:b:f:l")) != -1) {
                switch (opt) {
		case 'f':
			file = optarg;
			break;
		case 't':
			term=1;
			break;
		case 'i':
			default_iface = optarg;
			break;
		case 'l':
			edcl_platform_list_interfaces();
			exit(1);
			break;
		case 'h':
		default:
			usage(argv[0]);
		}
	};

	luaL_loadbuffer(L, setup_paths, strlen(setup_paths), "shell") ||
		lua_pcall(L, 0, 0, 0);

	printf("Loading edcl script: %s\n", file);
        int s = luaL_loadfile(L, file);
	
        printf("Done with result %d\n", s);
        if ( s==0 ) {
                s = lua_pcall(L, 0, LUA_MULTRET, 0);
		if ( s!=0 ) 
			report_errors(L, s);
        }
        report_errors(L, s);
	if (term)
		interactive_loop(L);
	return 1;
}
