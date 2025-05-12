//#include <iostream>
#include <stdint.h>
#include <fcntl.h>      // O_RDWR | O_SYNC etc
#include <unistd.h>     //close()
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "SWDPI_base.h"
#include "registers.h"

// a lot of sequences in openocd/src/jtag/swd.h

//for comparing:
// hexyl
// xxd -
// vbindiff

#define MAX_COMS 64

static const char * const jep106[][126] = {
#include "jedec.inc"
};

static const char * const ap_types[9] = {
#include "aptypes.inc"
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

//information for debug component
typedef struct DCinfo
{
    uint32_t CIDR[4];
    uint32_t PIDR[8];       //5-7 are reserved

    uint32_t compactCIDR;
    uint64_t compactPIDR;

    uint32_t class;
    uint32_t memType;       //could probably reduce size of these variables...
    uint32_t partNo;
    uint32_t jedec;
    uint32_t JEP106id;
    uint32_t JEP106cont;
    uint32_t size;
    uint32_t revand;
} DCinfo;

//for SWDB comms
typedef struct comArray
{
    uint32_t *cmdArray32;
    uint32_t *replyArray32;
    uint8_t  initDone;
    uint32_t filling;
} comArray;

comArray mainComArray;

void comArrayInit( comArray *myComArray )
{
    myComArray->cmdArray32 = calloc( MAX_COMS, 8 );         //8 bytes each command
    myComArray->replyArray32 = calloc( MAX_COMS, 8 );
    myComArray->initDone = 0xF0;
    myComArray->filling = 0;
}

int comArrayClear( comArray *myComArray )
{
    if( (myComArray == NULL) || (myComArray->initDone == 0) )
    {
        printf( "comArrayClear() NULL or not inited\n" );
        return -1;
    }
    myComArray->cmdArray32 = memset( myComArray->cmdArray32, 0x0, MAX_COMS*8 );
    myComArray->replyArray32 = memset( myComArray->cmdArray32, 0x0, MAX_COMS*8 );
    myComArray->initDone = 0xF0;
    myComArray->filling = 0;

    return 0;
}

int comArrayAdd( comArray *myComArray, uint32_t cmd, uint32_t val )
{
    if( (myComArray == NULL) || (myComArray->initDone == 0) )
    {
        printf( "comArrayAdd() NULL or not inited\n" );
        return -1;
    }
    if ( myComArray->filling >= MAX_COMS )
    {
        printf( "comArrayAdd() full\n" );
        return -2;
    }
    myComArray->cmdArray32[myComArray->filling*2] = cmd;
    myComArray->cmdArray32[myComArray->filling*2 + 1] = val;
    myComArray->filling++;

    return myComArray->filling;
}

uint32_t comArrayRead( comArray *myComArray, uint32_t index )       //index starts at 0
{
    if( (myComArray == NULL) || (myComArray->initDone == 0) )
    {
        printf( "comArrayRead() NULL or not inited\n" );
        return -1;
    }
    if ( index >= myComArray->filling )
    {
        printf( "comArrayRead() index not available\n" );
        return -2;
    }

    return myComArray->replyArray32[index*2 + 1];
}

int comArrayTransfer( comArray *myComArray )
{
    int SWDPIfile = open("/dev/SWDPI", O_RDWR | O_SYNC);
    write( SWDPIfile, myComArray->cmdArray32, myComArray->filling*2*4 );             //in bytes. each commandsDone has 4 bytes
    read( SWDPIfile, myComArray->replyArray32,  myComArray->filling*2*4 );

    //check for errors:
    for (size_t i = 0; i < myComArray->filling; i++)
    {
        if( (myComArray->replyArray32[i*2] >> 24) == 2 ) //if ACK not OK
        {
            printf( "SWD transfer error, received WAIT on transfer #%d\n", i);
            close( SWDPIfile );
            return -2;
        }
        else if( (myComArray->replyArray32[i*2] >> 24) == 4 ) //if ACK not OK
        {
            printf( "SWD transfer error, received FAIL on transfer #%d\n", i);
            close( SWDPIfile );
            return -4;
        }
    }

    close( SWDPIfile );
    return 0;
}

void processComponenFields( DCinfo *componentInfo )
{
    componentInfo->class = componentInfo->CIDR[1] >> 4;     //*** 0x0=generic, 0x1=ROM Table, 0x9=CoreSightComponent, 0xB=peripheralTestBlock, 0xE=genericIPComponent, 0xF=CoreLink or PrimeCell

    // from Peripheral Identification Registers
    componentInfo->size = (componentInfo->PIDR[4] & 0xF0) >> 4;

    componentInfo->partNo = componentInfo->PIDR[0] | ((componentInfo->PIDR[1] & 0xF) << 8);     //correct

    componentInfo->jedec = ( componentInfo->PIDR[2] & 0x8 ) >> 3;
    componentInfo->JEP106id = ((componentInfo->PIDR[1] & 0xF0) >> 4) | ((componentInfo->PIDR[2] & 0x7) << 4);
    componentInfo->JEP106cont = componentInfo->PIDR[4] & 0xF;

    componentInfo->compactCIDR = (componentInfo->CIDR[0] & 0xFF) << 0;
    componentInfo->compactCIDR |= (componentInfo->CIDR[1] & 0xFF) << 8;
    componentInfo->compactCIDR |= (componentInfo->CIDR[2] & 0xFF) << 16;
    componentInfo->compactCIDR |= (componentInfo->CIDR[3] & 0xFF) << 24;

    componentInfo->compactPIDR = (componentInfo->PIDR[4] & 0xFF);//
    componentInfo->compactPIDR <<= 32;
    componentInfo->compactPIDR |= (componentInfo->PIDR[0] & 0xFF) << 0;
    componentInfo->compactPIDR |= (componentInfo->PIDR[1] & 0xFF) << 8;
    componentInfo->compactPIDR |= (componentInfo->PIDR[2] & 0xFF) << 16;
    componentInfo->compactPIDR |= (componentInfo->PIDR[3] & 0xFF) << 24;

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
    comArrayClear( &mainComArray );

    comArrayAdd( &mainComArray, DP_IDCODE_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );
    comArrayAdd( &mainComArray, DP_CTRLSTAT_R_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_SELECT_CMD, 0x00 );
    comArrayAdd( &mainComArray, MEMAP_WRITE0_CMD, 0x22000012 );               // CSW
    comArrayAdd( &mainComArray, MEMAP_WRITE1_CMD, base + 0xFCC );             // TAR, set offset 0xFD0 to CIDR0
    // Any ROM Table must implement a set of Component and Peripheral ID Registers, that start at offset 0xFD0
    //0xF00-0xFFC are the CoreSight management registers - common to all CoreSight Components
    //0xFCC -> memtype
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFCC
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFCC      //delayed by 1

    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFD0 (PIDR4)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFD4 (PIDR5)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFD8 (PIDR6)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFDC (PIDR7)

    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFE0 (PIDR0)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFE4 (PIDR1)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFE8 (PIDR2)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFEC (PIDR3)

    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFF0 (CIDR0)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFF4 (CIDR1)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFF8 (CIDR2)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFFC (CIDR3)

    int err = comArrayTransfer( &mainComArray );

    if( err < 0 )
    {
        printf( "comArrayTransfer error!\n");
        return err;
    }

    componentInfo->memType = comArrayRead( &mainComArray, 7 ) & 0xFF;

    componentInfo->CIDR[0] = comArrayRead( &mainComArray, 16 );
    componentInfo->CIDR[1] = comArrayRead( &mainComArray, 17 );
    componentInfo->CIDR[2] = comArrayRead( &mainComArray, 18 );
    componentInfo->CIDR[3] = comArrayRead( &mainComArray, 19 );

    componentInfo->PIDR[0] = comArrayRead( &mainComArray, 12 ) & 0xFF;
    componentInfo->PIDR[1] = comArrayRead( &mainComArray, 13 ) & 0xFF;
    componentInfo->PIDR[2] = comArrayRead( &mainComArray, 14 ) & 0xFF;
    componentInfo->PIDR[3] = comArrayRead( &mainComArray, 15 ) & 0xFF;
    componentInfo->PIDR[4] = comArrayRead( &mainComArray, 8 ) & 0xFF;
    componentInfo->PIDR[5] = comArrayRead( &mainComArray, 9 ) & 0x0;    //reserved
    componentInfo->PIDR[6] = comArrayRead( &mainComArray, 10 ) & 0x0;   //reserved
    componentInfo->PIDR[7] = comArrayRead( &mainComArray, 11 ) & 0x0;   //reserved

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


    tabsf( depth );printf( "ExtractComponent at 0x%X:\n", base );

    // FIRST get information about this Component(s)
    int err = extractComponentInfo( base, &thisComponentInfo );
    if ( err < 0 )
    {

        tabsf( depth );printf( "This component seems invalid!\n" );
        return;
    }

    tabsf( depth );printf( "Class 0x%X; Memory type 0x%X\n", thisComponentInfo.class, thisComponentInfo.memType );
    tabsf( depth );printf( "PartNo 0x%X; jedec: %d; JEPID 0x%X;  JEPcont 0x%X\n", thisComponentInfo.partNo, thisComponentInfo.jedec, thisComponentInfo.JEP106id, thisComponentInfo.JEP106cont );
    tabsf( depth );printf( "CIDR 0-3: 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", thisComponentInfo.CIDR[0], thisComponentInfo.CIDR[1], thisComponentInfo.CIDR[2], thisComponentInfo.CIDR[3]);
    tabsf( depth );printf( "PIDR 0-4: 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", thisComponentInfo.PIDR[0], thisComponentInfo.PIDR[1], thisComponentInfo.PIDR[2], thisComponentInfo.PIDR[3], thisComponentInfo.PIDR[4]);
    tabsf( depth );printf( "manufacturer: %s\n\n", jep106[thisComponentInfo.JEP106cont][thisComponentInfo.JEP106id-1] );

    tabsf( depth );
    if( (thisComponentInfo.compactCIDR == 0xB105100D) && (thisComponentInfo.compactPIDR == 0x04000BB4C0 ) )
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
    else if( (thisComponentInfo.compactCIDR == 0xB105100D) && (thisComponentInfo.compactPIDR == 0x04000BB4C4 ) )
    {
        printf( "***** Cortex-M4 ROM table!!\n" );
    }
    else if( (thisComponentInfo.compactCIDR == 0xB105100D) && (thisComponentInfo.compactPIDR == 0x04000BB4C8 ) )
    {
        printf( "***** Cortex-M7 ROM table!!\n" );
    }

    // memory Type = 0b1 if system memory is present on the bus (deprecated)

    comArrayInit( &localComArray );

    if( thisComponentInfo.class == 0x1 )    //ROM table
    {
        tabsf( depth );printf( "== ROM table content:\n\n" );       //-->> READ ALL THE ADDRESSES:
        comArrayClear( &localComArray );

        comArrayAdd( &localComArray, DP_IDCODE_CMD, 0x0 );
        comArrayAdd( &localComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );
        comArrayAdd( &localComArray, DP_CTRLSTAT_R_CMD, 0x0 );
        comArrayAdd( &localComArray, DP_SELECT_CMD, 0x00 );
        comArrayAdd( &localComArray, MEMAP_WRITE0_CMD, 0x22000012 );
        comArrayAdd( &localComArray, MEMAP_WRITE1_CMD, base);               //is fine for base access port/component
        comArrayAdd( &localComArray, MEMAP_READ3_CMD, 0x0 );                // 0x000 DO NOT ALTERNATE WRITES AND READS TO DRW

        for( int i = 0; i < 32; i++ )
        {
            comArrayAdd( &localComArray, MEMAP_READ3_CMD, 0x0 );
        }
        comArrayTransfer( &localComArray );

        uint32_t    romRegister[32] = {0, };
        int         subRomCount = 0;

        for( int i = 0; i < 32; i++ )
        {
            romRegister[i] = comArrayRead( &localComArray, 7 + i );            //[31:12]=OFFSET, [8:4]=POWERID, [2]=poweridvalid, [1]=format, [0]=present
            if( romRegister[i] == 0x0 )
            {
                i = 32;
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
                    tabsf( depth );printf( "Component present: extract!\n\n");
                    extractComponent( nextComponentAddr & 0xFFFFFFFC, depth + 1 );                       //this some how does not work...
                }
                else
                {
                    tabsf( depth );printf( "Component not present: skip!\n");
                }

                printf("\n");
                subRomCount++;
            }
        }
        tabsf( depth );printf( "(found %d entries in ROM table!)\n\n", subRomCount );
    }
    else if( thisComponentInfo.class == 0x9 )   //IHI0029F_coresight_v3_0_architecture_specification.pdf
    {
        tabsf( depth );printf( "--> CoreSightComponent:\n" );       //not sure how to use those...
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



int extractAccessPort( APinfo *currentAP )
{
    printf( "Access Port Information:\n" );
    printf( "\BASE: 0x%08X (BASE2: 0x%08X)\n", currentAP->apBASE, currentAP->apBASE2 );
    printf( "\tAP CFG: %d (little endian = 0; big endian = 1)\n", currentAP->apCFG );
    printf( "\tformat: %d (1->ADIv5/0->legacy), present: %d\n", currentAP->format,  currentAP->present );
    printf( "\ttype: %d, variant: %d, class: 0x%X, designer: 0x%X, revision: 0x%X\n\n", currentAP->type, currentAP->variant, currentAP->class, currentAP->designer ,currentAP->revision );




    printf( "------------------------------------------------------\n\n");

    extractComponent( currentAP->apBASE, 0 );

    printf("\n\n");

    //extractComponent( 0xFFF0F000 );
/*printf("\n");
    extractComponent( 0xFFF02000 );
printf("\n");
    extractComponent( 0xFFF03000 );
printf("\n");
    extractComponent( 0xFFF01000 );
printf("\n");
    extractComponent( 0xFFF41000 );
printf("\n");
    extractComponent( 0xFFF42000 );*/

    //extractComponent( 0xFFF0F000 ); //same as before???
    //extractComponent( 0x00200000 ); // THIS crashes the MCU???

    return 0;
}



int detectSystem( void )
{
    //scan ACCESS PORTS:
    APinfo myAccessPorts[10];

    uint32_t APcount = 0;
    bool done = false;

    comArrayInit( &mainComArray );

    while( !done && (APcount < 10 ))
    {
        comArrayClear( &mainComArray );

        comArrayAdd( &mainComArray, DP_IDCODE_CMD, 0x0 );                   //read DP-idcode
        comArrayAdd( &mainComArray, DP_CTRLSTAT_R_CMD, 0x0 );               //read potential errors
        comArrayAdd( &mainComArray, DP_ABORT_CMD, 0x1E );                   //clean errors
        comArrayAdd( &mainComArray, DP_CTRLSTAT_W_CMD, 0x54000000 );        //power up requests [30], Debug power-up request [28], Debug reset request [26]. [30,28] = 0x50000000; [30,28,26] = 0x54000000
        comArrayAdd( &mainComArray, DP_CTRLSTAT_R_CMD, 0x0 );

        comArrayAdd( &mainComArray, DP_SELECT_CMD, (APcount << 24) | 0xF0 );                  //selects AP [31:24] = 0x0, bank [7:4] = 0xF

        //read ACCESS PORT REGISTERS
        comArrayAdd( &mainComArray, MEMAP_READ0_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ1_CMD, 0x0 );         //...bASE2
        comArrayAdd( &mainComArray, MEMAP_READ2_CMD, 0x0 );         //...CFG
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );         //...BASE
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );         //...IDR

        comArrayTransfer( &mainComArray );

        //printf( "*CTRLSTAT before cleaning errs: 0x%08X\n", comArrayRead( &mainComArray, 1 ) );
        //printf( "*CTRLSTAT after cleaning errs: 0x%08X\n", comArrayRead( &mainComArray, 4 ) );

        myAccessPorts[APcount].apIDR = comArrayRead( &mainComArray, 10 );    //** revision[31:28] designer[27:17] class[16:13] variant[7:4] type[3 0]

        if( myAccessPorts[APcount].apIDR != 0 ) //check IDCODE of AP --> AP is present if IDCODE nonzero
        {
            myAccessPorts[APcount].apBASE = comArrayRead( &mainComArray, 9 ) & 0xFFFFF000;      // 32-bit: [31:12] contain BASEADDR[31:12]
            myAccessPorts[APcount].apBASE2 = comArrayRead( &mainComArray, 7 );
            myAccessPorts[APcount].apCFG = comArrayRead( &mainComArray, 8 );

            myAccessPorts[APcount].format = (comArrayRead( &mainComArray, 9 ) & 0x2) >> 1;
            myAccessPorts[APcount].present = (comArrayRead( &mainComArray, 9 ) & 0x1);

            myAccessPorts[APcount].type = myAccessPorts[APcount].apIDR & 0xF;   //** 0x0:jtag 0x1:AMBA AHB3 bus 0x2:AMBA APB2 or APB3 ... C2-229
            myAccessPorts[APcount].variant = ((myAccessPorts[APcount].apIDR & 0xF0) >> 4);
            myAccessPorts[APcount].res0 = ((myAccessPorts[APcount].apIDR & 0x1F00) >> 8);
            myAccessPorts[APcount].class = ((myAccessPorts[APcount].apIDR & 0x1E000) >> 13);        //** 0b1000 --> MEM-AP
            myAccessPorts[APcount].designer = ((myAccessPorts[APcount].apIDR & 0xFFE0000) >> 17);
            myAccessPorts[APcount].revision = ((myAccessPorts[APcount].apIDR & 0xF0000000) >> 28);
            APcount++;
        }
        else
        {
            done = true;
        }

    }
    // here it only prints once :)
    uint32_t dpIDCODE = comArrayRead( &mainComArray, 0 );

    //DP
    printf( "\nSW-DP IDR (IDCODE): 0x%08X\n", dpIDCODE );

    switch( dpIDCODE )
    {
        case 0x0BC11477:
            printf( "default Cortex-M0+ Core\n");
            break;
        case 0x2BA01477:
            printf( "Cortex-M4 Core w. FPU\n");
            break;

        default:
            printf( "Unknown Architecture\n");
            break;
    }

    printf( "\t--> DP revision/version (0x4=JTAG; 0x2=SW; ...): 0x%X\n", ((dpIDCODE & 0xF0000000) >> 28 ) );
    printf( "\t--> DP partNo (0xBA00=JTAG; 0xBA01=SW): 0x%X\n", ((dpIDCODE & 0x0FFFF000) >> 12) );

    printf( "\t--> DP designer: 0x%X\n", ((dpIDCODE & 0x0FFE) >> 1) );

    printf( "\t--> DP partNo (???): 0x%X\n", ((dpIDCODE & 0x0FF00000) >> 20) );
    printf( "\t--> DP version: 0x%X\n", ((dpIDCODE & 0xF000) >> 12) );
    //printf( "CTRLSTAT: 0x%08X\n", comArrayRead( &mainComArray, 2 ) );c




    //APs
    printf( "\tfound %d Access Port(s)!\n\n", APcount );

    for (size_t i = 0; i < APcount; i++)
    {
        if( myAccessPorts[i].class == 8 )
        {
            printf( "MEM-AP #%d (IDR=0x%08X, %s):\n", i, myAccessPorts[i].apIDR, ap_types[myAccessPorts[i].type] );
        }
        else if( myAccessPorts[i].class == 0 )
        {
            printf( "generic AP #%d (IDR=0x%08X, type=%d):\n", i, myAccessPorts[i].apIDR, myAccessPorts[i].type );
        }
        else
        {
            printf( "AP class error\n" );
            return -1;
        }

        extractAccessPort( &myAccessPorts[i] );
    }
    return 0;
}


void read_ids( int file )       //reads some registers
{
    comArrayClear( &mainComArray );
    comArrayAdd( &mainComArray, DP_IDCODE_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );
    comArrayAdd( &mainComArray, DP_CTRLSTAT_R_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_SELECT_CMD, 0xF0 );
    comArrayAdd( &mainComArray, MEMAP_READ2_CMD, 0x0 );
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_READBUF_CMD, 0x0 );

    comArrayTransfer( &mainComArray );

    //printf( "IDCODE: 0x%08X (0x%08X)\n", myReadBuffer[2*0 + 1], myReadBuffer[2*0]);     //IDCODE
    //printf( "MEM-AP info: \n");
    //printf( "DEBUG BASE: 0x%08X (0x%08X)\n", myReadBuffer[2*5 + 1], myReadBuffer[2*5]);
    //printf( "AP-IDR: 0x%08X (0x%08X)\n", myReadBuffer[2*6 + 1], myReadBuffer[2*6]);     //AP ID
}

//this could be implemented in the driver as some higher level CMD resulting in the whole memory read portion terminated by a 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
void read_mcu_id()
{

    comArrayClear( &mainComArray );
    comArrayAdd( &mainComArray, DP_IDCODE_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );
    comArrayAdd( &mainComArray, DP_CTRLSTAT_R_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_SELECT_CMD, 0x00 );
    comArrayAdd( &mainComArray, MEMAP_WRITE0_CMD, 0x22000012 );
    comArrayAdd( &mainComArray, MEMAP_WRITE1_CMD, 0x40015800 );     // STM32L0x3
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );

    comArrayTransfer( &mainComArray );

    printf( "MCU ID: 0x%08X\n", comArrayRead( &mainComArray, 7 ) );     //AP ID
}

void fileprint( char *path, int wordsToRead )
{
    //binary dump:
    printf( "\ndump binary file:\n" );

    FILE *binFilePtr = NULL;

    binFilePtr = fopen( path, "rb");
    if( binFilePtr == NULL )
    {
        printf( "file does not exist!\n" );
    }

    //read complete words only
    uint8_t byteBuffer[4];
    uint32_t *wordBuffer = (uint32_t*)byteBuffer;
    int freadReply;

    bool done = false;
    printf( "words to read: %d\n", wordsToRead);
    int wordsRead = 0;

    if( wordsToRead == 0 )
    {
        wordsToRead = -1;
    }

    printf("\n0x%08X: ", 0);
    while( !done )
    {
        freadReply = fread( byteBuffer, sizeof(byteBuffer), 1 , binFilePtr );

        if( freadReply == 1 )   //all fine
        {
            printf("%08X ", wordBuffer[0]);
        }
        else
        {
            done = true;
        }

        wordsRead++;

        if( (wordsRead % 4) == 0 )
        {
            printf("\n0x%08X: ", wordsRead*4);
        }

        if( wordsRead == wordsToRead )      //max given
        {
            done = true;
        }
    }
}


//https://developer.arm.com/documentation/ka001375/latest/
/*
IMPORTANT NOTE FROM ARM:
Automatic address increment is only guaranteed to operate on the bottom 10-bits of the address held in the TAR. Auto address incrementing of bit [10] and beyond is implementation defined. This means that auto address incrementing at a 1KB boundary is implementation defined. For example, if TAR[31:0] is set to 0x14A4, and the access size is word, successive accesses to the DRW increment TAR to 0x14A8, 0x14A, and so on, up to the end of the 1KB range at 0x17FC. At this point, the auto-increment behavior on the next DRW access is implementation defined.

BUT THIS SHOUDL BE FINE... *(it fits to 400 though... )

M1: Increments and wraps within a 1KB address boundary, for example, for word incrementing from 0x1400-0x17FC.
If the start is at 0x14A0, then the counter increments to 0x17FC, wraps to 0x1400, then continues incrementing to 0x149C.

M3: same but with 4KB boundary

apparently this wraps... somehow...

do we have to somehow reset the auto increment... ??? so strange!
*/



//base offset needs to be aligned to 128 (0x80) bytes (corresponds to 32 words which is the smallest mutliple that the device driver will transfer (when subtracting the initialisiation data))
// this ensures, that we stay within a region where the automatic address increase works
int stmReadAligned( uint32_t baseAddr, uint32_t wordCount, uint32_t *returnBuffer, uint32_t *debugBuffer )      //*baseOffset = ( *baseAddr % 0x80 );
{
    if ( (baseAddr % 0x80) != 0 )
    {
        printf( "stmFetchAligned() error: baseAddr not aligned.\n");
        return -11;
    }

    uint32_t *cmdArray32 = calloc( 39, 2*sizeof(uint32_t) );    //allocate command buffer for 32 + 7 commands: 7 for initialisiation and 32 for data
    uint32_t *replyArray32 = calloc( 39, 2*sizeof(uint32_t) );

    uint32_t addressCounter = 0;

    uint32_t wordsTransferred = 0;
    uint32_t commandWordsTransferred = 14;      //double of words
    uint32_t wordsStored = 0;

    int reply = 0;

    int SWDPIfile =  open("/dev/SWDPI", O_RDWR | O_SYNC);

    while( wordsTransferred < wordCount )      //loop until all words have been read!
    {
        //initialisiation every 39 (32 data) transfers --> we are certainly aligned with memory areas in which the auto address increase works
        //printf( "bunch:\n" );
        cmdArray32[0] = DP_IDCODE_CMD; cmdArray32[1] = 0;
        cmdArray32[2] = DP_CTRLSTAT_W_CMD; cmdArray32[3] = 0x50000000;
        cmdArray32[4] = DP_CTRLSTAT_R_CMD; cmdArray32[5] = 0;
        cmdArray32[6] = DP_SELECT_CMD; cmdArray32[7] = 0;                                   // selects AP 0, BANK 0
        cmdArray32[8] = MEMAP_WRITE0_CMD; cmdArray32[9] = 0x22000012;                       // sets access (auto increment etc)
        cmdArray32[10] = MEMAP_WRITE1_CMD; cmdArray32[11] = baseAddr + addressCounter * 4;  // set correct starting address
        cmdArray32[12] = MEMAP_READ3_CMD; cmdArray32[13] = 0;                               //does not get ACK??        //initial read (no reply expected)
        cmdArray32[14] = DP_READBUF_CMD; cmdArray32[15] = 0;
        commandWordsTransferred = 16;
        wordsTransferred++; //first word has been transferred already here
        //addressCounter gets me!! WE HAVE TO READ BUFFER, otherwise the address is automatically increased

        //fill in remaining read commands:
        while( (wordsTransferred < wordCount) && (commandWordsTransferred < 78) )   //aligns with 1024 bit boundary of the address increase... what if w e choose stupid offset???
        {
            cmdArray32[commandWordsTransferred] = MEMAP_READ3_CMD;
            commandWordsTransferred++;
            cmdArray32[commandWordsTransferred] = 0;       //we read
            commandWordsTransferred++;
            wordsTransferred++;
        }

        //send and receive
        reply = write( SWDPIfile, cmdArray32,   commandWordsTransferred*4 );             //in bytes. each commandsDone has 4 bytes
        reply = read( SWDPIfile,  replyArray32, commandWordsTransferred*4 );

        //printf("debug: commandWordsTransferred: %d\n", commandWordsTransferred );
        //extract the actual data only:
        for( int i = 14; i < commandWordsTransferred; i+=2 )    //increase by two
        {
            //printf("debug: i: %d\n", i );
            returnBuffer[wordsStored] = replyArray32[i+1];

            if( debugBuffer != NULL )
            {
                debugBuffer[wordsStored] = replyArray32[i];
            }

            wordsStored++;
            addressCounter++;
        }
    }
    free( cmdArray32 );
    free( replyArray32 );
    close( SWDPIfile );

    return 0;
}

void align2mem( uint32_t *baseAddr, uint32_t *wordCount, uint32_t *baseOffset )
{
	*baseOffset = (*baseAddr % 0x80 );
	*baseAddr -= *baseOffset;
	*wordCount += *baseOffset / 4;

	if( *baseOffset != 0 )
    {
        printf( "automatically shifted base address by -0x%08X\n", *baseOffset);
        printf( "increased wordCount by %d\n", (*baseOffset/4));
    }
}

void stmPrint( uint32_t baseAddr, uint32_t wordCount )
{
    printf( "stmPrint: base: 0x%x, words: %d\n", baseAddr, wordCount );

    uint32_t newWordCount = wordCount;
    uint32_t newBaseAddr = baseAddr;
    uint32_t baseOffset = 0;

    align2mem( &newBaseAddr, &newWordCount, &baseOffset );    // shifts base addr to new location: baseAddr - baseOffset and increases wordCount to account for the shift

    uint32_t wordOffset = baseOffset / 4;

    uint32_t *dataArray = calloc( newWordCount, sizeof(uint32_t) );      //64bits * 64 transfers
    uint32_t *debugArray = calloc( newWordCount, sizeof(uint32_t) );

    printf( "newBaseAddr: %d newWordCount: %d \n", newBaseAddr, newWordCount);
    uint32_t reply = stmReadAligned( newBaseAddr, newWordCount, dataArray, debugArray );
    printf( "reply: %d\n", reply);

    for (int i = wordOffset; i < newWordCount; i++)
    {
        if( ( i % 4 ) == 0 )
        {
            printf( "0x%08X: ", i*4 + newBaseAddr );
        }
        printf( "%08X (%08X) ", dataArray[i], debugArray[i] );          //display ACK
        //printf( "%08X ", dataArray[i] );                                   //do not display ACK

        if( ( ( i + 1 ) % 4) == 0 )
        {
            printf( "\n" );
        }
    }
}


void stmDump( uint32_t baseAddr, uint32_t wordCount )        //wordCount is the number of words that are supposed to be displayed
{
    printf( "stmDump: base: 0x%x, words: %d\n", baseAddr, wordCount );

    //need 6 commands for setup + wordcount reads + 1 (because read is delayed) = 7 + wordcount reads
    //maximum commands that SWDPI supports: 64 --> 64-7 is 57 words can be read from the MCU
    char filenamestr[128];
    printf( "please enter filename: " );
    scanf( "%127[^\n]", filenamestr );

    //check if file exists
    FILE *file = fopen( filenamestr, "r");
    if( file )
    {
        fclose(file);
        printf( "file already exists - abort!\n" );
        return 0;
    }

    //file does not exist (OR we cannot read it...)
    file = fopen( filenamestr, "wb");

    uint32_t newWordCount = wordCount;
    uint32_t newBaseAddr = baseAddr;
    uint32_t baseOffset = 0;
    align2mem( &newBaseAddr, &newWordCount, &baseOffset );    // shifts base addr to new location: baseAddr - baseOffset and increases wordCount to account for the shift

    uint32_t wordOffset = baseOffset / 4;

    uint32_t *dataArray = calloc( newWordCount, sizeof(uint32_t) );      //64bits * 64 transfers
    uint32_t *debugArray = calloc( newWordCount, sizeof(uint32_t) );

    printf( "newBaseAddr: %d newWordCount: %d \n", newBaseAddr, newWordCount);
    uint32_t reply = stmReadAligned( newBaseAddr, newWordCount, dataArray, debugArray );
    printf( "reply: %d\n", reply);

    for( int i = wordOffset; i < newWordCount; i++ )
    {
        //printf( "%08X (%08X) ", dataArray[i], debugArray[i] );
        fwrite( &dataArray[i], sizeof(dataArray[i]), 1, file);
    }

    free( dataArray );
    free( debugArray );
    fclose( file );
}

int main( int argc, char *argv[] )
{
    //printf("param1: %s\n", argv[1]);
    //printf("param2: %s\n", argv[2]);
    //printf("param3: %s\n", argv[3]);

    char *nullstr = "";
    char *argstr1 = nullstr;
    char *argstr2 = nullstr;
    char *argstr3 = nullstr;

    long int param2 = 0;
    long int param3  = 0;

    char *ptr1;
    char *ptr2;

    if( argv[1] != NULL )
    {
        argstr1 = argv[1];

        if( argv[2] != NULL )
        {
            argstr2 = argv[2];
            param2 = strtol( argv[2], &ptr1, 0 );

            if( argv[3] != NULL )   //if 2 == NULL --> 3 is meaningless
            {
                argstr3 = argv[3];
                param3 = strtol( argv[3], &ptr2, 0 );
            }
        }
    }

    // first collect some information
    detectSystem();
    read_mcu_id();

    //printf("didit! %s, %s, %d, %d\n", argstr2, argstr3, (int)param2, (int)param3);

    if( strcmp(argstr1, "") == 0 )
    {
        //int cake =  open("/dev/SWDPI", O_RDWR | O_SYNC);
        //read_ids( cake );
        //read_mcu_id( cake );
    }
    else if( strcmp(argv[1], "-fileprint") == 0 ) // -filedump filename #ofWords
    {
        printf( "-fileprint\n");
        fileprint( argstr2, param3 );
    }
    else if ( strcmp(argv[1], "-stmprint") == 0 ) // -stmdump base-addr #ofWords
    {
        //printf( "-stmprint\n");
        //stmprint( param2, param3 );
        stmPrint( param2, param3 );
    }
    else if ( strcmp(argv[1], "-stmdump") == 0 ) // -stmdump base-addr #ofWords
    {
        //printf( "-stmdump\n");
        stmDump( param2, param3 );
    }

    return 0;
}





//    https://qcentlabs.com/posts/swd_banger/

    //RASPI 4 with  STM32L053R8     cortex M0+  12bit 1.14 MSPS A/D
    //RASPI 5 with  STM32F401...    cortex-M4 w 12bit 2.4 MSPS A/D

    //ARM_IMPLEMENTER_ARM = 0x41,
    //CORTEX_M4_PARTNO     = ARM_MAKE_CPUID(ARM_IMPLEMENTER_ARM, 0xC24),
	//CORTEX_M0P_PARTNO    = ARM_MAKE_CPUID(ARM_IMPLEMENTER_ARM, 0xC60),

    //0x24770011 on devices with a Cortex-M3 or Cortex-M4 core
    //On devices with a Cortex-M0+ it should return 0x0477003

    //DATA is stored LSB ... but each byte is transmitted in order. To transmit a value of 0xFF00FF00, we have to write the value backwards here! 00FF00FF

    // IMPORTANT:
    // If you require the value from an AP register read, that read must be followed by one of:
    //      A second AP register read, with the appropriate AP selected as the current AP.
    //      A read of the DP Read Buffer.


    //idea:
    // 1. write 32 bit cmd + 32 bit data (or 0x0 for reads) into the file repeatedly
    // 2. read same amount of lines --> a) triggers communication and b) returns ack + data/0x0
    // CMD: 32bits, DATA: 32bitss
    // CMD: [7-0(cmd)], [15-8], [23-16], [31-24(ACK)],        DATA: [7-0], [15-8], [23-16], [31-24],               a bit of a weird ordering for byte arrays
/*    uint8_t test[128] = { DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,                    //low bytes send first...
                          DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_CTRLSTAT_W_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x50,   //bit 28 and 30  --> power up system & debug. seems to work    & seems to be necessary to read out MEMAP_READ3_CMD etc in the NEXT RUN
                          //DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_SELECT_CMD,        0x00, 0x00, 0x00,   0xF0,  0x00,   0x00,    0x00,   // [7...4]   APBANKSEL  --> selects active 4 word register bank on current AP...
                          //DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //MEMAP_WRITE0_CMD,     0x00, 0x00, 0x00,   0x12,  0x00,   0x00,    0x22,
                          //DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00, // [31-24]   APSEL --> should be 0x00 for single AP
// single read after select is not sufficient!!!!

                          MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //actually reads the AP ID

                          DP_SELECT_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,

                          MEMAP_WRITE0_CMD,     0x00, 0x00, 0x00,   0x12,  0x00,   0x00,    0x22,
                          MEMAP_WRITE0_CMD,     0x00, 0x00, 0x00,   0x12,  0x00,   0x00,    0x22,
                          MEMAP_READ0_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,

                          //MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,

                          //MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //DP_SELECT_CMD,        0x00, 0x00, 0x00,   0xF0,  0x00,   0x00,    0x00,
                        //  MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,   //only works if DP_CTRLSTAT_W_CMD was written as above once before...
                          //MEMAP_READ0_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //DP_READBUF_CMD,     0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //DP_READRE_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //DP_READBUF_CMD,     0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00

                      };      //how do I select the access port...*/

/*    uint8_t commandCount = 11;

    //uint64_t test = 0xFFFFFFFFFFFFFFFE; //0xA5 << 56; //shift all to the left (left 8 bits are for direct commands)
    int reply = 0;
    uint32_t myReadBuffer[32];

    reply = write( cake, test, commandCount*8 );                //write list of commands
    printf( "write response: %d\n", reply );

    reply = read(cake, &myReadBuffer, commandCount*8);          //read 4 bytes -->> always reads IDCODE
    printf( "read response: %d\n", reply );
    if ( reply < 0)
    {
        printf( "read error response: %d\n", reply );
    }
    else
    {
        for( size_t i = 0; i < commandCount; i++ )
        {
            uint8_t ack = ((uint8_t*)myReadBuffer)[i*8 + 3];
            uint8_t cmd = ((uint8_t*)myReadBuffer)[i*8 + 0];
            uint32_t data = ((uint32_t*)myReadBuffer)[2*i + 1];
            printf( "test transfer - cmd: 0x%x ack: 0x%x data: 0x%x\n", cmd, ack, data );
        }
    }*/
