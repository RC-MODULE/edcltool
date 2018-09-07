#include "edcl.h"
#include <stdint.h>
struct edcl_chip_config g_edcl_chips[] = {
        {
                .name = "K1879XB",
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
                .board_addr = "192.168.0.48",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec,			0x17, 0x66, 0x77, 0x05, 0x00 },
                .endian = ENDIAN_LITTLE
        },
        {
                .name = "MM7705MPW",
                .board_addr = "192.168.0.52",
                .host_addr = "192.168.0.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec,			0x17, 0x66, 0x77, 0x05, 0x04 },
                .endian = ENDIAN_LITTLE
        },
        /* 100 Mbit # 0*/
        {
                .name = "1888ТХ018",
                .board_addr = "192.168.1.2",
                .host_addr = "192.168.1.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0x00, 0x00, 0x02 },
                .endian = ENDIAN_LITTLE
        },
        /* 100 Mbit # 1*/
        {
                .name = "1888ТХ018",
                .board_addr = "192.168.1.3",
                .host_addr = "192.168.1.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0x00, 0x00, 0x03 },
                .endian = ENDIAN_LITTLE
        },
        /* 100 Mbit # 1*/
        {
                .name = "1888ТХ018",
                .board_addr = "192.168.1.0",
                .host_addr = "192.168.1.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0x00, 0x00, 0x00 },
                .endian = ENDIAN_LITTLE
        },
        /* Gbit # 0*/
        {
                .name = "1888ТХ018",
                .board_addr = "192.168.1.49",
                .host_addr = "192.168.1.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0x77, 0x05, 0x01 },
                .endian = ENDIAN_LITTLE
        },

        {
                .name = "1888ТХ018",
                .board_addr = "192.168.1.48",
                .host_addr = "192.168.1.1",
                .maxpayload = 456,
                .local_port = 0x8088,
                .remote_port = 0x9099,
                .remote_mac ={ 0xec, 0x17, 0x66, 0x77, 0x05, 0x00 },
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
