// SPDX-License-Identifier: GPL-2.0-or-later
/*
*  partially from cd00167594-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics-1.pdf
*
* Copyright (C) 2025 Daniel Wegkamp
*/
// Information in here: 1. a way to identify the device quickly
//  2. some base addresses (maybe also not?)

// { partno, dbgbase/DBG_IDCODE, flash, DHCSR/DCRSR/DCRDR/DEMCR description (max 64 chars)}

struct device_info
{
    uint32_t    partno;

    uint32_t    devRegs[8];
//    uint32_t    DFSR;
//    uint32_t    DHCSR;
//    uint32_t    DCRSR;
//    uint32_t    DCRDR;
//    uint32_t    DEMCR;
//    uint32_t    CPUID;
//    uint32_t    AIRCR;
//    uint32_t    VTOR;

    uint32_t    baseDBG;
    uint32_t    baseFLASH;
    uint32_t    baseRAM;

    const char  description[64];
};

struct device_info stm_info[] = {
   // id,     DFSR,       DHCSR,      DCRSR,      DCRDR,      DEMCR,      CPUID,      AIRCR,      VTOR,       baseDBG,    baseFLASH,  baseRAM
   { 0x411, { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, 0x00000000, 0x00000000, 0x00000000, "STM32F2xxxx" },
   { 0x417, { 0xE000ED30, 0xE000EDF0, 0xE000EDF4, 0xE000EDF8, 0xE000EDFC, 0xE000ED00, 0xE000ED0C, 0xE000ED08 }, 0x40015800, 0x40022000, 0xE000EDF0, "STM32L05x?" },
   { 0x423, { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, 0x00000000, 0x00000000, 0x00000000, "STM32F401xB/C" },
   { 0x433, { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, 0x00000000, 0x00000000, 0x00000000, "STM32F401xD/E" },
   { 0x438, { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, 0x00000000, 0x00000000, 0x00000000, "STM32F303x6/8 & STM32F328" },
   { 0x440, { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, 0x00000000, 0x00000000, 0x00000000, "STM32F05xxx/F030x8" },
   { 0x442, { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, 0x00000000, 0x00000000, 0x00000000, "STM32F030xB/C & STM32F358" },
   { 0x446, { 0xE000ED30, 0xE000EDF0, 0xE000EDF4, 0xE000EDF8, 0xE000EDFC, 0xE000ED00, 0xE000ED0C, 0xE000ED08 }, 0xE0042000, 0x40022000, 0xE000EDF0, "STM32F302xD(E)/303xD(E) & STM32F398xE" },
   { 0x447, { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, 0x00000000, 0x00000000, 0x00000000, "STM32L07x??" }
};

const uint32_t stm_info_entry_count = sizeof(stm_info) / sizeof( stm_info[0] );//9;




/*
*  cortex-M0+   -->    ARMv6-M Architecture Reference Manual
*  cortex-M4    -->    ARMv7-M Architectural Reference Manual
*
*
*
*
*
*
*
*/
