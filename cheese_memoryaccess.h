// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <stdint.h>     //uint32_t etc
#include <stdbool.h>



#ifndef CHEESE_MEMORYACCESS_H
#define CHEESE_MEMORYACCESS_H


int stmReadAligned( uint32_t baseAddr, uint32_t wordCount, uint32_t *returnBuffer, uint32_t *debugBuffer );      //*baseOffset = ( *baseAddr % 0x80 );
void align2mem( uint32_t *baseAddr, uint32_t *wordCount, uint32_t *baseOffset );
int stmPrint( uint32_t baseAddr, uint32_t wordCount );
int stmDump( uint32_t baseAddr, uint32_t wordCount );        //wordCount is the number of words that are supposed to be displayed
void fileprint( char *path, int wordsToRead );













#endif
