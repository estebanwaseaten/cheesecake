// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <stdint.h>     //uint32_t etc
#include <stdbool.h>

#ifndef CHEESE_SYSTEMINFO_H
#define CHEESE_SYSTEMINFO_H

#define COMPONENTCLASS_NONE 0x00
#define COMPONENTCLASS_ROM  0x01
#define COMPONENTCLASS_SCS  0x02
#define COMPONENTCLASS_DWT  0x03
#define COMPONENTCLASS_BPU  0x04
#define COMPONENTCLASS_FPB  0x05
#define COMPONENTCLASS_ITM  0x06
#define COMPONENTCLASS_TPIU 0x07
#define COMPONENTCLASS_ETM  0x08

struct armComponent
{
    uint32_t    ID;                 // ID = 0x PIDR0 PIDR1 PIDR2 CIDR1
    uint8_t     componentClass;     // ROM: 0x01, SCS: 0x02, DWT: 0x03, BPU: 0x04...
    const char  description[64];
};

//information for access port
typedef struct APinfo
{
    uint32_t    apIDR;
    uint32_t    apBASE;
    uint32_t    apBASE2;
    uint32_t    apCFG;

    uint8_t     type;
    uint8_t     variant;
    uint8_t     res0;       //reserved
    uint8_t     class;
    uint16_t    designer;
    uint8_t     revision;

    uint8_t     format;
    uint8_t     present;
} APinfo;

typedef struct romTable romTable;

//information for debug component
// registers PIDR0-7 and CIDR0-3 are always present and should identify the component typer or class (ROM, SCS, etc...)
typedef struct debugComponent
{
    uint32_t baseAddr;  //...
    uint32_t depth;     //for printing
    uint32_t CIDR[4];
    uint32_t PIDR[8];       //5-7 are reserved

    uint32_t PCIDR;
    uint8_t  dbcompindex;
    uint32_t dbcompclass;

    uint32_t class;
    uint32_t memType;       //could probably reduce size of these variables...
    uint32_t partNo;
    uint32_t jedec;
    uint32_t JEP106id;
    uint32_t JEP106cont;
    uint32_t size;
    uint32_t revand;
    uint32_t custom;
    uint32_t revision;

    uint32_t cpuID;

    romTable *componentROMtable;
} debugComponent;

typedef struct romTable
{
    debugComponent  components[16];
    uint32_t        compCount;
} romTable;

//utilities
void tabsf( uint32_t depth );
uint32_t calcNewBaseTwosComplement( uint32_t base, uint32_t offset );

void processARMComponentClass( debugComponent *thisComponent );
uint8_t processARMComponent( debugComponent *thisComponent );   // ID = 0x PIDR0 PIDR1 PIDR2 CIDR1
void processComponenFields( debugComponent *thisComponent );

int extractComponent( debugComponent *thisComponent );
int extractComponentBaseInfo( debugComponent *thisComponent );
void printComponentBaseInfo( debugComponent *thisComponent );

    int extractROMtable( debugComponent *thisComponent );
    int extractCoreSight( debugComponent *thisComponent );
    int extractgenericIP( debugComponent *thisComponent );

        int extractSCScomponent( debugComponent *thisComponent );
        int extractBPUComponent( debugComponent *thisComponent );
        int extractDWTComponent( debugComponent *thisComponent );




int extractAccessPort( int i, APinfo *currentAP );
int detectSystem( void );



//deprecated:
void read_ids();       //reads some registers
void read_mcu_id();











#endif
