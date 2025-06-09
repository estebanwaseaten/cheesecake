// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/
#include <stdint.h>     //uint32_t etc


#ifndef CHEESE_DEVICES_H
#define CHEESE_DEVICES_H

//sequences might not work well because of return values...

//COMMAND SEQUENCES
enum
{
    SEQ_HALT,
    SEQ_UNHALT,
    SEQ_HALT_ON_RESET,
    SEQ_UNHALT_ON_RESET,
    SEQ_RESET,
    SEQ_UNLOCK_FLASH,
};

//DEVICE SPECIFIC DEVICE REGISTERS
enum
{
    DEV_REG_DFSR,
    DEV_REG_DHCSR,
    DEV_REG_DCRSR,
    DEV_REG_DCRDR,
    DEV_REG_DEMCR,
    DEV_REG_CPUID,
    DEV_REG_AIRCR,
    DEV_REG_VTOR,
    DEV_REG_DBG,
    DEV_REG_FLASH,
    DEV_REG_RAM
};

//PROCESSOR CORE REGISTERS (at least for armv6 and armv7)
enum
{
    CORE_REG_R0 = 0,
    CORE_REG_R1,
    CORE_REG_R2,
    CORE_REG_R3,
    CORE_REG_R4,
    CORE_REG_R5,
    CORE_REG_R6,
    CORE_REG_R7,
    CORE_REG_R8,
    CORE_REG_R9,
    CORE_REG_R10,
    CORE_REG_R11,
    CORE_REG_R12,
    CORE_REG_SP = 0xD,    //13, current stack pointer but according to armv6
    CORE_REG_LR,
    CORE_REG_DBG_RET,   //DebugReturnAddress is the address of the first instruction to be executed on exit from Debug state. This seems to be the Program Counter?
    CORE_REG_XPSR,      //The special-purpose Program Status Registers
    CORE_REG_MSP,       //main stack pointer - banked version of R13
    CORE_REG_PSP,       //process stack pointer - banked version of R13
    CORE_REG_PC,    //15

};

int initDevice();
int currentDeviceExecuteSequence( int sequenceIndex );

int currentDeviceWriteDeviceRegister( uint32_t reg, uint32_t value );
uint32_t currentDeviceReadDeviceRegister( uint32_t reg );
int currentDeviceWriteCoreRegister( uint32_t reg, uint32_t value );
uint32_t currentDeviceReadCoreRegister( uint32_t reg );













#endif
