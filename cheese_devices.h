// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/
#include <stdint.h>     //uint32_t etc


#ifndef CHEESE_DEVICES_H
#define CHEESE_DEVICES_H

struct mcuInfo
{
    uint32_t    partno;
    uint32_t    baseDBG;
    uint32_t    baseFLASH;
    uint32_t    baseRAM;

    const char  description[64];
};

enum
{
    SEQ_HALT_ON_RESET,
    SEQ_UNHALT_ON_RESET,
    SEQ_RESET,
};

struct device
{
    uint32_t    partno;

    uint32_t    haltOnReset[65];    //[0]=number of elements; [odd]=cmd, [even]=data
    uint32_t    unhaltOnReset[65];
    uint32_t    reset[65];
    uint32_t    unlockFlash[65];

    const char  description[64];
};

extern int currentDeviceIndex;
extern int currentSequenceDeviceIndex;

void setCurrentDeviceIndices( void );














#endif
