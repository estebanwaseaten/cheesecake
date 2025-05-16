// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include "cheese_systeminfo.h"
#include "cheese_comSWD.h"
#include "cheese_registers.h"

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

static const char * const arm_compClass[0xFF] = {
#include "arm_compclass.inc"
};

struct armComponent arm_comp[0xFF] = {
#include "arm_dbcomponents.inc"
};


// collecting information about the system via Serial Wire Debug (SW-DB)
// the STM32 (cortex-M based microcontrollers) allow debug access via the SW-DB
// via the SW-DB one or more Access Ports (AP) can be probed
// These APs consist of certain registers and a tree-like structure of components that describe/reflect the actual MCU system



int detectSystem( void )
{
    // depending on the Serial Wire Debug Command (cheese_comSWD.c) either Debug Port Registers or Access Port registers are read or written to

    //scan ACCESS PORTS:
    APinfo myAccessPorts[10];

    uint32_t APcount = 0;
    bool done = false;

    comArrayInit( &infoComArray );

    while( !done && (APcount < 10 ))
    {
        int nextIndex = comArray_prepAPaccess( &infoComArray, APcount*4, 0xF );        //NOT SURE ABOUT *4         //selects AP and AP bank
        printf("nextIndex: %d\n", nextIndex);

        //read ACCESS PORT REGISTERS
        comArrayAdd( &infoComArray, AP_READ0_CMD, 0x0 );                        //5+0
        comArrayAdd( &infoComArray, AP_READ1_CMD, 0x0 );         //...bASE2     //5+1
        comArrayAdd( &infoComArray, AP_READ2_CMD, 0x0 );         //...CFG       //5+2
        comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );         //...BASE      //5+3
        comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );         //...IDR       //5+4

        comArrayTransfer( &infoComArray );

        myAccessPorts[APcount].apIDR = comArrayRead( &infoComArray, nextIndex + 4 );        //** revision[31:28] designer[27:17] class[16:13] variant[7:4] type[3 0]

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
        }
    }

//*********************** info about debug port
    // here it only prints once :)
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


//*************** examine access ports:
    //APs
    printf( "\nFound %d Access Port(s):\n\n", APcount );
    for (size_t i = 0; i < APcount; i++)
    {
        extractAccessPort( i, &myAccessPorts[i] );
    }
    return 0;
}


void processARMComponentClass( DCinfo *info )
{

    switch( info->myclass )
    {
        case 0x02:  //SCS       System Control Block: offset 0xD00; Debug Regs offset: 0xDF0
            //The SCS is a memory-mapped 4KB address space that provides 32-bit registers for configuration, status reporting and control.
            //offsets: described in ARM6 manual
            //0x000 - 0x00F aux ctrl regs
            //0xD00 - 0xD8F System Control Block
            //0xF90 - 0xFCF implementation defined
            //0x010 - 0x0FF systick
            //0x100 - 0xCFF external interupt controller
            //0xDF0 - 0xEFF debug ctrl & conf
            //0xD90 - 0xDEF optional MPU
            info->cpuID = 0;


            break;
        default:
            break;
    }
}

uint8_t processARMComponent( DCinfo *info )   // ID = 0x PIDR0 PIDR1 PIDR2 CIDR1
{
    for (size_t i = 0; i < 0xFF; i++)
    {
        //printf( "0x%08X 0x%02X 0x%02X 0x%02X 0x%02X \n", arm_comp[i].ID, (arm_comp[i].ID >> 24) & 0xFF, (arm_comp[i].ID >> 16) & 0xFF, (arm_comp[i].ID >> 8) & 0xFF, (arm_comp[i].ID >> 0) & 0xFF );
        if( info->PCIDR == arm_comp[i].ID  )
        {
            printf( "%s, class: %s\n", arm_comp[i].description, arm_compClass[arm_comp[i].componentClass] );
            info->myclass = arm_compClass[arm_comp[i].componentClass];

            //should we process the class here??
            processARMComponentClass( info );
            return info->myclass;
        }
    }
    if ( info->class == 0x1) //ROM table
    {
        printf( "unidentified ROM table (implementation dependent)\n" );
    }
    else
    {
        printf( "unidentified\n" );
    }
    return 0x0;
}



void processComponenFields( DCinfo *componentInfo )
{
    componentInfo->class = componentInfo->CIDR[1] >> 4;     //*** 0x0=generic, 0x1=ROM Table, 0x9=CoreSightComponent, 0xB=peripheralTestBlock, 0xE=genericIPComponent, 0xF=CoreLink or PrimeCell

    // from Peripheral Identification Registers
    componentInfo->size = (componentInfo->PIDR[4] & 0xF0) >> 4;
    componentInfo->partNo = componentInfo->PIDR[0] | ((componentInfo->PIDR[1] & 0xF) << 8);     //correct

    componentInfo->revision = (componentInfo->PIDR[2] >> 4) & 0xF;
    componentInfo->revand = (componentInfo->PIDR[3] >> 4) & 0xF;
    componentInfo->custom = componentInfo->PIDR[3] & 0xF;

    componentInfo->jedec = ( componentInfo->PIDR[2] & 0x8 ) >> 3;
    componentInfo->JEP106id = ((componentInfo->PIDR[1] & 0xF0) >> 4) | ((componentInfo->PIDR[2] & 0x7) << 4);
    componentInfo->JEP106cont = componentInfo->PIDR[4] & 0xF;

    //compact single word containing hopefully all the important bits of information
    componentInfo->PCIDR = (componentInfo->PIDR[0] & 0xFF) << 24;
    componentInfo->PCIDR |= (componentInfo->PIDR[1] & 0xFF) << 16;
    componentInfo->PCIDR |= (componentInfo->PIDR[2] & 0xFF) << 8;
    componentInfo->PCIDR |= (componentInfo->CIDR[1] & 0xFF) << 0;
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

int extractComponentInfo( uint32_t base, DCinfo *componentInfo )
{
    // IHI0074E_debug_interface_v6_0_architecture_specification.pdf
    int nextIndex = comArray_prepMemAccess( &infoComArray, 0x00, base + 0xFCC );      //
    // Any ROM Table must implement a set of Component and Peripheral ID Registers, that start at offset 0xFD0
    //0xF00-0xFFC are the CoreSight management registers - common to all CoreSight Components
    //0xFCC -> memtype
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFCC
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFCC      //delayed by 1

    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFD0 (PIDR4)
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFD4 (PIDR5)
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFD8 (PIDR6)
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFDC (PIDR7)

    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFE0 (PIDR0)
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFE4 (PIDR1)
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFE8 (PIDR2)
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFEC (PIDR3)

    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFF0 (CIDR0)
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFF4 (CIDR1)
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFF8 (CIDR2)
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );   // 0xFFC (CIDR3)

    int err = comArrayTransfer( &infoComArray );

    if( err < 0 )
    {
        printf( "comArrayTransfer error!\n");
        return err;
    }

    componentInfo->memType = comArrayRead( &infoComArray, nextIndex + 1 ) & 0xFF;

    componentInfo->CIDR[0] = comArrayRead( &infoComArray, nextIndex + 10 );
    componentInfo->CIDR[1] = comArrayRead( &infoComArray, nextIndex + 11 );
    componentInfo->CIDR[2] = comArrayRead( &infoComArray, nextIndex + 12 );
    componentInfo->CIDR[3] = comArrayRead( &infoComArray, nextIndex + 13 );

    componentInfo->PIDR[0] = comArrayRead( &infoComArray, nextIndex + 6 ) & 0xFF;
    componentInfo->PIDR[1] = comArrayRead( &infoComArray, nextIndex + 7 ) & 0xFF;
    componentInfo->PIDR[2] = comArrayRead( &infoComArray, nextIndex + 8 ) & 0xFF;
    componentInfo->PIDR[3] = comArrayRead( &infoComArray, nextIndex + 9 ) & 0xFF;
    componentInfo->PIDR[4] = comArrayRead( &infoComArray, nextIndex + 2 ) & 0xFF;
    componentInfo->PIDR[5] = comArrayRead( &infoComArray, nextIndex + 3 ) & 0x0;    //reserved
    componentInfo->PIDR[6] = comArrayRead( &infoComArray, nextIndex + 4 ) & 0x0;   //reserved
    componentInfo->PIDR[7] = comArrayRead( &infoComArray, nextIndex + 5 ) & 0x0;   //reserved

    processComponenFields( componentInfo );

    return 0;
}

void tabsf( uint32_t depth )
{
    for (size_t i = 0; i < depth; i++)
    {
        printf("\t");
    }
}

// NON-critical Errors:
//  - BASE is a faulting location
//  - BASE + 0xFF0 are not valid registers
//  - ROM table entry points to faulting location
//  - ROM table entry has no valid registers at offset 0xFF0
// maybe subsystems have to be turned on first???

void extractComponent( uint32_t base, uint32_t depth )
{
    DCinfo thisComponentInfo;
    comArray localComArray;


    tabsf( depth );printf( "*** ExtractComponent at 0x%X:\n", base );

    // FIRST get information about this Component(s)
    int err = extractComponentInfo( base, &thisComponentInfo );
    if ( err < 0 )
    {

        tabsf( depth );printf( "This component seems invalid!\n" );
        return;
    }

    tabsf( depth );printf( "Class 0x%X; Memory type 0x%X\n", thisComponentInfo.class, thisComponentInfo.memType );
    tabsf( depth );printf( "PartNo 0x%X (%s); jedec: %d; JEPID 0x%X;  JEPcont 0x%X\n", thisComponentInfo.partNo, arm_partno[thisComponentInfo.partNo], thisComponentInfo.jedec, thisComponentInfo.JEP106id, thisComponentInfo.JEP106cont );
    tabsf( depth );printf( "CIDR 0-3: 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", thisComponentInfo.CIDR[0], thisComponentInfo.CIDR[1], thisComponentInfo.CIDR[2], thisComponentInfo.CIDR[3]);
    tabsf( depth );printf( "PIDR 0-4: 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", thisComponentInfo.PIDR[0], thisComponentInfo.PIDR[1], thisComponentInfo.PIDR[2], thisComponentInfo.PIDR[3], thisComponentInfo.PIDR[4]);
    tabsf( depth );printf( "Peripheral OD: 0x%02X%02X%02X%02X%02X\n", thisComponentInfo.PIDR[4] & 0xFF, thisComponentInfo.PIDR[3] & 0xFF, thisComponentInfo.PIDR[2] & 0xFF, thisComponentInfo.PIDR[1] & 0xFF, thisComponentInfo.PIDR[0] & 0xFF );
    tabsf( depth );printf( "manufacturer: %s\n", jep106[thisComponentInfo.JEP106cont][thisComponentInfo.JEP106id-1] );
    tabsf( depth );printf( "size: %d, revision: 0x%X, revand: 0x%X\n\n", thisComponentInfo.size, thisComponentInfo.revision, thisComponentInfo.revand );

    uint8_t thisComponentClass;
    tabsf( depth );printf("***** identified as: "); thisComponentClass = processARMComponent( &thisComponentInfo );




    /*if( (thisComponentInfo.compactCIDR == 0xB105100D) && (thisComponentInfo.compactPIDR == 0x04000BB4C0 ) )
    {
        printf( "***** Cortex-M0+ ROM table!!\n" );
    }
    else if( (thisComponentInfo.compactCIDR == 0xB105100D) && (thisComponentInfo.compactPIDR == 0x04002BB470 ) )
    {
        printf( "***** Cortex-M1 ROM table!!\n" );
    }
    else if( (thisComponentInfo.compactCIDR == 0xB105100D) && (thisComponentInfo.compactPIDR == 0x0 ) )
    {
        printf( "***** Cortex-M3 ROM table!!\n" );
    }
    else if( (thisComponentInfo.compactCIDR == 0xB105100D) && (thisComponentInfo.compactPIDR == 0x04000BB4C4 ) )    //this does not seem to work
    {
        printf( "***** Cortex-M4 ROM table!!\n" );
    }
    else if( (thisComponentInfo.compactCIDR == 0xB105100D) && (thisComponentInfo.compactPIDR == 0x04000BB4C8 ) )
    {
        printf( "***** Cortex-M7 ROM table!!\n" );
    }*/

    // memory Type = 0b1 if system memory is present on the bus (deprecated)

    comArrayInit( &localComArray );

    if( thisComponentInfo.class == 0x1 )    //ROM table
    {
        tabsf( depth );printf( "== ROM table content:\n\n" );       //-->> READ ALL THE ADDRESSES:

        comArray_prepMemAccess( &localComArray, 0x00, base ); //
        comArrayAdd( &localComArray, AP_READ3_CMD, 0x0 );                // 0x000 DO NOT ALTERNATE WRITES AND READS TO DRW

        for( int i = 0; i < 32; i++ )
        {
            comArrayAdd( &localComArray, AP_READ3_CMD, 0x0 );
        }
        comArrayTransfer( &localComArray );

        uint32_t    romRegister[32] = {0, };
        int         subRomCount = 0;

        for( int i = 0; i < 32; i++ )
        {
            romRegister[i] = comArrayRead( &localComArray, 7 + i );            //[31:12]=OFFSET, [8:4]=POWERID, [2]=poweridvalid, [1]=format, [0]=present
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
                    nextComponentAddr = base - ((~romRegister[i] & 0xFFFFF000) + 0x1000);     //two's complement... apparently
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
        tabsf( depth );printf( "--> Generic IP component:\n" );
    }
    else
    {
        tabsf( depth );printf( "--> not ROM table:\n" );
    }
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

    extractComponent( currentAP->apBASE, 0 );
    return 0;
}
















void read_ids()       //reads some registers
{
    int nextIndex = comArray_prepAPaccess( &infoComArray, 0, 0xF0 );

    comArrayAdd( &infoComArray, AP_READ2_CMD, 0x0 );
    comArrayAdd( &infoComArray, AP_READ3_CMD, 0x0 );
    comArrayAdd( &infoComArray, DP_READBUF_CMD, 0x0 );

    comArrayTransfer( &infoComArray );

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

    comArrayTransfer( &infoComArray );

    printf( "MCU ID: 0x%08X\n", comArrayRead( &infoComArray, nextIndex + 1 ) );     //AP ID
    printf( "MCU ID: 0x%08X\n", comArrayRead( &infoComArray, nextIndex + 4 ) );     //AP ID
}
