// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 *
 * Copyright (C) 2025 Daniel Wegkamp
 */

#ifndef SWDPI_H
#define SWDPI_H

#include <linux/types.h>
#include <linux/ioctl.h>

// start(1), DP(0) or AP(1), read(1) or write(0), addr1, addr2, parity(1) if last 4 has odd # of 1, stop(0), park(not driven-0)
#define SWD_CMD_READ_IDCODE 0xA4            // 0b10100100


#define SWD_IOC_MAGIC 's'

struct swd_ioc_transfer
{
    uint64_t		data_buf;         //address of 32 bit transfer data array
    uint64_t        cmds_buf;       //address of 8bit command array

    uint32_t        length;

    uint8_t         status;             // driver error and transfer error

    uint32_t       speed_hz;
};


#define SWD_IOC_TRANSFER __IOWR( SWD_IOC_MAGIC, 0, char[sizeof(struct swd_ioc_transfer)])

#define SWD_IOC_RD_SPEED_HZ		_IOR(SWD_IOC_MAGIC, 4, __u32)
#define SWD_IOC_WR_SPEED_HZ		_IOW(SWD_IOC_MAGIC, 4, __u32)



#endif
