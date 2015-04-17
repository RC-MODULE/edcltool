#include "edcl.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#pragma warning( disable : 4996 )
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stddef.h>
#include <iphlpapi.h>
#include <inaddr.h>
#include <pcap/pcap.h>

#pragma pack(push)
#pragma pack(1)


struct S_EthHdr {
	unsigned char  h_dest[6];
	unsigned char  h_source[6];
	unsigned short h_proto;
};

struct S_IP {
	unsigned char ip_hl:4;
	unsigned char ip_v:4;
	unsigned char ip_tos;
	short ip_len;
	unsigned short ip_id;
	short	ip_off;
	unsigned char ip_ttl;
	unsigned char ip_p;
	unsigned short ip_sum;
	struct in_addr ip_src;
	struct in_addr ip_dst;
};

struct S_UDPHdr {
	unsigned short source;
	unsigned short dest;
	unsigned short len;
	unsigned short check;
} ;

struct S_Hdr {
	struct S_EthHdr eth;
	struct S_IP ip;
	struct S_UDPHdr udp;
};


struct S_Packet {
	struct S_Hdr hdr;
	unsigned char data[1];
};


static int LOCAL_IP;
static int REMOTE_IP; 
static int sock;
static pcap_t *ppcap;
static unsigned int board_ip_addr;
static unsigned int host_ip_addr;
static struct S_Packet *psendpacket;


#define DEBUG


#ifdef DEBUG
#define dbg(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif


struct edcl_chip_config *chip_config;


/* FixMe: Use some windows definition here */
#define ETH_FRAME_LEN 1518
size_t edcl_platform_get_maxpacket() 
{
	return ETH_FRAME_LEN - sizeof(struct S_Hdr);
}


void edcl_platform_list_interfaces() 
{
	pcap_if_t *alldevs;
	pcap_t *res = NULL;
	char errbuf[PCAP_ERRBUF_SIZE + 1];

	if(pcap_findalldevs(&alldevs, errbuf) == -1)
		return;
	int i=0;
	for(pcap_if_t *d=alldevs; d && !res; d=d->next) {
		printf("%d). %s\n    %s\n\n", i++, d->name, d->description);
	}
	printf("ProTip(tm): If you do not see your interface here - make sure it is configured\n");
	printf("            and has some valid IP address set.\n");
	
	pcap_freealldevs(alldevs);
}

/**
 * int edcl_platform_ask_interfaces(char *iface_name, size_t iface_name_max_len)
 *
 * @retrun 0 - interface name received. 1 - interface name is not received. -1 - an error has occurred.
 */
int edcl_platform_ask_interfaces(char *iface_name, size_t iface_name_max_len)
{
	pcap_if_t *alldevs;
	pcap_t *res = NULL;
	char errbuf[PCAP_ERRBUF_SIZE + 1];
	unsigned iface_n;
	unsigned exit_item_n;

	if(pcap_findalldevs(&alldevs, errbuf) == -1)
		return;
	int i=0;
	for(pcap_if_t *d=alldevs; d && !res; d=d->next) {
		printf("%d). %s\n    %s\n\n", i++, d->name, d->description);
	}
	pcap_freealldevs(alldevs);
	exit_item_n = i;
	printf("%d). exit\n\n", exit_item_n);

	while(1) {
		printf("Enter interface number > ");

		if(fgets(iface_name, iface_name_max_len, stdin) == NULL)
			return -1;

		if((sscanf(iface_name, "%ud\r", &iface_n) != 1) || (iface_n > exit_item_n)) {
			printf("Invalid interface number. Please try again.\n");
			continue;
		}

		if(iface_n == exit_item_n)
			return 1;

		break;
	}

	return 0;
}

static pcap_t *select_interface_by_id(int id) 
{
	pcap_if_t *alldevs;
	pcap_if_t *dev;
	pcap_t *res = NULL;
	char errbuf[PCAP_ERRBUF_SIZE + 1];
	int i;

	if(pcap_findalldevs_ex("rpcap://", NULL, &alldevs, errbuf) == -1) { /* TODO: "rpcap://" -> PCAP_SRC_IF_STRING */
		fprintf(stderr,"Error in pcap_findalldevs_ex: %s\n", errbuf);
		return NULL;
	}

	for(dev=alldevs, i=0; dev != NULL; dev=dev->next, i++ ) {
		if(i == id) {
			res = pcap_open(dev->name,
					2048,	/* TODO: ? */
					1 | 16,	/* TODO: 1 | 16 -> PCAP_OPENFLAG_PROMISCUOUS | PCAP_OPENFLAG_MAX_RESPONSIVENESS */
					1000,
					NULL,
					errbuf);

			if(!res) {
				fprintf(stderr,"Unable to open the adapter.\n");
				return NULL;
			}

			pcap_setmintocopy(res, 0);

			break;
		}
	}

	if(dev == 0) {
		printf("No interfaces found!\n");
		return NULL;
	}

	pcap_freealldevs(alldevs);

	return res;
}

int edcl_platform_init(const char* name, struct edcl_chip_config *chip)
{
	if (ppcap) 
		return;
	
	memset(chip->local_mac, 0, MAC_ADDR_LEN);

	char mac[] = { 0x14, 0xda, 0xe9, 0x5e, 0x9e, 0x9f };

	memcpy(chip->local_mac, mac, 6);;
	psendpacket = (struct S_Packet*) malloc(sizeof(struct S_Packet) -1 + sizeof(struct EdclPacket) + chip->maxpayload);
	if(!psendpacket) {
		fprintf(stderr, "Memory allocation error\n");
		goto err;
	}

	memset(psendpacket, 0, sizeof(struct S_Packet) - 1);
	memcpy(psendpacket->hdr.eth.h_dest, chip->remote_mac, MAC_ADDR_LEN);
	psendpacket->hdr.eth.h_proto = htons(0x800);
	psendpacket->hdr.ip.ip_v     = 4;
	psendpacket->hdr.ip.ip_hl    = 5;
	psendpacket->hdr.ip.ip_p     = 0x11;
	psendpacket->hdr.ip.ip_ttl   = 1;

	psendpacket->hdr.ip.ip_src.s_addr = 0x0100a8c0;
	psendpacket->hdr.ip.ip_dst.s_addr = 0x0000a8c0;

	psendpacket->hdr.udp.source  = chip->local_port;
	psendpacket->hdr.udp.dest    = chip->remote_port;

	ppcap = select_interface_by_id(atoi(name));	

	if (!ppcap) {
		fprintf(stderr, "Couldn't open iface\n");
		goto errfree;
	}
	
	struct bpf_program fcode;
	char filter[128];
	sprintf(filter, "ether src %02X:%02X:%02X:%02X:%02X:%02X and ip proto 17 and udp[2]=%u and udp[3]=%u",
		chip->remote_mac[0], 
		chip->remote_mac[1], 
		chip->remote_mac[2], 
		chip->remote_mac[3], 
		chip->remote_mac[4],
		chip->remote_mac[5],  
		chip->local_port & 0xFF, (chip->local_port >> 8) & 0xFF);
	if (pcap_compile(ppcap, &fcode, filter, 1, 0xffffff) < 0) {
		fprintf(stderr, "Failed to compile filter\n");
		goto errfree;
	}

	if (pcap_setfilter(ppcap, &fcode) < 0) {
		fprintf(stderr, "Failed to set filter\n");
		goto errfree;
	}

	memcpy(psendpacket->hdr.eth.h_source, chip->local_mac, MAC_ADDR_LEN);

	return 0;
errfree:
//	if (ppcap)
//		ppcap_close(ppcap);
	free(psendpacket);
err: 
	
	return -1;
}


int edcl_platform_send(const void* data, size_t len) 
{
	memcpy(psendpacket->data, data, len);
	return pcap_sendpacket(ppcap, (const u_char *) psendpacket, len + sizeof(struct S_Packet) - 1);
}


int edcl_platform_recv(void* data, size_t len)
{
	struct pcap_pkthdr *header;
	const unsigned char *pkt_data;
	const struct S_Packet *packet;
	size_t rlen;
	if (pcap_next_ex(ppcap, &header, &pkt_data) <= 0) {
		fprintf(stderr, "edcl_windows: didn't receive a packet\n");
		return -1;
	}
	rlen = min((header->len - sizeof(struct S_Hdr)), len);   
	packet = (const struct S_Packet*) pkt_data;
	memcpy(data, pkt_data + sizeof(struct S_Hdr), rlen);
     
	return rlen;
}

