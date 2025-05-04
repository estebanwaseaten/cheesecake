// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#ifndef REGISTERS_H
#define REGISTERS_H

// LSB means we have to flip the order (read bits from right to left)
//cmds: 1 - AP(1)/DP(0) - R(1)/W(0) - A[2] - A[3] - P(last4) - 0 - 1 OTHER WAY AROUND!!!
// CMDS for DEBUG PORT REGS:
#define DP_ABORT_CMD    0x81      // 0b10000001
#define DP_IDCODE_CMD   0xA5      // 0b10100101
                                     // 10pyyra1
//depends on CTRLSEL bit in SELECT register below
#define DP_CTRLSTAT_R_CMD 0x8D       // 0b10001101  CTRLSEL 0
#define DP_WIRECTRL_R_CMD 0x8D       //same         CTRLSEL 1
#define DP_CTRLSTAT_W_CMD 0xA9       // 0b10101001  CTRLSEL 0
#define DP_WIRECTRL_W_CMD 0xA9       //same         CTRLSEL 1

#define DP_READRE_CMD   0x95          // 0b10010101
#define DP_SELECT_CMD   0xB1          // 0b10110001

#define DP_READBUF_CMD  0xBD          // 0b10111101
#define DP_ROUTESEL_CMD 0x99            //optional/undefined




//DP_ABORT
// [4] ORUNERRCLR, [3] WDERRCLR, [2] STKERRCLR, [1] STKCMPCLR, [0] DAPABORT
// note set to 0x1 if you got a lot of WAIT responses e.g.

//DP_IDCODE
//...

//DP_CTRLSTAT (CTRLSEL bit = 0 in SELECT)
// see ARM document 6-10
//contains transfer mode bits: normal, pushed verify and pushed compare mode

//DP_SELECT selects ACCESS PORT
// [31...24] APSEL      --> selects AP
// [23...8]  RAZ/SBZ    --> ?
// [7...4]   APBANKSEL  --> selects active 4 word register bank on current AP...
// [3...1]   SBZ
// [0]       CTRLSEL bit --> default=0

//DP_READBUF
// reads previous AP-read without initiating a new AP-transaction
// only works ONCE
// see manual

// DP_WIRECTRL
// [9:8]    turnaround length *dont touch
// [7:6]    Wiremode (sync vs async)
// [2:0]    Prescaler (for async)...

//DP_READRE read resend register
// recover data from corrupted transfer ...
// retuns last read from AP (,u;tiple times if needed)


// MEM-AP:
// access is defined by DP_SELECT (see above)

// 0x0:
// 0x00 Control/Status Word register (CSW)      -->set auto-increment TAR
// 0x04 Transfer Address Register (TAR)
// 0x08 reserved
// 0x0C Data Read Write (DRW)

// 0x1:
// 0x10 banked data 0 (BD0) ,,,
//....

// 0xF:
// 0xF0 reserved
// 0xF4 Configuration Reg (CFG)
// 0xF8 Debug Base Addr (BASE)
// 0xFC Identification Register


// 1. select bank 0
// 2. write into TAR ()
// 3. read/write from register specified by tar via DRW


// 1. select bank F (DP_SELECT)
// 3. read from register 0xFC


//cmds: 1 - AP(1)/DP(0) - R(1)/W(0) - A[2] - A[3] - P(last4) - 0 - 1

#define MEMAP_READ0_CMD     0x87    // 0b10000111
#define MEMAP_READ1_CMD     0xAF    // 0b10101111
#define MEMAP_READ2_CMD     0xB7    // 0b10110111
#define MEMAP_READ3_CMD     0x9F    // 0b10011111

#define MEMAP_WRITE0_CMD    0xA3    // 0b10100011
#define MEMAP_WRITE1_CMD    0x8B    // 0b10001011
#define MEMAP_WRITE2_CMD    0x93    // 0b10010011
#define MEMAP_WRITE3_CMD    0xBB    // 0b10111011


#endif
