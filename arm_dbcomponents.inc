// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 *
 * Copyright (C) 2025 Daniel Wegkamp
 */

// based on PIDR0-2 and CIDR1 an ARM debug component can be identified. These 4 8bit values are stored in a single uint32_t for easy comparison.
// this table lists all of these 32bit IDs together with a component class (see arm_compclass.inc) and a descriptive string

// ID = 0x PIDR0 PIDR1 PIDR2 CIDR1
{ 0x00000000, 0x00, "unknown" },
{ 0xC0B40B10, 0x01, "generic Cortex-M0+ ROM table" },       // 0x4C0
{ 0x08B00BE0, 0x02, "Cortex-M0+ SCS" },             // System Control Space 0x8
{ 0x0AB00BE0, 0x03, "Cortex-M0+ DWT" },             // Data Watchpoint Unit 0xA
{ 0x0BB00BE0, 0x04, "Cortex-M0+ BPU" },             // BreakPoint Unit 0xB
{ 0xC4B40B10, 0x01, "generic Cortex-M4 ROM table" },
{ 0x00000010, 0x01, "generic Cortex-M3 ROM table" },       //very weird entry, maybe because it was the first cortex?
{ 0x0CB00BE0, 0x02, "Cortex-M3/M4 SCS" },
{ 0x02B03BE0, 0x03, "Cortex-M3/M4 DWT" },
{ 0x03B02BE0, 0x05, "Cortex-M3/M4 FPB" },
{ 0x01B03BE0, 0x06, "Cortex-M3/M4 ITM" },
{ 0xA1B90B90, 0x07, "Cortex-M3/M4 TPIU" },
{ 0x25B90B90, 0x08, "Cortex-M4 ETM" },
{ 0x24B93B90, 0x08, "Cortex-M3 ETM" },
{ 0xC9B40B10, 0x01, "Cortex-M33 ROM table" },
{ 0x21BD0B90, 0x02, "Cortex-M33 SCS" },
{ 0x21BD0B90, 0x06, "Cortex-M33 ITM" },     //same as before...???!
{ 0x21BD0B90, 0x03, "Cortex-M33 DWT" },     //same as before...???!
{ 0x21BD0B90, 0x07, "Cortex-M33 TPIU" },     //same as before...???!
/* EOF */
