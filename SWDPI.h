// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  userspace header
 *
 * Copyright (C) 2025 Daniel Wegkamp
 */

#ifndef SWDPI_H
#define SWDPI_H

#include <linux/ioctl.h>        //defines _IO/_IOR/_IOW/_IOWR --> 8bit type, 8bit cmd nr, sizeof(data) 13/14bit (max 8191 bytes).

// ioctl    https://www.kernel.org/doc/html/latest/driver-api/ioctl.html
#define SWD_IOC_TRANSFER        __IOWR( SWD_IOC_MAGIC, 0, char[sizeof(struct swd_ioc_transfer)])
#define SWD_IOC_RD_SPEED_HZ		_IOR(SWD_IOC_MAGIC, 4, __u32)
#define SWD_IOC_WR_SPEED_HZ		_IOW(SWD_IOC_MAGIC, 4, __u32)

//commands for write:




//read format






















#endif
