// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <stdio.h>
#include "cheese_utils.h"


void color_default()
{
    printf("\033[0m");
}

void color_red()
{
    printf("\033[1;31m");
}

void color_green()
{
    printf("\033[0;32m");
}

void color_blue()
{
    printf("\033[0;34m");
}

void color_white()
{
    printf("\033[0;37m");
}
