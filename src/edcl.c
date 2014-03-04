#include "edcl.h"
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>



#ifdef EDCL_WINDOWS
#include <winsock2.h>
#pragma warning( disable : 4996 )
#include <windows.h>
//#include <Iphlpapi.h>
#endif


/* Windows. Oh, crap */
#ifndef EPROTO
#define EPROTO    134
#define GNULIB_defined_EPROTO 1
#endif

static int seq = -1;
static struct edcl_chip_config *chip_config = &g_edcl_chips[0]; 
static int LOCAL_IP;
static int REMOTE_IP; 
static int initialized;


int edcl_set_profile(char* name) {
	struct chip_config *conf = &g_edcl_chips[0];
	
}

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


int edcl_init(const char* ifname) {
	int i;

	seq = 0;

	edcl_platform_init(ifname, chip_config);

	struct EdclPacket rq = { .control = edcl_control(0, 0, 0) };

	for(i = 0; i < 3; ++i) {
		if(edcl_send(&rq, sizeof(rq))) {
			fprintf(stderr, "edcl_init: Failed to send pkt\n");
			return -1; 
		}
		struct EdclPacket rs;
		if(edcl_recv(&rs, sizeof(rs)) && rs.address == rq.address && edcl_len(&rs) == edcl_len(&rq)) {
			seq = edcl_seq(&rs);
			initialized++;
			return 0;
		}
	}
	fprintf(stderr, "edcl_init: Board not responding (Is it powered on?)\n");
	return -1;
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

int edcl_read(unsigned int address, void* obuf, size_t len) 
{
	check_initialized();
	if(len > edcl_platform_get_maxpacket()) {
		errno = EINVAL;
		return -1;
	}

	address = htonl(address);

	struct EdclPacket rq = {.control = edcl_control(seq++, 0, len), .address = address };
	char buf[2000];
	struct EdclPacket* rs = (struct EdclPacket*)buf;
	ssize_t n = edcl_transaction(&rq, sizeof(rq), rs, sizeof(buf));
	
	if(n < len + sizeof(*rs)) {
		errno = EPROTO;
		return -1;
	}

	if(rs->address != address || edcl_len(rs) != len) {
		errno = EPROTO;
		return -1;
	}

	memcpy(obuf, buf + sizeof(*rs), len);
	return 0;
}

int edcl_write(unsigned int address, const void* ibuf, size_t len) {
	char buf[2000] = {0};
	check_initialized();
	struct EdclPacket* rq = (struct EdclPacket*)buf;
	struct EdclPacket rs;

	if((len > edcl_platform_get_maxpacket()) || len > chip_config->maxpayload) {
		errno = EINVAL;
		fprintf(stderr, "payload too large: %zu \n",
			len );
		return -1;
	}


	rq->address = htonl(address);
	rq->control = edcl_control(seq++, 1, len);

	memcpy(buf + sizeof(*rq), ibuf, len);
	return edcl_transaction(rq, sizeof(*rq) + len, &rs, sizeof(rs)) > 0 ? 0 : -1;
}

