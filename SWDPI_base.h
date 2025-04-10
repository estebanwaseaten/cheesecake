// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 *
 * Copyright (C) 2025 Daniel Wegkamp
 */

#ifndef SWDPI_base_H
#define SWDPI_base_H

#include <linux/types.h>

// start(1), DP(0) or AP(1), read(1) or write(0), addr1, addr2, parity(1) if last 4 has odd # of 1, stop(0), park(1)
#define SWD_CMD_READ_IDCODE 0xA5            // 0b10100101
//response: [31-28] Version, [27-12] PARTNO (0xBA00 or 0xBA10), [11, 1] DESIGNER (ARM=01000111011), [0]=1

#define SWD_ACK_OK 0x001
#define SWD_ACK_WAIT 0x010
#define SWD_ACK_FAULT 0x100
#define SWD_ACK_PERR 0x111  //protocol error

#define GPIO_PULL_NONE    0
#define GPIO_PULL_UP      1
#define GPIO_PULL_DOWN    2



// ioctl stuff
#define SWD_IOC_MAGIC 's'

struct swd_ioc_transfer
{
    uint64_t		data_buf;         //address of 32 bit transfer data array is read for read commands and
    uint64_t        cmds_buf;         //address of 8bit command array
    uint64_t        ack_buf;          //

    uint32_t        length;

    uint8_t         status;             // driver error and transfer error

    uint32_t       speed_hz;
};

struct gpio_interface
{
	int (*init) ( void );
	int (*configPinPull) (uint8_t, uint32_t);
	int (*setPinOutput) (uint8_t);
	int (*setPinInput) (uint8_t);
	int (*readPin) (uint8_t);
	int (*setPin) (uint8_t);
	int (*unsetPin) (uint8_t);
	int (*cleanup) (void);
};

static const uint8_t swd_sequence_jtag2swd[] =
{
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x9E, 0xE7,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00
};
static const uint32_t swd_sequence_jtag2swd_length = 17; //17 bytes






#endif
