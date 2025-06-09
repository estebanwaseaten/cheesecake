// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cheese_devices.h"
#include "cheese_devices_identification_and_regs.h"
#include "cheese_comSWD.h"


int currentDeviceIndex = -1;
int currentSequenceDeviceIndex = -1;


//DEVICE SEQUENCES:
struct device_sequences
{
    uint32_t    partno;
    uint32_t    sequences[7][65];
};
//select first access port and do not increase addresses automatically upon read/write
#define SEQ_AP0_noInc DP_IDCODE_CMD, 0x0, DP_CTRLSTAT_R_CMD, 0x0, DP_ABORT_CMD, 0x1E, DP_CTRLSTAT_W_CMD, 0x50000000, DP_SELECT_CMD, 0x0, AP_WRITE0_CMD, 0x23000002
#include "stm_registers.h"
struct device_sequences stm_sequences[] = {
    #include "stm_device_sequences.inc"
};
const uint32_t devices_entry_count = sizeof(stm_sequences) / sizeof( stm_sequences[0] );//9;


void setCurrentDeviceIndices( void );
int executeSequence( int deviceIndex, int sequenceIndex );

int initDevice()
{
    setCurrentDeviceIndices();
    if( (currentDeviceIndex == -1) || (currentSequenceDeviceIndex == -1))
    {
        printf( "could not find device!\n");
        return -1;
    }
    else
    {
        printf( "found partno: 0x%X at index %d: %s\n", stm_info[currentDeviceIndex].partno, currentDeviceIndex, stm_info[currentDeviceIndex].description );
    }

    return 0;
}

int currentDeviceExecuteSequence( int sequenceIndex )
{
    if(currentSequenceDeviceIndex == -1)
    {
        printf( "device not available!\n");
        return -1;
    }

    return executeSequence( currentSequenceDeviceIndex, sequenceIndex );
}

int currentDeviceWriteDeviceRegister( uint32_t reg, uint32_t value )
{
    uint32_t regAddr = stm_info[currentDeviceIndex].devRegs[reg];
    com_writeWord( regAddr, value );
}

uint32_t currentDeviceReadDeviceRegister( uint32_t reg )
{
    uint32_t regAddr = stm_info[currentDeviceIndex].devRegs[reg];
    return com_readWord( regAddr );
}

int currentDeviceWriteCoreRegister( uint32_t reg, uint32_t value )
{
    //write value to DCRDR (Debug Core Register Data Resgister)
    com_writeWord( stm_info[currentDeviceIndex].devRegs[DEV_REG_DCRDR], value);

    //write to DCRSR (Debug Core Register Selector Register)
    uint32_t coreRegisterSelectValue = 0 | (1 << 16) | reg;
    com_writeWord( stm_info[currentDeviceIndex].devRegs[DEV_REG_DCRSR], coreRegisterSelectValue);

    // Polls DHCSR until DHCSR.S_REGRDY reads-as-one
    uint32_t status = 0;
    do
    {   //check if transfer out of DEV_REG_DCRDR into core reg is complete
        status = com_readWord( stm_info[currentDeviceIndex].devRegs[DEV_REG_DHCSR] );
    } while( !(status & (1 << 16)) );
}

uint32_t currentDeviceReadCoreRegister( uint32_t reg )
{
    //write to DCRSR (Debug Core Register Selector Register)
    uint32_t coreRegisterSelectValue = 0 | (0 << 16) | reg;
    com_writeWord( stm_info[currentDeviceIndex].devRegs[DEV_REG_DCRSR], coreRegisterSelectValue );

    // poll DHCSR until DHCSR.S_REGRDY reads-as-one
    uint32_t status = 0;
    do
    {   //check if transfer form core reg into DEV_REG_DCRDR is complete
        status = com_readWord( stm_info[currentDeviceIndex].devRegs[DEV_REG_DHCSR] );
    } while( !(status & (1 << 16)) );

    // read DCRDR (Debug Core Register Data Register)
    return com_readWord( stm_info[currentDeviceIndex].devRegs[DEV_REG_DCRDR] );
}




void setCurrentDeviceIndices( void )
{
    int partindex = -1;
    uint32_t partno;

    //loop through entries of stm_partno_registers.inc:
    for (int i = 0; i < stm_info_entry_count; i++)
    {
        if ( (com_readWord( stm_info[i].baseDBG ) & 0xFFF ) == stm_info[i].partno )
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
        if( stm_sequences[i].partno == partno )
        {
            currentSequenceDeviceIndex = i;
        }
    }
}

int executeSequence( int deviceIndex, int sequenceIndex )
{
    if ( deviceIndex >= devices_entry_count )
    {
        printf( "execute Sequence() error: device index does not exist!\n");
        return -1;
    }

    int sequence_length = stm_sequences[deviceIndex].sequences[sequenceIndex][0];       //the unit is "commands" --> 64bits
    uint32_t *buffer = calloc( sequence_length, 8 );    //8bytes (64bits) per command
    memcpy(buffer, stm_sequences[deviceIndex].sequences[sequenceIndex], (sequence_length * 8) + 4);

    //this overwrites contents of sequence
    com_transferSequence( buffer );            //does this overwrite values in the sequence?

    /*for( int i = 0; i < sequence_length * 2; i++ )
    {
        printf( "0x%08X - ", buffer[i+1]);
    }*/

    uint32_t final_return_value = buffer[sequence_length*2];

    free( buffer );

    return final_return_value;
}
