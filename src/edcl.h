#ifndef __EDCL_H__
#define __EDCL_H__

#include <stdlib.h>
#include "stdbool.h"


enum CHIP_ENDIANNESS
{
	ENDIAN_LITTLE=0,
	ENDIAN_BIG,
};

struct edcl_chip_config {
	char* name;
	char* board_addr;
	char* host_addr;
	int maxpayload;
	int local_port;
	int remote_port;
	char local_mac[6];
	char remote_mac[6];
	enum CHIP_ENDIANNESS endian;
	bool need_swap
};

struct EdclPacket {
	unsigned short offset;
	unsigned int control;
	unsigned int address;
} __attribute__((packed));


#define min(a,b) ((a) > (b) ? (b) : (a))

#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN 6
#endif

/**
 *
 * Set the communications profile to use
 * Call this before edcl_init
 *
 * @param profile Profile from edcl_platforms enum
 *
 * @return 0 on success, -1 on failure (no such profile)
 */
int edcl_set_profile(char* name);


/**
 * Get the maximum payload for edcl communications
 * This is chip specific
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
bool edcl_get_host_endianness();
void edcl_set_swap_need_flag(struct edcl_chip_config* chip_conf);
bool edcl_test_init(const char* ifname, struct edcl_chip_config* chip_conf);
const char* edcl_init(const char* iname);
int edcl_read(unsigned int address, void* buffer, size_t len);
int edcl_write(unsigned int address, const void* buffer, size_t len);


/* Global struct with supported profiles */
extern struct edcl_chip_config g_edcl_chips[];

/* EDCL Platform functions */
size_t edcl_platform_get_maxpacket();
void edcl_platform_list_interfaces();
#ifndef EDCL_WINDOWS
int edcl_platform_ask_interfaces(char *iface_name, size_t iface_name_max_len);
#endif
int edcl_platform_init(const char* name, struct edcl_chip_config *chip);
int edcl_platform_send(const void* data, size_t len);
int edcl_platform_recv(void* data, size_t len);


#endif
