#include "edcl.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define min(a,b) ((a) > (b) ? (b) : (a))

static int seq = -1;
static int sock;

static const int LOCAL_PORT = 0x8088;
static const int REMOTE_PORT = 0x9099;

static int LOCAL_IP;
static int REMOTE_IP; 

static char LOCAL_MAC[6];
static char REMOTE_MAC[] = {0,0,0x5e, 0,0,0};

struct Hdr {
	struct ethhdr eth;
	struct ip ip;
	struct udphdr udp;
}	__attribute__((packed));

struct EdclPacket {
	unsigned short offset;
	unsigned int control;
	unsigned int address;
} __attribute__((packed));

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

int edcl_send(const void* buf, size_t len);
int edcl_recv(void* buf, size_t len);

int edcl_init(const char* name, char* board, char* self) {
	int i;

	LOCAL_IP = inet_addr(self);
	REMOTE_IP = inet_addr(board);

	seq = 0;

	sock = socket(PF_PACKET, SOCK_RAW, htons(0x800));
	if(sock < 0) return -1;
	
	struct ifreq iface;
	strncpy(iface.ifr_name, name, sizeof(iface.ifr_name));
	if(ioctl(sock,SIOCGIFINDEX,&iface)) return -1;

	struct sockaddr_ll address = {
		AF_PACKET,
		htons(0x800),
		iface.ifr_ifindex,
		1,
		PACKET_HOST
	};

	if(bind(sock, (struct sockaddr*)&address, sizeof(address))) return -1;

	if(ioctl(sock,SIOCGIFHWADDR, &iface)) return -1;
	memcpy(LOCAL_MAC, &iface.ifr_hwaddr.sa_data, sizeof(LOCAL_MAC));
	
	struct EdclPacket rq = { .control = edcl_control(0, 0, 0) };

	for(i = 0; i < 3; ++i) {
		if(edcl_send(&rq, sizeof(rq)))
			return -1; 

		struct EdclPacket rs;
		if(edcl_recv(&rs, sizeof(rs)) && rs.address == rq.address && edcl_len(&rs) == edcl_len(&rq)) {
			seq = edcl_seq(&rs);
			return 0;
		}
	}

	return -1;
}

int edcl_send(const void* data, size_t len) {
	char buf[2000] = {0};
	struct Hdr* hdr = (struct Hdr*)buf;

	memcpy(hdr->eth.h_dest, REMOTE_MAC, sizeof(REMOTE_MAC));
	memcpy(hdr->eth.h_source, LOCAL_MAC, sizeof(LOCAL_MAC));
	hdr->eth.h_proto = htons(0x800);
	
	hdr->ip.ip_v = 4;
	hdr->ip.ip_hl = 5;
	hdr->ip.ip_p = 0x11;
	hdr->ip.ip_src.s_addr = LOCAL_IP;
	hdr->ip.ip_dst.s_addr = REMOTE_IP;
	hdr->ip.ip_ttl = 1;
	
	hdr->udp.source = LOCAL_PORT;
	hdr->udp.dest = REMOTE_PORT;

	memcpy(buf + sizeof(*hdr), data, len);
	
	if(0 > send(sock, buf, sizeof(*hdr) + len, 0)) return -1;
	
	return 0;
}

int edcl_recv(void* data, size_t len) {
	char buf[2000];
	struct Hdr* hdr = (struct Hdr*)buf;
	time_t t1 = time(0);

	for(;;) {
		fd_set rdset;
		struct timeval timeout = {.tv_sec = 1};

		FD_ZERO(&rdset);
		FD_SET(sock, &rdset);

		if(select(sock+1, &rdset, 0, 0, &timeout) <= 0) {
			errno = ETIMEDOUT;
			return -1;
		}

		ssize_t n = recv(sock, buf, sizeof(buf), 0);

		if(n < sizeof(struct Hdr)) return -1;

		if(memcmp(REMOTE_MAC, hdr->eth.h_source, sizeof(hdr->eth.h_source)) == 0
				&& hdr->ip.ip_src.s_addr == REMOTE_IP 
				&& hdr->ip.ip_dst.s_addr == LOCAL_IP
				&& hdr->ip.ip_p == 0x11
				&& hdr->udp.dest == LOCAL_PORT) 
		{
			memcpy(data, buf + sizeof(*hdr), min(n - sizeof(*hdr), len));
			return n - sizeof(*hdr);
		}

		if(time(0) - t1 > 1) return -1;
		errno = ETIMEDOUT;
	}
}

int edcl_transaction(struct EdclPacket* rq, size_t rqlen, struct EdclPacket* rs, size_t rslen) {
	int i;
	for(i = 0; i < 3; ++i) {
		if(edcl_send(rq, rqlen)) return -1;
		ssize_t n = edcl_recv(rs, rslen);
		if(n < 0) return -1;

		if(edcl_rwnak(rs)) {
			seq = edcl_seq(rs) + 1;
			continue;
		}

		if(edcl_seq(rs) == edcl_seq(rq))
			return n;
	}

	return 0;
}

int edcl_read(unsigned int address, void* obuf, size_t len) {
	if(len > ETH_FRAME_LEN - sizeof(struct Hdr)) {
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
	struct EdclPacket* rq = (struct EdclPacket*)buf;
	struct EdclPacket rs;

	if(len > ETH_FRAME_LEN - sizeof(struct Hdr)) {
		errno = EINVAL;
		return -1;
	}


	rq->address = htonl(address);
	rq->control = edcl_control(seq++, 1, len);

	memcpy(buf + sizeof(*rq), ibuf, len);
	
	return edcl_transaction(rq, sizeof(*rq) + len, &rs, sizeof(rs)) > 0 ? 0 : -1;
}

