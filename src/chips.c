#include "edcl.h"
#include <stdint.h>
struct edcl_chip_config g_edcl_chips[] = {
        {
                .name = "K1879XB",
                .comment = "MB77.07 & others",
                .board_addr = "192.168.0.0",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0,			0,    0x5e, 0,	  0,	0    },
                .endian = ENDIAN_LITTLE                        // 0 - little endian, 1 - big endian
        },
        {
                .name = "MM7705MPW",
                .comment = "eth0",
                .board_addr = "192.168.0.48",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec,			0x17, 0x66, 0x77, 0x04, 0x00 },
                .endian = ENDIAN_BIG
        },
        {
                .name = "MM7705MPW",
                .comment = "eth1",
                .board_addr = "192.168.0.52",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec,			0x17, 0x66, 0x77, 0x04, 0x04 },
                .endian = ENDIAN_BIG
        },
        {
                .name = "1888ТХ018",
                .comment = "Greth 100Mbit #0",
                .board_addr = "192.168.1.2",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0x00, 0x00, 0x02 },
                .endian = ENDIAN_LITTLE
        },
        {
                .name = "1888ТХ018",
                .comment = "Greth 100Mbit #1",
                .board_addr = "192.168.1.3",
                .host_addr = "192.168.1.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0x00, 0x00, 0x03 },
                .endian = ENDIAN_LITTLE
        },
        {
                .name = "1888ТХ018",
                .comment = "Greth 100Mbit #2",
                .board_addr = "192.168.1.0",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0x00, 0x00, 0x00 },
                .endian = ENDIAN_LITTLE
        },
        {
                .name = "1888ТХ018",
                .comment = "Greth 1Gbit #0",
                .board_addr = "192.168.1.49",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0x77, 0x05, 0x01 },
                .endian = ENDIAN_LITTLE
        },
        {
                .name = "1888ТХ018",
                .comment = "Greth 1Gbit #1",
                .board_addr = "192.168.1.48",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0x77, 0x05, 0x00 },
                .endian = ENDIAN_LITTLE
        },
        {
                .name = "1888BM018",
                .comment = "Greth #1",
                .board_addr = "192.168.1.48",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0xe, 0x10, 0x00 },
                .endian = ENDIAN_LITTLE
        },
                {
                .name = "1888BM018",
                .comment = "Greth #2",
                .board_addr = "192.168.1.49",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0xe, 0x10, 0x01 },
                .endian = ENDIAN_LITTLE
        },
        {
                /* sentinel */
        }
};

bool edcl_get_host_endianness()
{
        union {
                uint32_t	i;
                char		c[4];
        } e = { 0x01000000 };

        if (e.c[0]) {
                return 1;
        } else {
                return 0;
        }
}

void edcl_set_swap_need_flag(struct edcl_chip_config *chip_config)
{
        if (chip_config == 0) {
                perror("Wrong memory!");
        }

        if (chip_config->endian == edcl_get_host_endianness()) {
                chip_config->need_swap = false;
        } else {
                chip_config->need_swap = true;
        }
}
