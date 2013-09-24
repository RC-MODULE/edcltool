#ifndef __EDCL_H__
#define __EDCL_H__

#include <stdlib.h>

#define EDCL_DEFAULT_BOARD_ADDRESS  "192.168.0.0"
#define EDCL_DEFAULT_HOST_ADDRESS   "192.168.0.1"



enum edcl_platforms {
	K1879XB=0,
	/* More to come */
};



/** 
 * 
 * Set the communications profile to use
 * Call this before edcl_init
 * 
 * @param profile Profile from edcl_platforms enum
 * 
 * @return 0 on success, -1 on failure (no such profile)
 */
int edcl_set_profile(enum edcl_platforms profile);

/** 
 * Get the maximum payload for edcl communications 
 * This is platform specific
 * 
 * @return Maximum size of payload for read/write
 */
int edcl_get_max_payload();


/** 
 * 
 * Initialise edcl communications
 * 
 * @param iname interface name, e.g. eth0
 * 
 * @return 
 */

int edcl_init(const char* iname);
int edcl_read(unsigned int address, void* buffer, size_t len);
int edcl_write(unsigned int address, const void* buffer, size_t len);

#endif
