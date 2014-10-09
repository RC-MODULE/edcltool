#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>
#include <lauxlib.h>
#include <lualib.h>

#include "toolhelpers.h"

#define ERR -1


struct edcl_elf {
	int fd;
	Elf *elf;
	GElf_Ehdr elf_header;
};


static int l_elf_open(lua_State *L) 
{
	CHECK_ARGCOUNT("edcl_elf_open", 1); 
	const char* filename = lua_tostring(L, 1);

	int fd; 
        if ((fd = open(filename, O_RDONLY)) == ERR) {
		perror("open");
		ERROR("failed to open file\n");
	}

	struct edcl_elf *e = malloc(sizeof(struct edcl_elf));
	if (!e) {
		ERROR("malloc failed\n");
	}
	
	e->fd = fd;

	/* Check libelf version first */
	if(elf_version(EV_CURRENT) == EV_NONE)
	{
		fprintf(stderr, "WARNING: Elf Library is out of date!\n");
	}
	
	if ((e->elf = elf_begin(fd, ELF_C_READ , NULL)) == NULL) {
		ERROR("elf_begin() failed: %s.",
		      elf_errmsg(-1));
	}

	int i;
	char *id;
	GElf_Ehdr ehdr; 
	
	if (gelf_getehdr(e->elf, &ehdr) == NULL) 
		ERROR( "getehdr() failed: %s.",
		     elf_errmsg(-1));
	e->elf_header = ehdr;
	if ((i = gelf_getclass(e->elf)) == ELFCLASSNONE) 
		ERROR( "getclass() failed: %s.",
		     elf_errmsg(-1));
	
	(void) printf("%d-bit ELF object\n",
		      i == ELFCLASS32 ? 32 : 64);
	
	if ((id = elf_getident(e->elf, NULL)) == NULL) 
		ERROR( "getident() failed: %s.",
		     elf_errmsg(-1));
	
	printf("Machine id: 0x%x\n", ehdr.e_machine);

	lua_pushlightuserdata (L, e);	
	lua_pushnumber(L, ehdr.e_entry);
	return 2;
}


static int l_elf_getsections(lua_State *L) 
{
	CHECK_ARGCOUNT("edcl_elf_getsection", 1);
	struct edcl_elf *l = lua_touserdata(L, 1);
	int i=1;
	Elf_Scn *scn = NULL;                   /* Section Descriptor */
	GElf_Shdr shdr;                 /* Section Header */
	l->elf = elf_begin(l->fd, ELF_C_READ , NULL);
	
	
	lua_newtable(L);
	
	while((scn = elf_nextscn(l->elf, scn)) != 0)
	{
		lua_newtable(L);

		gelf_getshdr(scn, &shdr);
		
                switch(shdr.sh_type)
                {
		case SHT_NULL: lua_pushstring(L, "SHT_NULL");                  break;
		case SHT_PROGBITS: lua_pushstring(L, "SHT_PROGBITS");          break;
		case SHT_SYMTAB: lua_pushstring(L, "SHT_SYMTAB");              break;
		case SHT_STRTAB: lua_pushstring(L, "SHT_STRTAB");              break;
		case SHT_RELA: lua_pushstring(L, "SHT_RELA");                  break;
		case SHT_HASH: lua_pushstring(L, "SHT_HASH");                  break;
		case SHT_DYNAMIC: lua_pushstring(L, "SHT_DYNAMIC");            break;
		case SHT_NOTE: lua_pushstring(L, "SHT_NOTE");                  break;
		case SHT_NOBITS: lua_pushstring(L, "SHT_NOBITS");              break;
		case SHT_REL: lua_pushstring(L, "SHT_REL");                    break;
		case SHT_SHLIB: lua_pushstring(L, "SHT_SHLIB");                break;
		case SHT_DYNSYM: lua_pushstring(L, "SHT_DYNSYM");              break;
		case SHT_INIT_ARRAY: lua_pushstring(L, "SHT_INIT_ARRAY");      break;
		case SHT_FINI_ARRAY: lua_pushstring(L, "SHT_FINI_ARRAY");      break;
		case SHT_PREINIT_ARRAY: lua_pushstring(L, "SHT_PREINIT_ARRAY");break;
		case SHT_GROUP: lua_pushstring(L, "SHT_GROUP");                break;
		case SHT_SYMTAB_SHNDX: lua_pushstring(L, "SHT_SYMTAB_SHNDX");  break;
		case SHT_NUM: lua_pushstring(L, "SHT_NUM");                    break;
		case SHT_LOOS: lua_pushstring(L, "SHT_LOOS");                  break;
		case SHT_GNU_verdef: lua_pushstring(L, "SHT_GNU_verdef");      break;
		case SHT_GNU_verneed: lua_pushstring(L, "SHT_VERNEED");        break;
		case SHT_GNU_versym: lua_pushstring(L, "SHT_GNU_versym");      break;
		default: lua_pushstring(L, "(none) ");                         break;
                }
	
		lua_setfield(L, -2, "type");
	
		char flags[] = "----";

                if(shdr.sh_flags & SHF_WRITE) {     flags[0] = 'W'; }
                if(shdr.sh_flags & SHF_ALLOC) {     flags[1] = 'A'; }
                if(shdr.sh_flags & SHF_EXECINSTR) { flags[2] = 'X'; }
                if(shdr.sh_flags & SHF_STRINGS) {   flags[3] = 'S'; }

		lua_pushstring(L, flags);
		lua_setfield(L, -2, "flags");
	       
		lua_pushstring(L, elf_strptr(l->elf, l->elf_header.e_shstrndx, shdr.sh_name));
		lua_setfield(L, -2, "name");

		lua_pushnumber(L, shdr.sh_offset);
		lua_setfield(L, -2, "sh_offset");

		lua_pushnumber(L, shdr.sh_addr);
		lua_setfield(L, -2, "sh_addr");

		lua_pushnumber(L, shdr.sh_size);
		lua_setfield(L, -2, "sh_size");
	
		lua_rawseti(L, -2, i++);
		
	}
	return 1;
}

void l_elf_register(lua_State *L)
{
	lua_pushcfunction(L, l_elf_open);
	lua_setglobal(L, "edcl_elf_open");
	lua_pushcfunction(L, l_elf_getsections);
	lua_setglobal(L, "edcl_elf_getsections");
}
