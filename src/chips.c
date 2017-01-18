#include "edcl.h"

struct edcl_chip_config g_edcl_chips[] = {
	{
		.name = "K1879XB",
		.board_addr = "192.168.0.0",
		.host_addr = "192.168.0.1",
		.maxpayload = 456,
		.local_port = 0x8088,
		.remote_port = 0x9099,
		.remote_mac =  {0, 0, 0x5e, 0, 0, 0}
	},
	{
		.name = "MM7705MPW-ETH0",
    .board_addr = "192.168.0.48",
    .host_addr = "192.168.0.1",
    .maxpayload = 456,
    .local_port = 0x8088,
    .remote_port = 0x9099,
		.remote_mac =  {0xec, 0x17, 0x66, 0x77, 0x05, 0x00}
	},
	{
		.name = "MM7705MPW-ETH1",
		.board_addr = "192.168.0.52",
		.host_addr = "192.168.0.1",
		.maxpayload = 456,
		.local_port = 0x8088,
		.remote_port = 0x9099,
		.remote_mac =  {0xec, 0x17, 0x66, 0x77, 0x05, 0x04}
	},
	{
		/* sentinel */
	}
};
