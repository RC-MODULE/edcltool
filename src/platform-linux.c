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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <ifaddrs.h>
#include <sys/types.h>


static int LOCAL_IP;
static int REMOTE_IP;
static int sock;


struct edcl_chip_config *chip_config;

struct Hdr {
	struct ethhdr eth;
	struct ip ip;
	struct udphdr udp;
}	__attribute__((packed));


size_t edcl_platform_get_maxpacket()
{
	return ETH_FRAME_LEN - sizeof(struct Hdr);
}

void edcl_platform_list_interfaces()
{
	struct ifaddrs *ifaddr, *ifa;
	int family, s;

	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	/* Walk through linked list, maintaining head pointer so we
	   can free list later */

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		/* Display interface name and family (including symbolic
		   form of the latter for the common families) */


		/* Display interface name and family (including symbolic
		   form of the latter for the common families) */

		if (family == AF_PACKET) {
			printf("%s\n", ifa->ifa_name);
			printf("\n");
		}
	}

	freeifaddrs(ifaddr);
	exit(EXIT_SUCCESS);
}

int edcl_platform_init(const char* name, struct edcl_chip_config *chip)
{
	int i;

	chip_config = chip;
	char* board = chip->board_addr;
	char* self = chip->host_addr;

	LOCAL_IP = inet_addr(self);
	REMOTE_IP = inet_addr(board);


	sock = socket(PF_PACKET, SOCK_RAW, htons(0x800));
	if(sock < 0)
		return -1;

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
	memcpy(chip_config->local_mac, &iface.ifr_hwaddr.sa_data, sizeof(chip_config->local_mac));

	return 0;

}


int edcl_platform_send(const void* data, size_t len) {
	char buf[2000] = {0};
	struct Hdr* hdr = (struct Hdr*)buf;

	memcpy(hdr->eth.h_dest, chip_config->remote_mac, sizeof(chip_config->remote_mac));
	memcpy(hdr->eth.h_source, chip_config->local_mac, sizeof(chip_config->local_mac));
	hdr->eth.h_proto = htons(0x800);

	hdr->ip.ip_v = 4;
	hdr->ip.ip_hl = 5;
	hdr->ip.ip_p = 0x11;
	hdr->ip.ip_src.s_addr = LOCAL_IP;
	hdr->ip.ip_dst.s_addr = REMOTE_IP;
	hdr->ip.ip_ttl = 1;

	hdr->udp.source = chip_config->local_port;
	hdr->udp.dest = chip_config->remote_port;

	memcpy(buf + sizeof(*hdr), data, len);

	if(0 > send(sock, buf, sizeof(*hdr) + len, 0)) return -1;

	return 0;
}

int edcl_platform_recv(void* data, size_t len) {
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

		if(memcmp(chip_config->remote_mac, hdr->eth.h_source, sizeof(hdr->eth.h_source)) == 0
				&& hdr->ip.ip_src.s_addr == REMOTE_IP
				&& hdr->ip.ip_dst.s_addr == LOCAL_IP
				&& hdr->ip.ip_p == 0x11
				&& hdr->udp.dest == chip_config->local_port)
		{
			memcpy(data, buf + sizeof(*hdr), min(n - sizeof(*hdr), len));
			return n - sizeof(*hdr);
		}

		if(time(0) - t1 > 1) return -1;
		errno = ETIMEDOUT;
	}
}
