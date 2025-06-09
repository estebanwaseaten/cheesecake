// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <stdint.h>     //uint32_t etc
#include <stdbool.h>



#ifndef CHEESE_COMSWD_H
#define CHEESE_COMSWD_H

#define MAX_COMS 64

//for SW-DB comms
typedef struct comArray
{
    uint32_t *cmdArray32;           //commands 32-bit for commands, 32-bit for data (total 64bits per transfer)
    uint32_t *replyArray32;         //responses
    uint8_t  initDone;              //if allocations have been performed
    uint32_t filling;               //how many commands are stored right now
} comArray;


//not using the comArray:
int com_transferSequence( uint32_t *sequence );     ///needs to be in multiples of 64bits though

//using the comArray
void comArrayInit( comArray *myComArray );
int comArrayClear( comArray *myComArray );
int comArrayAdd( comArray *myComArray, uint32_t cmd, uint32_t val );

int comArray_prepAPaccess( comArray *myComArray, uint8_t accessPort, uint8_t accessPortBank );
int comArray_prepMemAccess( comArray *myComArray, uint8_t accessPort, uint32_t memBase );

uint32_t comArrayRead( comArray *myComArray, uint32_t index );       //index starts at 0
int comArrayTransfer( comArray *myComArray );

int comArray_getSWDerr();
void com_writeWord( uint32_t addr, uint32_t word );
uint32_t com_readWord( uint32_t addr );

#endif
