#include "edcl.h"
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>



#ifdef EDCL_WINDOWS
#include <winsock2.h>
#include <windows.h>
#pragma warning( disable : 4996 )
//#include <Iphlpapi.h>
#endif


/* Windows. Oh, crap */
#ifndef EPROTO
#define EPROTO    134
#define GNULIB_defined_EPROTO 1
#endif

#define ARRAY_SIZE(a)                               \
  ((sizeof(a) / sizeof(*(a)))

#define CHIPS_ARRAY_SIZE() ARRAY_SIZE(g_edcl_chips)

#define __swap32(value)                                 \
        ((((uint32_t)((value) & 0x000000FF)) << 24) |   \
         (((uint32_t)((value) & 0x0000FF00)) << 8) |    \
         (((uint32_t)((value) & 0x00FF0000)) >> 8) |    \
         (((uint32_t)((value) & 0xFF000000)) >> 24))


static int seq = -1;
static struct edcl_chip_config *chip_config;
static int LOCAL_IP;
static int REMOTE_IP;
static int initialized;

int edcl_get_max_payload() {
		return chip_config->maxpayload;
}

unsigned int edcl_seq(struct EdclPacket const* p) {
	return ntohl(p->control) >> 18;
}

unsigned int edcl_rwnak(struct EdclPacket const* p) {
	return (ntohl(p->control) >> 17) & 1;
}

unsigned int edcl_len(struct EdclPacket const* p) {
	return (ntohl(p->control) >> 7) & 0x3FF;
}

unsigned int edcl_control(unsigned seq, unsigned flag, unsigned len) {
	return htonl((seq & 0x3FFF) << 18 | ((!!flag) & 1) << 17 | (len & 0x3FF) << 7);
}

int edcl_send(const void* buf, size_t len) {
	return edcl_platform_send(buf, len);
}

int edcl_recv(void* buf, size_t len) {
	return edcl_platform_recv(buf, len);
}

bool edcl_test_init(const char* ifname, struct edcl_chip_config* chip_conf)
{
	bool is_init = false;

	struct EdclPacket rq = {
		.control = edcl_control(0, 0, 0)
	};

	if (0 != edcl_platform_init(ifname, chip_conf))
        return false;

	struct EdclPacket rs;

	int ret;
	ret = edcl_send(&rq, sizeof(rq));
	if (ret) {
		fprintf(stderr, "edcl_init: Failed to send pkt\n");
		return false;
	}

	ret = edcl_recv(&rs, sizeof(rs));
	if (ret && rs.address == rq.address && edcl_len(&rs) == edcl_len(&rq)) {
		seq = edcl_seq(&rs);
		initialized++;
		return true;
	}

	return false;
}

const char* edcl_init(const char* ifname) {
	seq = 0;

  const char *ret = NULL;
	struct edcl_chip_config *i;

	for(i = &g_edcl_chips[0]; i->name; i++) {
		chip_config = i;
		edcl_set_swap_need_flag(chip_config);
		//Check endianness
		if(edcl_test_init(ifname, chip_config)) {
			printf("Detected %s target IP %s (%s)\n", i->name, i->board_addr, i->comment);
      		ret = i->name;
      		break;
    	}
	}
	return ret;
}

int edcl_transaction(struct EdclPacket* rq, size_t rqlen, struct EdclPacket* rs, size_t rslen) {
	int i;
	for(i = 0; i < 5; ++i) {
		if(edcl_send(rq, rqlen))
			return -1;
		ssize_t n = edcl_recv(rs, rslen);
		if(n < 0)
			return -1;

		if(edcl_rwnak(rs)) {
			seq = edcl_seq(rs) + 1;
			continue;
		}

		if(edcl_seq(rs) == edcl_seq(rq))
			return n;
	}

	return 0;
}

static void check_initialized()
{
	if (!initialized) {
		fprintf(stderr, "FATAL: Trying to do something without calling 'edcl_init' first\n");
		fprintf(stderr, "       Looks like a bug in your script\n");
		exit(1);
	}
}

int edcl_read(unsigned int address, void* obuf, size_t len) {
	check_initialized();
	if(len > edcl_platform_get_maxpacket()) {
		errno = EINVAL;
		return -1;
	}

	address = htonl(address);

	struct EdclPacket rq = {.control = edcl_control(seq++, 0, len), .address = address };
	char buf[2000];
	struct EdclPacket* rs = (struct EdclPacket*) buf;
	ssize_t n = edcl_transaction(&rq, sizeof(rq), rs, sizeof(buf));

	if(n < len + sizeof(*rs)) {
		errno = EPROTO;
		return -1;
	}

	if(rs->address != address || edcl_len(rs) != len) {
		errno = EPROTO;
		return -1;
	}

	if(chip_config->need_swap)  {
		uint32_t *srcptr = (uint32_t *) (buf + sizeof(*rs));
		uint32_t *destptr = (uint32_t *) obuf;

  	int num = len / sizeof(uint32_t);
		int i;
		for(i=0; i<num; ++i) {
			*destptr++ = __swap32(*srcptr);
			srcptr++;
		}
	}
	else {
		memcpy(obuf, buf + sizeof(*rs), len);
	}
	return 0;
}

int edcl_write(unsigned int address, const void* ibuf, size_t len) {
	char buf[2000] = {0};

	check_initialized();
	struct EdclPacket* rq = (struct EdclPacket*)buf;
	struct EdclPacket rs;

	if((len > edcl_platform_get_maxpacket()) || len > chip_config->maxpayload) {
		errno = EINVAL;
		fprintf(stderr, "payload too large: %zu \n", len );
		return -1;
	}

	rq->address = htonl(address);
	//What is this?
	rq->control = edcl_control(seq++, 1, len);

	if(chip_config->need_swap) {
    const uint32_t *srcptr = ibuf;
    uint32_t *destptr = (uint32_t *) (buf + sizeof(*rq));

  	int num = len / sizeof(uint32_t);
  	//If number isn't a whole number, we will get the problems!
  	//Exeption!
  	if(len % sizeof(uint32_t) != 0)
  		perror("Data isn't correct!");

  	int i;
  	for(i=0; i<num; ++i) {
  		*destptr++ = __swap32(*srcptr);
			srcptr++;
  	}
	}
	else {
		memcpy(buf + sizeof(*rq), ibuf, len);
	}
	return edcl_transaction(rq, sizeof(*rq) + len, &rs, sizeof(rs)) > 0 ? 0 : -1;
}
