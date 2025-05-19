// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include "cheese_systeminfo.h"
#include "cheese_memoryaccess.h"
#include "cheese_comSWD.h"
#include "cheese_registers.h"
#include "cheese_utils.h"
#include <stdlib.h>             //calloc
#include <stdio.h>

comArray infoComArray;

static const char * const jep106[][126] = {
#include "jedec.inc"
};

static const char * const ap_types[9] = {
#include "arm_aptypes.inc"
};

static const char * const arm_partno[0xFFF] = {
#include "arm_partno.inc"
};

static const char * const arm_compClass[0xFF] = {       //is an array of constant char pointers...
#include "arm_compclass.inc"
};

struct armComponent arm_comp[0xFF] = {
#include "arm_dbcomponents.inc"
};

extern int reply;



// collecting information about the system via Serial Wire Debug (SW-DB)
// the STM32 (cortex-M based microcontrollers) allow debug access via the SW-DB
// via the SW-DB one or more Access Ports (AP) can be probed
// These APs consist of certain registers and a tree-like structure of components that describe/reflect the actual MCU system



int detectSystem( void )
{
    // depending on the Serial Wire Debug Command (see cheese_registers.h) either Debug Port Registers or Access Port registers are read or written to

    //scan ACCESS PORTS:
    APinfo myAccessPorts[10];

    uint32_t APcount = 0;
    bool done = false;

    comArrayInit( &infoComArray );

    //*********************** info about debug port
    comArray_prepAPaccess( &infoComArray, 0x0, 0x0 );   //extract IDCODE
    reply = comArrayTransfer( &infoComArray );
    if( reply < 0 )
    {
        printf( "com Error: %d\nAborting!\n", reply );
        return reply;
    }


    uint32_t dpIDCODE = comArrayRead( &infoComArray, 0 );
    //DP
    printf( "\nSW-DP IDR (IDCODE): 0x%08X\n", dpIDCODE );

    switch( dpIDCODE )
    {
        case 0x0BC11477:
            printf( "* Default Cortex-M0+ Core (uncomfirmed)\n"); //this is not confirmed
            break;
        case 0x2BA01477:
            printf( "* Cortex-M4 Core w. FPU (uncomfirmed)\n");   //this is not confirmed
            break;

        default:
            printf( "* Unknown Architecture\n");
            break;
    }

    uint32_t dpPartNo = (dpIDCODE >> 12) & 0xFFFF;
    uint32_t dpVersion = (dpIDCODE >> 28) & 0xF;

// version=0x4->JTAG-DP version=0x2->SW-DP,  version=0x3->SW-DP     ... these parameters seem to be defined differently amongst versions...
// partno=0xBA00->JTAG otherwise???->SW-DP
    if( dpPartNo == 0xBA00 )         //implies version is 0x0
    {
        printf( "* ARM JTAG-DP version: 0x%X, partno: 0x%X\n", dpVersion, dpPartNo);
    }
    else if( (dpPartNo == 0xBA10) || (dpPartNo == 0xBA01 ) || (dpPartNo == 0xBA02 ) )
    {
        printf( "* ARM SW-DP version: 0x%X, partno: 0x%X\n", dpVersion, dpPartNo);
    }
    else        // if this does not fit the older format
    {
        printf( "* other DP version: 0x%X, partno: 0x%X\n", dpVersion, dpPartNo);
    }
    printf( "* DP designer: 0x%X (%s)\n", ((dpIDCODE & 0x0FFE) >> 1), jep106[(dpIDCODE >> 8 ) & 0xF][((dpIDCODE >> 1 ) & 0x7F)-1] );




    printf( "scanning for access points: \n");
    while( !done && (APcount < 10 ))                        // access port adresses are per byte?
    {
        int nextIndex = comArray_prepAPaccess( &infoComArray, APcount, 0xF );        //NOT SURE ABOUT *4         //selects AP and AP bank
        //printf("nextIndex: %d\n", nextIndex);

        //read ACCESS PORT REGISTERS
        comArrayAdd( &infoComArray, AP_READ0_CMD, 0x0 );                        //5+0
        comArrayAdd( &infoComArray, AP_READ1_CMD, 0x0 );         //...bASE2     //5+1
        comArrayAdd( &infoComArray, AP_READ2_CMD, 0x0 );         //...CFG       //5+2
        comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );         //...BASE      //5+3
        comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );         //...IDR       //5+4

        reply = comArrayTransfer( &infoComArray );
        if( reply < 0 )
        {
            printf( "com Error: %d\nAborting!\n", reply );
            return reply;
        }

        myAccessPorts[APcount].apIDR = comArrayRead( &infoComArray, nextIndex + 4 );        //** revision[31:28] designer[27:17] class[16:13] variant[7:4] type[3 0]
        printf( "AP-IDR: 0x%08X\n", myAccessPorts[APcount].apIDR);
        if( myAccessPorts[APcount].apIDR != 0 ) //check IDCODE of AP --> AP is present if IDCODE nonzero
        {
            myAccessPorts[APcount].apBASE = comArrayRead( &infoComArray, nextIndex + 3 ) & 0xFFFFF000;      // 32-bit: [31:12] contain BASEADDR[31:12]
            myAccessPorts[APcount].apBASE2 = comArrayRead( &infoComArray, nextIndex + 1 );
            myAccessPorts[APcount].apCFG = comArrayRead( &infoComArray, nextIndex + 2 );

            myAccessPorts[APcount].format = (myAccessPorts[APcount].apBASE & 0x2) >> 1;
            myAccessPorts[APcount].present = (myAccessPorts[APcount].apBASE & 0x1);

            myAccessPorts[APcount].type = myAccessPorts[APcount].apIDR & 0xF;   //** 0x0:jtag 0x1:AMBA AHB3 bus 0x2:AMBA APB2 or APB3 ... C2-229
            myAccessPorts[APcount].variant = ((myAccessPorts[APcount].apIDR & 0xF0) >> 4);
            myAccessPorts[APcount].res0 = ((myAccessPorts[APcount].apIDR & 0x1F00) >> 8);
            myAccessPorts[APcount].class = ((myAccessPorts[APcount].apIDR & 0x1E000) >> 13);        //** 0b1000 --> MEM-AP //sometimes only bit 16 is used for class definition...
            myAccessPorts[APcount].designer = ((myAccessPorts[APcount].apIDR & 0xFFE0000) >> 17);
            myAccessPorts[APcount].revision = ((myAccessPorts[APcount].apIDR & 0xF0000000) >> 28);

            APcount++;
        }
        else
        {
            // /printf( "next AP: 0x%08X, 0x%08X, 0x%08X\n", comArrayRead( &infoComArray, 10 ), comArrayRead( &infoComArray, 9 ) & 0xFFFFF000, comArrayRead( &infoComArray, 8 ) );
            done = true;
            //APcount++;
        }

    }

//*************** extract access ports:
//APs
    printf( "\nFound %d Access Port(s):\n\n", APcount );
    for (size_t i = 0; i < APcount; i++)
    {
        extractAccessPort( i, &myAccessPorts[i] );
    }
    return 0;
}


int extractAccessPort( int i, APinfo *currentAP )
{
    if( currentAP->class == 0 )
    {
        printf( "generic AP #%d (IDR=0x%08X, type=%d):\n", i, currentAP->apIDR, ap_types[currentAP->type] );
    }
    else if( (currentAP->class == 0x8) || (currentAP->class == 0x1) )
    {
        //class could be 0x8 or 0x1 for mem-ap depending on definition of fields...
        printf( "MEM-AP #%d (IDR=0x%08X, %s):\n", i, currentAP->apIDR, ap_types[currentAP->type] );
    }
    else
    {
        printf( "Unknown Access Port Type #%d. Aborting!\n", i );
    }

    printf( "* Access Port Information:\n" );
    printf( "* BASE: 0x%08X (BASE2: 0x%08X)\n", currentAP->apBASE, currentAP->apBASE2 );
    printf( "* AP CFG: %d (little endian = 0; big endian = 1)\n", currentAP->apCFG );
    printf( "* format: %d (1->ADIv5/0->legacy), present: %d\n", currentAP->format,  currentAP->present );
    printf( "* type: %d, variant: %d, class: 0x%X, designer: 0x%X (%s), revision: 0x%X\n\n", currentAP->type, currentAP->variant, currentAP->class, currentAP->designer, jep106[(currentAP->designer >> 7 ) & 0xF][((currentAP->designer ) & 0x7F)-1]  ,currentAP->revision );

    printf( "------------------------------------------------------\n\n");

    debugComponent mainDebugComponent = {.componentROMtable = NULL };
    mainDebugComponent.baseAddr = currentAP->apBASE;
    mainDebugComponent.depth = 0;
    extractComponent( &mainDebugComponent );       //extract single base compoinent (probably a ROM table)
    //extractComponent( currentAP->apBASE + 4, 0 );
    return 0;
}



// * IHI0031G_debug_interface_v5_2_architecture_specification.pdf
// ** IHI0074E_debug_interface_v6_0_architecture_specification.pdf
// *** IHI0029F_coresight_v3_0_architecture_specification.pdf

// DDI0484C_cortex_m0p_r0p1_trm.pdf
// DDI0419E_armv6m_arm.pdf

// DDI0439B_cortex_m4_r0p0_trm.pdf
// DDI0403E_e_armv7m_arm.pdf

// DBGMCU_IDCODE at address 0xE0042000  (for some devices)
// the Cortex SCB->CPUID would also be interesting... but where are these located???

int extractComponentBaseInfo( debugComponent *thisComponent )
{
    // IHI0074E_debug_interface_v6_0_architecture_specification.pdf
    int nextIndex = comArray_prepMemAccess( &infoComArray, 0x00, thisComponent->baseAddr + 0xFCC );      //
    // Any ROM Table must implement a set of Component and Peripheral ID Registers, that start at offset 0xFD0
    //0xF00-0xFFC are the CoreSight management registers - common to all CoreSight Components
    //0xFCC -> memtype

    for( int i = 0; i < 14; i++ )
    {
        comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // starting at 0xFCC ending at 0xFFC
    }

    reply = comArrayTransfer( &infoComArray );
    if( reply  < 0 )
    {
        printf( "comArrayTransfer error!\n");
        return reply;
    }

    thisComponent->memType = comArrayRead( &infoComArray, nextIndex + 1 ) & 0xFF;

    thisComponent->CIDR[0] = comArrayRead( &infoComArray, nextIndex + 10 );
    thisComponent->CIDR[1] = comArrayRead( &infoComArray, nextIndex + 11 );
    thisComponent->CIDR[2] = comArrayRead( &infoComArray, nextIndex + 12 );
    thisComponent->CIDR[3] = comArrayRead( &infoComArray, nextIndex + 13 );

    thisComponent->PIDR[0] = comArrayRead( &infoComArray, nextIndex + 6 ) & 0xFF;
    thisComponent->PIDR[1] = comArrayRead( &infoComArray, nextIndex + 7 ) & 0xFF;
    thisComponent->PIDR[2] = comArrayRead( &infoComArray, nextIndex + 8 ) & 0xFF;
    thisComponent->PIDR[3] = comArrayRead( &infoComArray, nextIndex + 9 ) & 0xFF;
    thisComponent->PIDR[4] = comArrayRead( &infoComArray, nextIndex + 2 ) & 0xFF;
    thisComponent->PIDR[5] = comArrayRead( &infoComArray, nextIndex + 3 ) & 0x0;    //reserved
    thisComponent->PIDR[6] = comArrayRead( &infoComArray, nextIndex + 4 ) & 0x0;   //reserved
    thisComponent->PIDR[7] = comArrayRead( &infoComArray, nextIndex + 5 ) & 0x0;   //reserved

    if( (thisComponent->CIDR[0] != 0xD) || (thisComponent->CIDR[2] != 0x5) || (thisComponent->CIDR[3] != 0xB1) )
    {
        return -8;
    }

    processComponenFields( thisComponent );

    return 0;
}

// NON-critical Errors:
//  - BASE is a faulting location
//  - BASE + 0xFF0 are not valid registers
//  - ROM table entry points to faulting location
//  - ROM table entry has no valid registers at offset 0xFF0
// maybe subsystems have to be turned on first???

int extractComponent( debugComponent *thisComponent )
{
    //wait
    tabsf( thisComponent->depth ); printf( "\n*** ExtractComponent at 0x%X:\n", thisComponent->baseAddr );

    int err = extractComponentBaseInfo( thisComponent );
    if ( err < 0 )
    {
        tabsf( thisComponent->depth );printf( "This component seems invalid!\n" );
        return -8;
    }

    printComponentBaseInfo( thisComponent );

    //wait

    //class dependent further processing
    switch( thisComponent->class )
    {
        case 0x1:       //ROM table
            extractROMtable( thisComponent );
            break;

        case 0x9:       //CoreSightComponent
            extractCoreSight( thisComponent );
            break;

        case 0xE:       //genericIPComponent
            extractgenericIP( thisComponent );
            break;

        default:
            break;
    }
}

int extractROMtable( debugComponent *thisComponent )
{
    tabsf( thisComponent->depth ); printf( "*** extractROMtable:\n" );

    comArray localComArray;
    comArrayInit( &localComArray );
    int nextIndex = comArray_prepMemAccess( &localComArray, 0x00, thisComponent->baseAddr );

    comArrayAdd( &localComArray, AP_READ3_CMD, 0x0 );                       //start reading rom table entries
    for( int i = 0; i < 32; i++ )
    {
        comArrayAdd( &localComArray, AP_READ3_CMD, 0x0 );
    }
    reply = comArrayTransfer( &localComArray );
    if( reply < 0 )
    {
        printf( "com Error: %d\nAborting!\n", reply );
        return reply;
    }

    uint32_t    romRegister[32] = {0, };
    thisComponent->componentROMtable = (romTable*)calloc( 1, sizeof( romTable ) );     //create a rom table with max 16 components
    thisComponent->componentROMtable->compCount = 0;

    for( int i = 0; i < 16; i++ )   //--> check for valid rom table entries
    {
        uint32_t tempReg = comArrayRead( &localComArray, nextIndex + 1 + i );            //[31:12]=OFFSET, [8:4]=POWERID, [2]=poweridvalid, [1]=format, [0]=present
        if( tempReg == 0x0 )
        {
            //skip
        }
        else
        {
            if( tempReg & 0x1 )
            {// calculate correct compoinent base Addr:
                uint32_t newBase = calcNewBaseTwosComplement( thisComponent->baseAddr, tempReg );
                tabsf( thisComponent->depth ); printf( "rom table entry: 0x%08X Component is present --> component BASE: 0x%08X\n", tempReg, newBase );
                thisComponent->componentROMtable->components[thisComponent->componentROMtable->compCount].baseAddr = newBase;
                thisComponent->componentROMtable->components[thisComponent->componentROMtable->compCount].depth = thisComponent->depth + 1;
                thisComponent->componentROMtable->compCount++;
            }
            else
            {
                tabsf( thisComponent->depth ); printf( "rom table entry: 0x%08X Component is NOT present \n", tempReg );
            }
        }
    }
    tabsf( thisComponent->depth ); printf( "*** found %d ROM table entries\n", thisComponent->componentROMtable->compCount );
    //process the components of the rom table
    for( int i = 0; i < thisComponent->componentROMtable->compCount; i++ )
    {
        reply = extractComponent( &thisComponent->componentROMtable->components[i] );
    }
    return 0;
}

int extractCoreSight( debugComponent *thisComponent )
{
    tabsf( thisComponent->depth ); printf( "*** extractCoreSight:\n" );
    return 0;
}

int extractgenericIP( debugComponent *thisComponent )
{
    tabsf( thisComponent->depth ); printf( "*** extractgenericIP:\n" );

    //
    switch( thisComponent->dbcompclass )
    {
        case COMPONENTCLASS_SCS:
            extractSCScomponent( thisComponent );
            break;
    }




    return 0;
}

int extractSCScomponent( debugComponent *thisComponent )
{
    //uint32_t registersSCB[0x40] = {0};
    //stmFetch( thisComponent->baseAddr + 0xD00, 0x40, registersSCB );
    uint32_t cpuid = comArray_readWord( thisComponent->baseAddr + 0xD00 );      //same, just checking

    tabsf( thisComponent->depth );printf( "SCS CPUID: 0x%08X (%s)\n", cpuid, arm_partno[(cpuid >> 4) & 0xFFF] );

    //this is from the datasheet though...


    uint32_t flash_size = comArray_readWord( 0x1FF8007C ) & 0xFFFF;
    tabsf( thisComponent->depth );printf( "flash size: %d kB\n", flash_size );


}





void printComponentBaseInfo( debugComponent *thisComponent )
{
    tabsf( thisComponent->depth );printf( "Class 0x%X; Memory type 0x%X\n", thisComponent->class, thisComponent->memType );
    tabsf( thisComponent->depth );printf( "PartNo 0x%X (%s); jedec: %d; JEPID 0x%X;  JEPcont 0x%X\n", thisComponent->partNo, arm_partno[thisComponent->partNo], thisComponent->jedec, thisComponent->JEP106id, thisComponent->JEP106cont );
    tabsf( thisComponent->depth );printf( "CIDR 0-3: 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", thisComponent->CIDR[0], thisComponent->CIDR[1], thisComponent->CIDR[2], thisComponent->CIDR[3]);
    tabsf( thisComponent->depth );printf( "PIDR 0-4: 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", thisComponent->PIDR[0], thisComponent->PIDR[1], thisComponent->PIDR[2], thisComponent->PIDR[3], thisComponent->PIDR[4]);
    //tabsf( thisComponent->depth );printf( "PCIDR: 0x%08X\n", thisComponent->PCIDR );
    tabsf( thisComponent->depth );printf( "manufacturer: %s\n", jep106[thisComponent->JEP106cont][thisComponent->JEP106id-1] );
    tabsf( thisComponent->depth );printf( "size: %d, revision: 0x%X, revand: 0x%X\n", thisComponent->size, thisComponent->revision, thisComponent->revand );
    tabsf( thisComponent->depth );printf("***** identified as: %s\n", arm_comp[thisComponent->dbcompindex].description);
}

void processComponenFields( debugComponent *thisComponent )
{
    thisComponent->class = thisComponent->CIDR[1] >> 4;     //*** 0x0=generic, 0x1=ROM Table, 0x9=CoreSightComponent, 0xB=peripheralTestBlock, 0xE=genericIPComponent, 0xF=CoreLink or PrimeCell

    // from Peripheral Identification Registers
    thisComponent->size = (thisComponent->PIDR[4] & 0xF0) >> 4;
    thisComponent->partNo = thisComponent->PIDR[0] | ((thisComponent->PIDR[1] & 0xF) << 8);     //correct

    thisComponent->revision = (thisComponent->PIDR[2] >> 4) & 0xF;
    thisComponent->revand = (thisComponent->PIDR[3] >> 4) & 0xF;
    thisComponent->custom = thisComponent->PIDR[3] & 0xF;

    thisComponent->jedec = ( thisComponent->PIDR[2] & 0x8 ) >> 3;
    thisComponent->JEP106id = ((thisComponent->PIDR[1] & 0xF0) >> 4) | ((thisComponent->PIDR[2] & 0x7) << 4);
    thisComponent->JEP106cont = thisComponent->PIDR[4] & 0xF;

    //compact single word containing hopefully all the important bits of information
    thisComponent->PCIDR = (thisComponent->PIDR[0] & 0xFF) << 24;
    thisComponent->PCIDR |= (thisComponent->PIDR[1] & 0xFF) << 16;
    thisComponent->PCIDR |= (thisComponent->PIDR[2] & 0xFF) << 8;
    thisComponent->PCIDR |= (thisComponent->CIDR[1] & 0xFF) << 0;

    //defaults:
    thisComponent->dbcompindex = 0;
    thisComponent->dbcompclass = 0x0;
    for( int i = 0; i < 0xFF; i++ )     //look up the PCIDR in the arm_comp[] table.
    {
        if( arm_comp[i].ID == thisComponent->PCIDR )
        {
            thisComponent->dbcompindex = i;
            thisComponent->dbcompclass = arm_comp[i].componentClass;
        }
    }
}

















/*
int extractComponent( uint32_t base, uint32_t depth )
{
    bool valid = true;

    tabsf( depth );printf( "*** ExtractComponent at 0x%X:\n", base );

    // FIRST get information about this Component(s)
    componentInfo mainComponentInfo;
    int err = extractComponentBaseInfo( base, &thisComponentInfo );
    if ( err < 0 )
    {
        tabsf( depth );printf( "This component seems invalid!\n" );
        valid = false;
        return -8;
    }
    color_green();
    if( (thisComponentInfo.CIDR[0] != 0xD) || (thisComponentInfo.CIDR[2] != 0x5) || (thisComponentInfo.CIDR[3] != 0xB1) )
    {
        color_default();
        tabsf( depth ); printf( "This component seems invalid!\n" );
        valid = false;
    }

    tabsf( depth );printf( "Class 0x%X; Memory type 0x%X\n", thisComponentInfo.class, thisComponentInfo.memType );
    tabsf( depth );printf( "PartNo 0x%X (%s); jedec: %d; JEPID 0x%X;  JEPcont 0x%X\n", thisComponentInfo.partNo, arm_partno[thisComponentInfo.partNo], thisComponentInfo.jedec, thisComponentInfo.JEP106id, thisComponentInfo.JEP106cont );
    tabsf( depth );printf( "CIDR 0-3: 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", thisComponentInfo.CIDR[0], thisComponentInfo.CIDR[1], thisComponentInfo.CIDR[2], thisComponentInfo.CIDR[3]);
    tabsf( depth );printf( "PIDR 0-4: 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", thisComponentInfo.PIDR[0], thisComponentInfo.PIDR[1], thisComponentInfo.PIDR[2], thisComponentInfo.PIDR[3], thisComponentInfo.PIDR[4]);

    if ( !valid )
    {
        return -8;
    }


    uint8_t thisComponentClass;
    tabsf( depth );printf("***** identified as: "); thisComponentClass = processARMComponent( &thisComponentInfo );
    color_default();



    if( thisComponentInfo.class == 0x1 )    //ROM table
    {
        tabsf( depth );printf( "== ROM table content:\n\n" );       //-->> READ ALL THE ADDRESSES:

        int nextIndex = comArray_prepMemAccess( &localComArray, 0x00, base ); // SEGMENTATION FAULT

        comArrayAdd( &localComArray, AP_READ3_CMD, 0x0 );                // 0x000 DO NOT ALTERNATE WRITES AND READS TO DRW

        for( int i = 0; i < 32; i++ )
        {
            comArrayAdd( &localComArray, AP_READ3_CMD, 0x0 );
        }

        reply = comArrayTransfer( &localComArray );
        if( reply < 0 )
        {
            printf( "com Error: %d\nAborting!\n", reply );
            return reply;
        }

        uint32_t    romRegister[32] = {0, };
        int         subRomCount = 0;

        for( int i = 0; i < 32; i++ )
        {
            romRegister[i] = comArrayRead( &localComArray, nextIndex + 1 + i );            //[31:12]=OFFSET, [8:4]=POWERID, [2]=poweridvalid, [1]=format, [0]=present
            if( romRegister[i] == 0x0 )
            {
                //i = 32;
            }
            else
            {
                tabsf( depth );printf("RAW ROM REGISTER: 0x%08X\n", romRegister[i] );

                uint32_t nextComponentAddr;
                if( (romRegister[i] >> 31) == 1 )    //negative    (most significant bit true -> negative)
                {
                    nextComponentAddr = base - ((~romRegister[i] & 0xFFFFF000) + 0x1000);     //two's complement (bitshifted 12 to the left)... apparently
                    tabsf( depth );printf( "---> offset: -0x%08X addr: 0x%08X\n", ~(romRegister[i] & 0xFFFFF000) + 0x1000, nextComponentAddr );
                }
                else
                {
                    nextComponentAddr = base + (romRegister[i] & 0x7FFFF000);
                    tabsf( depth );printf( "---> offset: 0x%08X addr: 0x%08X\n", (romRegister[i] & 0x7FFFF000), nextComponentAddr );   //drop bit 19
                }
                tabsf( depth );printf( "POWERID: 0x%02X, valid=%d, format=%d(RAO), present=%d\n", ((romRegister[i] & 0x1F0) >> 4), ((romRegister[i] & 0x4) >> 2), ((romRegister[i] & 0x2) >> 1), (romRegister[i] & 0x1) );

                if( (romRegister[i] & 0x1) )
                {
                    tabsf( depth );printf( "Component present - extract:\n\n");
                    extractComponent( nextComponentAddr & 0xFFFFFFFC, depth + 1 );
                }
                else
                {
                    tabsf( depth );printf( "Component not present - skip!\n");
                }

                printf("\n");
                subRomCount++;
            }
        }
        tabsf( depth );printf( "(found %d entries in ROM table!)\n\n", subRomCount );
    }
    else if( thisComponentInfo.class == 0x9 )   //IHI0029F_coresight_v3_0_architecture_specification.pdf
    {
        tabsf( depth );printf( "--> CoreSightComponent:\n" );       //not sure how to use those...      Additional regs at 0xF00 to 0xFFF
        //Addresses 0xF00 to 0xFCC are reserved for use by CoreSight management registers.
        //AUTHSTATUS @ 0xFB8
        //CLAIMSET @ 0xFA0 & CLAIMCLR @ 0xFA4
        // Device Affinity Registers DEVAFF0 @ 0xFA8 and DEVAFF1 @ 0xFAC
        // Device Architecture Register DEVARCH @ 0xFBC
        // Device Configuration Register DEVID @ 0xFC8 DEVID1 @ 0xFC4 DEVID2 @ 0xFC0
        // Device Type Identifier Register DEVTYPE @ 0xFCC
        // Integration Mode Control Register ITCTRL @ 0xF00
        // Software Lock Status/Access Register LSR @ 0xFB4 LAR @ 0xFB0


    }
    else if( thisComponentInfo.class == 0xE )
    {
        tabsf( depth );printf( "(Generic IP component)\n" );
        //here we can extract SCS, FPB and ETM usw... if we want...


    }
    else
    {
        tabsf( depth );printf( "(not ROM table)s\n" );
    }

    comArrayDestroy( &localComArray );

    return 0;
}*/













uint32_t calcNewBaseTwosComplement( uint32_t base, uint32_t offset )
{
    if( (offset >> 31) == 1 )    //negative    (most significant bit true -> negative)
    {
        return base - ((~offset & 0xFFFFF000) + 0x1000);     //two's complement (bitshifted 12 to the left)... apparently
        //printf( "---> offset: -0x%08X addr: 0x%08X\n", ~(offset & 0xFFFFF000) + 0x1000, nextComponentAddr );
    }
    else
    {
        return base + (offset & 0x7FFFF000);
        //printf( "---> offset: 0x%08X addr: 0x%08X\n", (offset & 0x7FFFF000), nextComponentAddr );   //drop bit 19
    }
}

void tabsf( uint32_t depth )
{
    for (size_t i = 0; i < depth; i++)
    {
        printf("\t");
    }
}



void read_ids()       //reads some registers
{
    int nextIndex = comArray_prepAPaccess( &infoComArray, 0, 0xF0 );

    comArrayAdd( &infoComArray, AP_READ2_CMD, 0x0 );
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );
    comArrayAdd( &infoComArray, DP_READBUF_CMD, 0x0 );

    reply = comArrayTransfer( &infoComArray );
    if( reply < 0 )
    {
        printf( "com Error: %d\nAborting!\n", reply );
        return;
    }

    //printf( "IDCODE: 0x%08X (0x%08X)\n", myReadBuffer[2*0 + 1], myReadBuffer[2*0]);     //IDCODE
    //printf( "MEM-AP info: \n");
    //printf( "DEBUG BASE: 0x%08X (0x%08X)\n", myReadBuffer[2*5 + 1], myReadBuffer[2*5]);
    //printf( "AP-IDR: 0x%08X (0x%08X)\n", myReadBuffer[2*6 + 1], myReadBuffer[2*6]);     //AP ID
}

//this could be implemented in the driver as some higher level CMD resulting in the whole memory read portion terminated by a 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
void read_mcu_id()
{
    int nextIndex = comArray_prepMemAccess( &infoComArray, 0x0, 0x40015800 );

    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );
    comArrayAdd( &infoComArray, AP_WRITE1_CMD, 0xE0042000 );     // STM32F...
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );

    reply = comArrayTransfer( &infoComArray );
    if( reply < 0 )
    {
        printf( "com Error: %d\nAborting!\n", reply );
        return;
    }

    printf( "MCU ID: 0x%08X\n", comArrayRead( &infoComArray, nextIndex + 1 ) );     //AP ID
    printf( "MCU ID: 0x%08X\n", comArrayRead( &infoComArray, nextIndex + 4 ) );     //AP ID
}
