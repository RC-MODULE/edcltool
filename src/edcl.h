#ifndef __EDCL_H__
#define __EDCL_H__

#include <stdlib.h>

#define EDCL_DEFAULT_BOARD_ADDRESS  "192.168.0.0"
#define EDCL_DEFAULT_HOST_ADDRESS   "192.168.0.1"

int edcl_init(const char* iname, char* board, char* self);
int edcl_read(unsigned int address, void* buffer, size_t len);
int edcl_write(unsigned int address, const void* buffer, size_t len);

#endif
