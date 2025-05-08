//#include <iostream>
#include <stdint.h>
#include <fcntl.h>      // O_RDWR | O_SYNC etc
#include <unistd.h>     //close()
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "SWDPI_base.h"
#include "registers.h"

//for comparing:
// hexyl
// xxd -
// vbindiff

#define MAX_COMS 64

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
} APinfo;

//information for debug component
typedef struct DCinfo
{
    uint32_t CIDR[4];
    uint32_t PIDR[8];
    uint32_t class;
    uint32_t memType;

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

void comArrayTransfer( comArray *myComArray )
{
    int SWDPIfile = open("/dev/SWDPI", O_RDWR | O_SYNC);
    write( SWDPIfile, myComArray->cmdArray32, myComArray->filling*2*4 );             //in bytes. each commandsDone has 4 bytes
    read( SWDPIfile, myComArray->replyArray32,  myComArray->filling*2*4 );
    close( SWDPIfile );
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

void extractComponentInfo( uint32_t base, DCinfo *componentInfo )
{
    // IHI0074E_debug_interface_v6_0_architecture_specification.pdf
    comArrayClear( &mainComArray );

    comArrayAdd( &mainComArray, DP_IDCODE_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );
    comArrayAdd( &mainComArray, DP_CTRLSTAT_R_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_SELECT_CMD, 0x00 );

    comArrayAdd( &mainComArray, MEMAP_WRITE0_CMD, 0x22000012 );
    comArrayAdd( &mainComArray, MEMAP_WRITE1_CMD, base + 0x0FCC );                  //offset for fixed registers
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFCC                  // ... memory type register
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0xFD0 (PIDR4)          // ... memory type register (delayed access)
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

    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );

    comArrayTransfer( &mainComArray );

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

    componentInfo->class = componentInfo->CIDR[1] >> 4; //*** 0x0=generic, 0x1=ROM Table, 0x9=CoreSightComponent, 0xB=peripheralTestBlock, 0xE=genericIPComponent, 0xF=CoreLink or PrimeCell
//    uint32_t partNo = regPIDR0 | ((regPIDR1 & 0xF) << 8);
//    uint32_t JEP106id = ((regPIDR1 & 0xF0) >> 4) | ((regPIDR2 & 0x7) << 4);
//    uint32_t JEP106cont = regPIDR4 & 0xF;


}

void extractComponent( uint32_t base )
{
    DCinfo thisComponentInfo;
    comArray localComArray;

    printf( "ExtractComponent at 0x%X:\n", base );

    comArrayInit( &localComArray );

    extractComponentInfo( base, &thisComponentInfo );
    printf( "This component has class 0x%X and memory type 0x%X\n", thisComponentInfo.class, thisComponentInfo.memType );
    // memory Type = 0b1 if system memory is present on the bus (deprecated)

    uint32_t romAddresses[32];
    int romAddrCount = 0;

    if( thisComponentInfo.class == 0x1 )    //ROM table
    {
        comArrayClear( &localComArray );

        comArrayAdd( &localComArray, DP_IDCODE_CMD, 0x0 );
        comArrayAdd( &localComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );
        comArrayAdd( &localComArray, DP_CTRLSTAT_R_CMD, 0x0 );
        comArrayAdd( &localComArray, DP_SELECT_CMD, 0x00 );
        comArrayAdd( &localComArray, MEMAP_WRITE0_CMD, 0x22000012 );
        comArrayAdd( &localComArray, MEMAP_WRITE1_CMD, base);
        comArrayAdd( &localComArray, MEMAP_READ3_CMD, 0x0 );   // 0x000

        for( int i = 0; i < 32; i++ )
        {
            comArrayAdd( &localComArray, MEMAP_READ3_CMD, 0x0 );
        }
        comArrayTransfer( &localComArray );

        printf( "ROM table:\n" );
        for( int i = 0; i < 32; i++ )
        {
            romAddresses[i] = comArrayRead( &localComArray, 7 + i );
            if( romAddresses[i] == 0x0 )
            {
                i = 32;
            }
            else
            {
                if( (romAddresses[i] & 0xFFFFFFFC) != base )    //avoid infinite loop
                {
                    printf( "\t 0x%08X\n", romAddresses[i] & 0xFFFFFFFC );
                //    extractComponent( romAddresses[i] & 0xFFFFFFFC );                       //this somehow crashes the MCU when romAddresses[i] == 0x00200000
                //    printf("\n");
                }
                romAddrCount++;
            }
        }
        printf( "(found %d entries in ROM table!)\n\n", romAddrCount );
    }
}



int extractAccessPort( APinfo *currentAP )
{
    printf( "\tdebugBase: 0x%08X (BASE2: 0x%08X)\n", currentAP->apBASE, currentAP->apBASE2 );
    printf( "\tAP CFG: %d (little endian = 0; big endian = 1)\n\n", currentAP->apCFG );

    extractComponent( currentAP->apBASE );

    extractComponent( 0xF00FF000 ); //same as before???
    //extractComponent( 0x00200000 ); // THIS crashes the MCU???


    return 0;
}
/*
    //look at CIDR0 - CIDR3 located at apBASE + 0xFF0 + n*4


    // IHI0074E_debug_interface_v6_0_architecture_specification.pdf
    // IHI0029F_coresight_v3_0_architecture_specification.pdf

    // more ROM table:
    comArrayClear( &mainComArray );

    comArrayAdd( &mainComArray, DP_IDCODE_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );
    comArrayAdd( &mainComArray, DP_CTRLSTAT_R_CMD, 0x0 );
    comArrayAdd( &mainComArray, DP_SELECT_CMD, 0x00 );

    comArrayAdd( &mainComArray, MEMAP_WRITE0_CMD, 0x22000012 );
    comArrayAdd( &mainComArray, MEMAP_WRITE1_CMD, apBASE);
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x000
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x004 (0x000)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x008 (0x004)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x00C (0x008)
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x010 (0x00C)

    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x010
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x014
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x018
    comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x01C

    comArrayTransfer( &mainComArray );

    uint32_t romRegs[6];

    romRegs[0] = comArrayRead( &mainComArray, 7 );
    romRegs[1] = comArrayRead( &mainComArray, 8 );
    romRegs[2] = comArrayRead( &mainComArray, 9 );
    romRegs[3] = comArrayRead( &mainComArray, 10 );

    romRegs[4] = comArrayRead( &mainComArray, 11 );
    romRegs[5] = comArrayRead( &mainComArray, 12 );

    printf( "The ROM table points to components (via their address):\n");   //IHI0074E_debug_interface_v6_0_architecture_specification.pdf
    for (size_t i = 0; i < 6; i++) {
        printf( "ROM reg %d: 0x%08X; offset: 0x%08X; powerid: %04X; poweridvalid: %d; format: %d; present: %d\n", i, romRegs[i], ((romRegs[i] & 0xFFFFF000) >> 12), ((romRegs[i] & 0x1F0) >> 4), ((romRegs[i] & 0x4) >> 2),  ((romRegs[i] & 0x2) >> 1), (romRegs[i] & 0x1) );
    }

    printf( "ROM 0x%04X: 0x%08X (final entry is all zero)\n", 0x018, comArrayRead( &mainComArray, 13 ) );
    //printf( "ROM 0x%04X: 0x%08X\n", 0x01C, comArrayRead( &mainComArray, 14 ) );

    for (size_t i = 0; i < 6; i++)
    {
        printf( "\nComponent #%d:\n", i );

        // more ROM table:
        comArrayClear( &mainComArray );

        comArrayAdd( &mainComArray, DP_IDCODE_CMD, 0x0 );
        comArrayAdd( &mainComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );
        comArrayAdd( &mainComArray, DP_CTRLSTAT_R_CMD, 0x0 );
        comArrayAdd( &mainComArray, DP_SELECT_CMD, 0x00 );

        comArrayAdd( &mainComArray, MEMAP_WRITE0_CMD, 0x22000012 );
        comArrayAdd( &mainComArray, MEMAP_WRITE1_CMD, apBASE + (romRegs[i] & 0xFFFFF000));
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x000
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x004 (0x000)
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x008 (0x004)
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x00C (0x008)
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );   // 0x010 (0x00C)

        comArrayAdd( &mainComArray, MEMAP_WRITE1_CMD, apBASE + (romRegs[i] & 0xFFFFF000) + 0xFD0);      //component and peripheral ID registers

        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );     //cidr1
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );

        comArrayTransfer( &mainComArray );
        printf( "Component registers:\n" );
        printf("0x000: 0x%08X\n", comArrayRead( &mainComArray, 7 ));
        printf("0x004: 0x%08X\n", comArrayRead( &mainComArray, 8 ));
        printf("0x008: 0x%08X\n", comArrayRead( &mainComArray, 9 ));
        printf("0x00C: 0x%08X\n", comArrayRead( &mainComArray, 10 ));
        printf( "Component and peripheral ID registers:\n" );
        printf("0x000: 0x%08X\n", comArrayRead( &mainComArray, 13 ));
        printf("0x004: 0x%08X\n", comArrayRead( &mainComArray, 14 ));
        printf("0x008: 0x%08X\n", comArrayRead( &mainComArray, 15 ));
        printf("0x00C: 0x%08X\n", comArrayRead( &mainComArray, 16 ));
        printf("0x000: 0x%08X\n", comArrayRead( &mainComArray, 17 ));
        printf("0x004: 0x%08X\n", comArrayRead( &mainComArray, 18 ));
        printf("0x008: 0x%08X\n", comArrayRead( &mainComArray, 19 ));
        printf("0x00C: 0x%08X\n", comArrayRead( &mainComArray, 20 ));
        printf("0x000: 0x%08X\n", comArrayRead( &mainComArray, 21 ));
        printf("0x004: 0x%08X\n", comArrayRead( &mainComArray, 22 ));    //CIDR1??
        printf("0x008: 0x%08X\n", comArrayRead( &mainComArray, 23 ));
        printf("0x00C: 0x%08X\n", comArrayRead( &mainComArray, 24 ));

        printf("class: 0x%08X\n", ((comArrayRead( &mainComArray, 22 ) & 0xF0) >> 4) );
    }



    return ((dpIDCODE & (15 << 12)) >> 12);*/


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
        comArrayAdd( &mainComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );        //power up requests [30], Debug power-up request [28], Debug reset request [26]. [30,28] = 0x50000000; [30,28,26] = 0x54000000
        comArrayAdd( &mainComArray, DP_CTRLSTAT_R_CMD, 0x0 );
        comArrayAdd( &mainComArray, DP_SELECT_CMD, (APcount << 24) | 0xF0 );                  //selects AP [31:24] = 0x0, bank [7:4] = 0xF

        comArrayAdd( &mainComArray, MEMAP_READ0_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ1_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ2_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );
        comArrayAdd( &mainComArray, MEMAP_READ3_CMD, 0x0 );

        comArrayTransfer( &mainComArray );

        myAccessPorts[APcount].apIDR = comArrayRead( &mainComArray, 8 );    //** revision[31:28] designer[27:17] class[16:13] variant[7:4] type[3 0]

        if( myAccessPorts[APcount].apIDR != 0 ) //check IDCODE of AP --> AP is present if IDCODE nonzero
        {
            myAccessPorts[APcount].apBASE = comArrayRead( &mainComArray, 7 ) & 0xFFFFFFFC;
            myAccessPorts[APcount].apBASE2 = comArrayRead( &mainComArray, 5 );
            myAccessPorts[APcount].apCFG = comArrayRead( &mainComArray, 6 );

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
    uint32_t dpIDCODE = comArrayRead( &mainComArray, 0 );

    //DP
    printf( "DP IDR (IDCODE): 0x%08X\n", dpIDCODE );
    printf( "\t--> version: %d\n", ((dpIDCODE & (15 << 12)) >> 12) );
    //printf( "CTRLSTAT: 0x%08X\n", comArrayRead( &mainComArray, 2 ) );

    //APs
    printf( "found %d Access Port(s)!\n\n", APcount );

    for (size_t i = 0; i < APcount; i++)
    {
        if( myAccessPorts[i].class == 8 )
        {
            printf( "MEM-AP #%d (IDR=0x%08X, type=%d):\n", i, myAccessPorts[i].apIDR, myAccessPorts[i].type );
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
    uint8_t cmdArray[7*8] = {
    // CMD: [7-0(cmd)], [15-8], [23-16], [31-24(ACK)],        DATA: [7-0], [15-8], [23-16], [31-24],
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //low bytes send first...
                          DP_CTRLSTAT_W_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x50,       //bit 28 and 30  --> power up system & debug. seems to work    & seems to be necessary to read out MEMAP_READ3_CMD etc in the NEXT RUN
                          DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_SELECT_CMD,        0x00, 0x00, 0x00,   0xF0,  0x00,   0x00,    0x00,       // [7...4]   APBANKSEL  --> selects active 4 word register bank on current AP...

                          MEMAP_READ2_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //read
                          MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       // read MEM-AP
                          DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //actually reads the AP ID
                      };

    int32_t myReadBuffer[7*2] = {0,};   //8 bytes per command --> 6*8=48 bytes is 12 32bit words
    int reply = 0;

    reply = write( file, cmdArray, 7*8 );                //write list of commands
    //printf( "write response: %d\n", reply );

    reply = read( file, &myReadBuffer, 7*8);   //read 4 bytes -->> always reads IDCODE
    //printf( "read response: %d\n", reply );

    //printf( "IDCODE: 0x%08X (0x%08X)\n", myReadBuffer[2*0 + 1], myReadBuffer[2*0]);     //IDCODE
    printf( "MEM-AP info: \n");
    printf( "DEBUG BASE: 0x%08X (0x%08X)\n", myReadBuffer[2*5 + 1], myReadBuffer[2*5]);
    printf( "AP-IDR: 0x%08X (0x%08X)\n", myReadBuffer[2*6 + 1], myReadBuffer[2*6]);     //AP ID
}

//this could be implemented in the driver as some higher level CMD resulting in the whole memory read portion terminated by a 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
void read_mcu_id( int file )
{
    uint8_t cmdArray[8*8] = {
    // CMD: [7-0(cmd)], [15-8], [23-16], [31-24(ACK)],        DATA: [7-0], [15-8], [23-16], [31-24],
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //low bytes send first...
                          DP_CTRLSTAT_W_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x54,       //bit 26, 28 and 30  --> power up system & debug. seems to work    & seems to be necessary to read out MEMAP_READ3_CMD etc in the NEXT RUN
                          DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       // 0x50 or 0x54
                          DP_SELECT_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       // [31...24] APSEL selects AP; [7...4] APBANKSEL selects active 4 word register bank on current AP...
//is his always correct:
                          MEMAP_WRITE0_CMD,     0x00, 0x00, 0x00,   0x12,  0x00,   0x00,    0x22,       //cmdArray32[8] = MEMAP_WRITE0_CMD; cmdArray32[9] = 0x22000012;
                          // write address:
                          MEMAP_WRITE1_CMD,     0x00, 0x00, 0x00,   0x00,  0x58,   0x01,    0x40,       // --> addr 0x40015800 is address of MCU device ID (maybe) maybe this is not universal...
                          //MEMAP_WRITE1_CMD,     0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x08,
// preivous seems to fail...
                          MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
// here we get a wait cmd
                          MEMAP_READ3_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,

                      };

      int32_t myReadBuffer[8*2] = {0, };   //8 bytes per command --> 6*8=48 bytes is 12 32bit words
      int reply = 0;

      reply = write( file, cmdArray, 8*8 );                //write list of commands
     // printf( "write response: %d\n", reply );

      reply = read( file, &myReadBuffer, 8*8);
      //printf( "read response: %d\n", reply );

      printf( "MCU ID: 0x%08X (0x%08X)\n", myReadBuffer[2*7 + 1], myReadBuffer[2*7]);     //AP ID
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
