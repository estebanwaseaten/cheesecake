// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <stdio.h>

#include "cheese_devices.h"
#include "cheese_comSWD.h"

int currentDeviceIndex = -1;
int currentSequenceDeviceIndex = -1;

struct mcuInfo stm_info[] = {
#include "stm_device_identification.inc"
};
const uint32_t stm_info_entry_count = sizeof(stm_info) / sizeof( stm_info[0] );//9;

//select first access port and do not increase addresses automatically upon read/write
#define SEQ_AP0_noInc DP_IDCODE_CMD, 0x0, DP_CTRLSTAT_R_CMD, 0x0, DP_ABORT_CMD, 0x1E, DP_CTRLSTAT_W_CMD, 0x50000000, DP_SELECT_CMD, 0x0, AP_WRITE0_CMD, 0x23000002
#include "stm_device_registers.h"
struct device devices_sequences[] = {
    #include "stm_device_sequences.inc"
};
const uint32_t devices_entry_count = sizeof(devices_sequences) / sizeof( devices_sequences[0] );//9;

void setCurrentDeviceIndices( void )
{
    int partindex = -1;
    uint32_t partno;

    //loop through entries of stm_partno_registers.inc:
    for (int i = 0; i < stm_info_entry_count; i++)
    {
        if ( (comArray_readWord( stm_info[i].baseDBG ) & 0xFFF ) == stm_info[i].partno )
        {
            currentDeviceIndex = i;
            partno = stm_info[i].partno;
            //uint32_t debugBaseAddr = stm_info[i].baseDBG;
            //uint32_t flashBaseAddr = stm_info[i].baseFLASH;
            //printf( "found partno: 0x%X at index %d: %s\n", partno, i, stm_info[i].description );
            //printf( "debug base: 0x%08X\nflash base: 0x%08X\nSRAM base: ", debugBaseAddr, flashBaseAddr, ramBaseAddr );
        }
        //printf( "not found partno: 0x%X at index %d; 0x%X\n", stm_info[i].partno, i, stm_info[i].baseDBG );
    }


    for (int i = 0; i < devices_entry_count; i++ )
    {
        if( devices_sequences[i].partno == partno )
        {
            currentSequenceDeviceIndex = i;
        }
    }
}

void executeSequence( int deviceIndex, int sequenceIndex )
{
    if ( deviceIndex >= devices_entry_count )
    {
        printf( "execute Sequence() error: device index does not exist!\n");
        return;
    }
    switch( sequenceIndex )
    {
        case SEQ_HALT_ON_RESET:
            com_transferSequence( devices_sequences[deviceIndex].haltOnReset );
            break;
        case SEQ_UNHALT_ON_RESET:
            com_transferSequence( devices_sequences[deviceIndex].unhaltOnReset );
            break;
        case SEQ_RESET:
            com_transferSequence( devices_sequences[deviceIndex].reset );
            break;
        default:
            break;
    }
}
