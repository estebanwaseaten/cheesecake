// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 *
 * Copyright (C) 2025 Daniel Wegkamp
 */

#ifndef SWDPI_base_H
#define SWDPI_base_H

#include <linux/types.h>

#define SWDPI_ERR_GENERIC           -1
#define SWDPI_ERR_RECBUF_NOTEMPTY   -2
#define SWDPI_ERR_WRONG_RW_SIZE     -3

static const uint8_t swd_sequence_jtag2swd[] =
{
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x9E, 0xE7,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00
};
static const uint32_t swd_sequence_jtag2swd_length = 17; //17 bytes


static const uint8_t swd_sequence_null[] =
{
	0x00,
};
static const uint32_t swd_sequence_null_length = 1; //17 bytes



#endif
