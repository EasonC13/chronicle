/*
 * Copyright (c) 2019 IOTA Stiftung
 * Copyright (c) 2019 Cybercrypt A/S
 * https://github.com/iotaledger/entangled
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "common/crypto/troika/troika.h"

#define COLUMNS 9
#define ROWS 3
#define SLICES 27
#define SLICESIZE COLUMNS *ROWS
#define STATESIZE COLUMNS *ROWS *SLICES
#define NUM_SBOXES SLICES *ROWS *COLUMNS / 3

#define PADDING 0x1

static const trit_t round_constants[NUM_ROUNDS][COLUMNS * SLICES] = {
    {2, 2, 2, 2, 1, 2, 0, 1, 0, 1, 1, 0, 2, 0, 1, 0, 1, 1, 0, 0, 1, 2, 1, 1, 1, 0, 0, 2, 0, 2, 1, 0, 2, 2, 2,
     1, 0, 2, 2, 0, 0, 1, 2, 2, 1, 0, 1, 0, 1, 2, 2, 2, 0, 1, 2, 2, 1, 1, 2, 1, 1, 2, 0, 2, 0, 2, 0, 0, 0, 0,
     2, 1, 1, 2, 1, 0, 1, 0, 2, 1, 1, 0, 0, 2, 2, 2, 2, 0, 1, 1, 2, 1, 2, 2, 0, 1, 2, 2, 2, 0, 1, 0, 2, 2, 0,
     2, 1, 1, 2, 1, 2, 1, 0, 0, 2, 1, 0, 0, 1, 2, 2, 1, 1, 1, 0, 1, 0, 2, 2, 0, 2, 2, 2, 0, 2, 2, 1, 0, 0, 0,
     2, 1, 0, 0, 1, 1, 1, 2, 2, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 2, 2, 1, 0, 1, 0, 2, 0, 1, 2, 0, 1, 2, 2, 2, 2,
     1, 0, 0, 0, 0, 2, 1, 0, 2, 1, 1, 2, 0, 2, 1, 0, 0, 0, 1, 0, 2, 1, 2, 0, 1, 2, 1, 0, 2, 0, 2, 1, 0, 0, 1,
     2, 0, 2, 2, 2, 0, 1, 0, 2, 0, 1, 0, 2, 1, 2, 1, 2, 2, 1, 1, 2, 0, 2, 2, 1, 0, 0, 2, 0, 2, 1, 0, 1},
    {1, 1, 1, 0, 2, 2, 0, 2, 0, 1, 0, 2, 1, 1, 0, 0, 1, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 1, 2, 0, 2, 2, 2, 0, 2,
     1, 1, 2, 1, 0, 2, 1, 0, 2, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 2, 0, 0, 0, 2,
     1, 1, 0, 1, 2, 0, 1, 2, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 2, 2, 0, 2, 0, 2, 1, 0, 2, 1, 0, 0, 1, 2, 2, 0, 0,
     0, 0, 1, 0, 2, 2, 2, 1, 1, 0, 1, 0, 2, 1, 2, 2, 2, 1, 0, 2, 2, 0, 2, 0, 1, 2, 1, 0, 1, 0, 0, 1, 1, 0, 1,
     2, 2, 2, 0, 0, 1, 0, 0, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2, 1, 0, 2, 1, 2, 2, 1, 2, 1, 0, 0, 0, 2, 0, 0, 0, 2,
     2, 1, 2, 2, 0, 0, 1, 2, 2, 1, 0, 0, 2, 1, 2, 2, 2, 0, 1, 1, 1, 1, 2, 0, 1, 1, 2, 2, 1, 0, 1, 2, 0, 2, 2,
     1, 0, 1, 2, 1, 0, 1, 0, 1, 1, 2, 1, 1, 2, 2, 2, 1, 0, 2, 0, 0, 0, 1, 1, 2, 1, 0, 2, 0, 1, 1, 1, 2},
    {0, 2, 0, 2, 1, 2, 1, 1, 2, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 2,
     2, 0, 1, 0, 2, 2, 1, 2, 2, 2, 2, 1, 2, 1, 1, 0, 0, 1, 0, 2, 0, 2, 0, 1, 2, 0, 0, 2, 2, 2, 1, 1, 0, 0, 2,
     0, 2, 2, 2, 2, 1, 2, 1, 0, 2, 0, 2, 0, 2, 0, 2, 2, 0, 2, 2, 1, 2, 1, 2, 0, 0, 0, 0, 1, 0, 2, 1, 1, 2, 1,
     0, 1, 0, 2, 0, 1, 0, 0, 2, 2, 2, 2, 2, 1, 1, 0, 1, 2, 2, 0, 0, 2, 2, 1, 0, 1, 2, 2, 1, 0, 2, 1, 1, 2, 1,
     2, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 2, 0, 0, 1, 0, 0, 1, 0, 2, 1, 0, 2, 0,
     0, 0, 0, 2, 0, 0, 0, 2, 0, 1, 0, 2, 0, 0, 1, 2, 0, 1, 1, 2, 2, 0, 2, 2, 0, 0, 2, 2, 1, 2, 0, 0, 0, 1, 0,
     2, 1, 0, 1, 2, 1, 1, 0, 2, 0, 0, 2, 1, 1, 0, 1, 1, 2, 0, 0, 1, 1, 1, 0, 0, 2, 2, 2, 2, 1, 1, 2, 2},
    {1, 2, 2, 0, 2, 2, 0, 1, 0, 0, 0, 2, 0, 0, 0, 2, 1, 0, 2, 2, 0, 0, 1, 2, 1, 0, 0, 1, 0, 1, 2, 2, 1, 2, 1,
     0, 0, 1, 1, 2, 0, 0, 2, 2, 1, 0, 1, 2, 2, 2, 0, 2, 1, 1, 2, 1, 2, 1, 1, 0, 2, 1, 0, 0, 1, 2, 0, 1, 1, 0,
     0, 1, 0, 2, 0, 0, 2, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 1, 0, 0, 2, 0, 1, 1, 2, 1, 0, 1, 1, 0, 1, 2, 2, 0, 2,
     2, 0, 0, 0, 1, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 0, 0, 0, 0, 0, 1, 1, 1,
     1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 2, 2, 0, 1, 1, 2, 2, 2, 2, 2, 0, 2, 2, 2, 1, 1, 0, 0, 1, 0, 0, 2, 2, 2,
     1, 2, 0, 0, 0, 1, 2, 2, 2, 0, 1, 2, 1, 1, 2, 2, 1, 1, 2, 0, 1, 0, 0, 1, 0, 2, 0, 2, 0, 1, 0, 0, 0, 2, 2,
     2, 1, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 0, 0, 0, 0, 0, 2, 2, 0, 2, 2, 1, 0, 0, 2, 2, 0, 0},
    {0, 1, 1, 1, 1, 2, 0, 1, 1, 1, 1, 1, 0, 1, 2, 0, 2, 1, 0, 0, 2, 0, 1, 0, 1, 2, 0, 1, 1, 0, 1, 1, 1, 1, 0,
     0, 2, 0, 0, 0, 1, 0, 2, 2, 1, 0, 0, 1, 1, 0, 2, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 2, 0, 2, 0, 2, 1, 2, 1, 1,
     0, 1, 1, 2, 2, 2, 2, 0, 1, 0, 0, 2, 1, 1, 0, 1, 2, 1, 0, 1, 1, 1, 1, 0, 1, 1, 2, 2, 0, 1, 0, 2, 0, 0, 2,
     1, 2, 2, 1, 2, 2, 0, 0, 1, 2, 0, 0, 0, 0, 2, 1, 2, 2, 0, 2, 1, 0, 2, 1, 2, 0, 2, 0, 2, 0, 0, 0, 2, 1, 1,
     1, 2, 1, 0, 1, 2, 1, 1, 2, 1, 0, 2, 2, 1, 1, 0, 0, 0, 2, 0, 1, 1, 2, 1, 2, 2, 2, 0, 1, 2, 2, 0, 1, 0, 1,
     1, 2, 0, 2, 2, 2, 1, 1, 0, 2, 2, 1, 0, 2, 2, 2, 1, 1, 1, 1, 1, 0, 1, 2, 2, 2, 2, 0, 0, 2, 0, 1, 2, 1, 0,
     1, 1, 0, 0, 1, 0, 1, 2, 2, 0, 0, 2, 0, 0, 1, 1, 2, 2, 1, 0, 0, 0, 2, 1, 2, 1, 0, 1, 1, 2, 2, 1, 2},
    {2, 2, 2, 0, 2, 1, 0, 0, 0, 1, 0, 1, 0, 2, 0, 1, 2, 1, 0, 1, 2, 2, 2, 1, 0, 1, 2, 2, 1, 2, 1, 2, 1, 2, 1,
     2, 0, 0, 2, 1, 2, 1, 2, 1, 1, 2, 0, 1, 2, 2, 1, 2, 0, 0, 2, 0, 0, 2, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 2, 2,
     0, 2, 1, 0, 0, 0, 2, 2, 0, 0, 2, 0, 1, 2, 2, 2, 0, 1, 0, 0, 1, 0, 2, 1, 1, 2, 1, 0, 0, 0, 2, 0, 1, 0, 0,
     2, 1, 2, 2, 0, 1, 1, 0, 1, 1, 2, 0, 2, 2, 2, 0, 0, 0, 2, 2, 1, 0, 2, 1, 1, 1, 2, 2, 1, 1, 0, 0, 1, 2, 1,
     1, 0, 2, 1, 2, 0, 2, 1, 0, 1, 1, 0, 2, 1, 1, 2, 0, 2, 0, 0, 1, 0, 1, 0, 2, 1, 1, 0, 2, 0, 1, 2, 2, 0, 1,
     1, 1, 2, 1, 0, 2, 2, 2, 2, 1, 1, 0, 2, 0, 1, 2, 0, 2, 2, 1, 1, 2, 1, 0, 0, 1, 0, 1, 2, 0, 0, 2, 1, 0, 0,
     1, 1, 0, 2, 0, 1, 0, 1, 0, 1, 0, 1, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1, 0, 2, 1, 0, 1, 2, 0, 2, 2, 1, 0},
    {0, 2, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 2, 2, 1, 0, 1, 0, 0, 2, 1, 1, 1, 1, 2, 2, 0, 1, 1, 1, 2, 0, 1, 1, 2,
     2, 2, 1, 1, 2, 0, 2, 2, 1, 1, 2, 2, 0, 2, 1, 0, 1, 2, 0, 1, 2, 0, 2, 0, 2, 1, 0, 0, 0, 0, 1, 1, 2, 2, 0,
     1, 2, 0, 1, 1, 2, 1, 2, 2, 0, 0, 0, 2, 2, 0, 1, 0, 2, 1, 1, 2, 2, 0, 2, 1, 2, 0, 1, 0, 1, 2, 0, 2, 2, 1,
     0, 1, 1, 1, 0, 1, 0, 1, 1, 2, 0, 1, 2, 0, 2, 1, 0, 2, 2, 0, 0, 0, 1, 2, 0, 0, 1, 0, 1, 1, 1, 2, 0, 2, 2,
     0, 1, 0, 1, 1, 2, 1, 0, 0, 2, 1, 1, 0, 2, 0, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 2, 0, 0, 0, 1, 1, 1, 2, 0,
     1, 2, 1, 1, 1, 1, 1, 2, 0, 0, 1, 0, 2, 0, 0, 1, 2, 2, 2, 0, 2, 2, 0, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 2, 2,
     2, 1, 2, 2, 0, 0, 2, 2, 2, 2, 0, 0, 2, 1, 0, 2, 2, 0, 1, 1, 0, 1, 0, 0, 1, 0, 2, 2, 0, 0, 2, 0, 0},
    {0, 2, 1, 0, 1, 0, 0, 0, 1, 2, 1, 0, 2, 2, 0, 2, 1, 2, 1, 2, 0, 1, 0, 0, 2, 2, 2, 1, 1, 0, 1, 0, 1, 2, 2,
     2, 2, 1, 0, 2, 1, 1, 2, 1, 0, 2, 1, 0, 0, 1, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 2, 1, 0,
     0, 0, 1, 0, 2, 1, 1, 0, 1, 2, 1, 0, 2, 0, 1, 1, 0, 1, 1, 2, 0, 2, 1, 2, 0, 0, 0, 2, 2, 1, 2, 2, 1, 2, 1,
     2, 2, 1, 0, 0, 0, 0, 2, 1, 0, 0, 1, 1, 2, 0, 2, 1, 0, 1, 0, 1, 2, 2, 1, 2, 0, 2, 2, 1, 1, 2, 0, 0, 1, 1,
     0, 1, 2, 0, 2, 2, 2, 1, 0, 0, 1, 0, 1, 0, 2, 2, 1, 1, 0, 0, 1, 2, 2, 1, 1, 2, 1, 2, 0, 2, 2, 0, 2, 0, 0,
     1, 1, 1, 0, 0, 0, 1, 0, 2, 1, 1, 2, 2, 2, 1, 0, 2, 0, 1, 0, 1, 1, 2, 1, 0, 2, 1, 1, 1, 0, 2, 0, 2, 0, 0,
     1, 2, 2, 1, 2, 2, 1, 0, 2, 2, 2, 0, 0, 0, 0, 1, 0, 1, 2, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 2, 2, 0, 2},
    {1, 0, 1, 2, 1, 1, 0, 0, 2, 0, 2, 1, 1, 0, 1, 2, 1, 0, 2, 2, 1, 1, 0, 1, 1, 2, 0, 1, 1, 2, 1, 0, 0, 2, 2,
     0, 2, 2, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 2, 1, 0, 2, 2, 1, 0, 0, 2, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
     1, 0, 0, 0, 2, 0, 2, 1, 0, 0, 2, 2, 2, 0, 2, 2, 0, 1, 1, 2, 2, 1, 0, 0, 0, 2, 2, 2, 1, 0, 1, 1, 2, 2, 2,
     2, 2, 1, 2, 0, 2, 1, 1, 0, 0, 2, 0, 1, 1, 2, 1, 1, 2, 1, 0, 1, 2, 2, 0, 0, 0, 0, 2, 2, 1, 2, 2, 1, 1, 0,
     2, 2, 1, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 2, 0, 0, 0, 1, 1, 0, 2, 0, 2, 2, 1, 1, 1, 0, 1, 2, 2,
     0, 1, 2, 2, 2, 0, 1, 2, 2, 2, 0, 2, 1, 1, 2, 0, 2, 1, 1, 0, 2, 1, 0, 2, 1, 2, 1, 1, 1, 0, 0, 0, 0, 2, 2,
     0, 2, 2, 2, 2, 0, 2, 2, 0, 0, 0, 2, 0, 1, 0, 0, 0, 1, 1, 2, 0, 1, 1, 0, 2, 1, 1, 2, 2, 0, 2, 0, 1},
    {0, 1, 0, 1, 2, 0, 1, 1, 1, 1, 2, 1, 1, 0, 0, 2, 1, 0, 1, 0, 0, 1, 2, 1, 1, 0, 2, 2, 0, 0, 2, 1, 0, 1, 1,
     1, 0, 1, 0, 1, 0, 2, 0, 1, 2, 0, 2, 1, 2, 2, 2, 1, 0, 0, 1, 2, 2, 0, 1, 2, 1, 1, 0, 2, 2, 2, 2, 0, 1, 0,
     1, 1, 1, 2, 0, 1, 2, 1, 1, 0, 1, 1, 2, 0, 0, 1, 0, 1, 0, 0, 2, 2, 2, 2, 0, 1, 2, 0, 1, 2, 2, 0, 1, 2, 0,
     0, 0, 0, 2, 2, 2, 0, 0, 2, 1, 0, 2, 2, 2, 1, 1, 0, 1, 0, 0, 1, 2, 2, 2, 1, 0, 2, 0, 0, 2, 2, 1, 2, 1, 0,
     2, 0, 0, 2, 1, 0, 2, 2, 0, 2, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 1, 0, 0, 0,
     0, 2, 2, 0, 2, 1, 0, 2, 0, 2, 2, 0, 0, 2, 0, 0, 2, 2, 0, 0, 1, 0, 0, 0, 0, 2, 0, 1, 2, 0, 0, 2, 0, 2, 0,
     1, 0, 0, 2, 0, 0, 2, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 2, 2, 0, 2, 0, 2, 1, 1, 2, 1, 2, 0, 1, 2, 2, 1},
    {0, 0, 1, 1, 0, 0, 2, 0, 1, 1, 0, 1, 0, 2, 1, 0, 1, 2, 0, 0, 2, 2, 0, 0, 2, 1, 0, 2, 0, 2, 0, 1, 0, 1, 0,
     0, 2, 2, 1, 1, 1, 1, 2, 0, 1, 2, 1, 2, 2, 0, 1, 2, 0, 0, 1, 1, 0, 2, 2, 0, 0, 2, 2, 1, 0, 1, 1, 0, 1, 0,
     2, 1, 1, 2, 0, 0, 0, 2, 2, 0, 1, 0, 2, 2, 1, 2, 2, 0, 2, 1, 2, 1, 1, 0, 0, 2, 0, 2, 2, 2, 0, 1, 2, 1, 0,
     2, 0, 2, 1, 2, 0, 1, 2, 0, 2, 2, 2, 2, 1, 0, 0, 0, 1, 0, 2, 0, 2, 1, 1, 2, 1, 0, 2, 2, 2, 2, 1, 0, 0, 2,
     0, 1, 2, 0, 2, 1, 1, 1, 0, 1, 0, 0, 1, 2, 1, 2, 2, 0, 2, 0, 0, 2, 1, 1, 0, 2, 0, 1, 0, 0, 1, 1, 1, 1, 2,
     1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 2, 0, 1, 0, 0, 2, 0, 0, 2, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 2, 0, 1, 2, 0,
     2, 0, 0, 2, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 2, 2, 1, 1, 0, 2, 0, 0, 2, 1, 1, 2, 1, 1},
    {2, 0, 0, 1, 1, 0, 0, 0, 0, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 2, 1, 0, 1, 0, 0, 0, 2, 0, 2, 1, 2, 2, 0, 2,
     2, 0, 2, 2, 2, 0, 2, 0, 0, 0, 0, 0, 2, 1, 0, 1, 0, 1, 0, 0, 2, 1, 0, 2, 2, 1, 2, 0, 1, 1, 0, 0, 1, 1, 0,
     1, 0, 2, 0, 2, 0, 1, 0, 0, 2, 2, 2, 2, 1, 1, 1, 0, 1, 2, 2, 0, 2, 2, 2, 2, 0, 1, 2, 2, 0, 0, 2, 0, 1, 2,
     0, 2, 2, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 2, 2, 2, 0, 0, 1, 0, 1, 1, 2, 1, 1, 1,
     2, 0, 1, 0, 2, 0, 0, 2, 1, 2, 0, 1, 2, 2, 0, 0, 1, 2, 1, 0, 0, 2, 2, 1, 2, 2, 1, 2, 1, 1, 2, 1, 0, 0, 0,
     0, 2, 0, 0, 0, 2, 1, 2, 0, 2, 0, 0, 1, 2, 1, 2, 1, 1, 1, 0, 2, 2, 1, 1, 2, 0, 2, 2, 1, 1, 1, 2, 0, 2, 1,
     0, 1, 2, 2, 1, 2, 1, 2, 0, 2, 1, 2, 0, 0, 2, 1, 1, 1, 2, 2, 1, 2, 0, 1, 1, 2, 1, 1, 0, 0, 1, 0, 2},
    {2, 0, 0, 1, 2, 0, 0, 2, 1, 0, 1, 2, 2, 0, 2, 0, 1, 0, 2, 1, 2, 2, 0, 1, 1, 1, 2, 0, 2, 0, 2, 2, 2, 1, 1,
     2, 1, 1, 2, 0, 2, 2, 2, 0, 0, 0, 0, 2, 1, 0, 2, 1, 1, 1, 0, 2, 1, 0, 0, 0, 1, 2, 2, 1, 0, 0, 1, 2, 1, 2,
     2, 0, 1, 1, 0, 2, 1, 1, 0, 2, 2, 2, 0, 1, 0, 1, 1, 1, 1, 2, 1, 2, 1, 1, 0, 1, 0, 1, 0, 1, 2, 0, 1, 0, 2,
     1, 2, 1, 1, 0, 0, 1, 2, 0, 2, 2, 0, 1, 2, 0, 2, 0, 1, 0, 0, 2, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 2, 1, 1, 0,
     2, 0, 2, 0, 1, 1, 1, 1, 1, 2, 2, 1, 1, 2, 1, 0, 0, 1, 1, 0, 2, 0, 0, 2, 1, 0, 1, 0, 1, 2, 0, 0, 1, 0, 2,
     2, 1, 1, 0, 2, 2, 0, 2, 1, 1, 2, 1, 1, 1, 0, 0, 2, 1, 0, 0, 0, 2, 2, 2, 1, 1, 0, 1, 2, 2, 2, 2, 2, 2, 1,
     0, 1, 2, 1, 0, 0, 0, 2, 1, 2, 1, 1, 2, 1, 2, 2, 1, 2, 2, 0, 0, 0, 1, 0, 0, 0, 0, 2, 1, 1, 1, 0, 0},
    {2, 0, 2, 1, 1, 2, 2, 2, 1, 0, 2, 2, 1, 0, 1, 1, 2, 1, 0, 1, 1, 1, 2, 0, 2, 0, 2, 2, 0, 1, 1, 2, 1, 1, 2,
     0, 0, 2, 2, 2, 0, 0, 0, 2, 2, 0, 2, 2, 1, 1, 1, 2, 2, 0, 0, 0, 1, 2, 2, 1, 1, 2, 1, 1, 1, 2, 2, 0, 2, 0,
     0, 0, 2, 1, 1, 2, 0, 1, 0, 1, 2, 1, 1, 0, 2, 0, 1, 1, 1, 1, 0, 1, 1, 2, 1, 2, 1, 0, 2, 0, 0, 2, 0, 1, 2,
     2, 0, 2, 0, 0, 0, 1, 0, 2, 2, 0, 1, 0, 1, 1, 0, 2, 1, 0, 2, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0,
     2, 0, 1, 1, 2, 2, 2, 1, 2, 0, 1, 2, 2, 1, 1, 2, 0, 1, 0, 0, 2, 0, 2, 0, 2, 0, 1, 0, 1, 0, 2, 1, 2, 1, 1,
     1, 1, 2, 2, 0, 2, 2, 0, 2, 0, 1, 1, 2, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 1, 0, 1, 1, 2, 1, 1, 0, 2, 1, 2, 0,
     2, 0, 0, 1, 1, 0, 2, 1, 1, 1, 0, 2, 1, 0, 1, 0, 1, 2, 2, 1, 0, 0, 2, 2, 1, 1, 2, 0, 1, 1, 1, 2, 1},
    {2, 0, 2, 0, 2, 1, 1, 0, 1, 1, 1, 1, 2, 2, 1, 1, 0, 0, 1, 0, 1, 1, 0, 2, 1, 2, 0, 0, 1, 0, 0, 1, 0, 2, 1,
     2, 2, 0, 0, 0, 0, 2, 0, 2, 0, 2, 1, 1, 0, 2, 0, 2, 1, 2, 2, 1, 1, 1, 2, 2, 2, 2, 0, 0, 2, 2, 1, 1, 1, 0,
     1, 1, 0, 2, 1, 2, 2, 2, 0, 0, 0, 1, 0, 2, 0, 1, 1, 1, 1, 1, 0, 2, 2, 1, 2, 1, 0, 0, 2, 1, 1, 1, 0, 2, 2,
     1, 1, 1, 1, 2, 2, 1, 1, 1, 2, 2, 0, 1, 1, 0, 2, 2, 1, 1, 2, 2, 2, 0, 1, 1, 1, 2, 0, 1, 1, 1, 2, 2, 1, 1,
     2, 0, 2, 1, 1, 1, 0, 2, 0, 2, 1, 2, 1, 2, 2, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 1, 0, 0, 2, 1, 1, 2, 0, 1,
     0, 0, 1, 1, 1, 0, 2, 0, 1, 0, 0, 1, 1, 2, 1, 2, 1, 1, 0, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 2, 0, 0, 0, 0, 0,
     0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 2, 0, 2, 0, 1, 1, 0, 2, 1, 0, 2, 1, 1, 2, 0, 1, 0, 0, 0},
    {0, 1, 0, 2, 0, 1, 0, 2, 0, 1, 0, 2, 2, 1, 1, 2, 2, 1, 1, 2, 1, 1, 2, 0, 1, 0, 2, 0, 0, 0, 0, 2, 0, 1, 2,
     2, 0, 1, 0, 2, 0, 1, 0, 2, 2, 2, 1, 2, 2, 1, 1, 2, 1, 2, 2, 0, 0, 0, 2, 0, 0, 1, 0, 2, 1, 1, 2, 0, 0, 2,
     0, 2, 0, 1, 0, 2, 2, 0, 0, 2, 1, 1, 1, 2, 1, 0, 1, 0, 1, 1, 2, 1, 0, 2, 2, 2, 1, 0, 2, 0, 2, 0, 1, 2, 2,
     1, 0, 2, 2, 1, 1, 0, 2, 0, 1, 0, 1, 1, 2, 1, 1, 2, 1, 1, 1, 0, 2, 0, 0, 0, 0, 0, 2, 2, 1, 2, 0, 1, 0, 0,
     2, 2, 1, 0, 1, 0, 1, 0, 1, 2, 1, 1, 2, 2, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 2, 0, 2, 2, 2, 0, 0, 0, 1, 0, 2,
     0, 2, 1, 1, 1, 1, 0, 2, 2, 2, 2, 1, 2, 0, 2, 1, 1, 2, 0, 2, 0, 1, 1, 2, 1, 0, 2, 1, 1, 1, 2, 2, 0, 2, 0,
     0, 1, 2, 1, 1, 2, 0, 1, 0, 2, 2, 1, 0, 0, 2, 0, 1, 2, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 2, 0, 0, 2, 0},
    {2, 1, 2, 2, 2, 0, 0, 0, 2, 2, 2, 0, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2, 1, 0, 1, 1, 1, 2, 0, 0, 0, 1, 2, 0, 1,
     1, 2, 2, 1, 1, 2, 0, 0, 2, 2, 1, 0, 2, 0, 2, 2, 0, 2, 1, 1, 0, 2, 2, 0, 0, 0, 2, 1, 1, 1, 1, 0, 1, 1, 2,
     1, 1, 2, 0, 2, 0, 0, 2, 0, 0, 0, 2, 1, 1, 0, 0, 0, 0, 1, 2, 1, 1, 1, 2, 2, 0, 1, 2, 1, 0, 2, 1, 1, 2, 2,
     0, 1, 2, 0, 0, 1, 0, 1, 2, 2, 0, 0, 2, 2, 0, 1, 1, 2, 2, 1, 0, 2, 0, 2, 2, 2, 1, 0, 1, 0, 2, 2, 0, 2, 2,
     1, 2, 2, 2, 1, 0, 0, 0, 1, 0, 0, 1, 2, 1, 1, 2, 1, 0, 0, 0, 2, 1, 0, 0, 0, 2, 1, 2, 2, 1, 0, 1, 2, 2, 1,
     2, 0, 0, 1, 2, 1, 2, 0, 0, 1, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 0, 2, 0, 0, 1, 2, 2, 2,
     2, 1, 2, 0, 2, 2, 2, 1, 0, 2, 0, 1, 1, 0, 2, 2, 1, 0, 2, 1, 2, 0, 1, 1, 1, 1, 0, 0, 2, 1, 0, 2, 1},
    {0, 2, 2, 1, 1, 0, 0, 0, 0, 0, 1, 1, 2, 1, 2, 2, 0, 0, 1, 1, 2, 0, 1, 0, 2, 1, 2, 1, 2, 2, 0, 1, 2, 0, 2,
     2, 1, 0, 2, 2, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 2, 0, 1, 0, 0, 0, 2, 1, 1, 0, 0, 1, 0, 2, 2, 1, 1, 1, 2, 0,
     0, 2, 1, 1, 2, 2, 1, 2, 2, 0, 1, 1, 0, 1, 0, 0, 0, 2, 2, 2, 0, 0, 2, 0, 2, 2, 2, 2, 1, 1, 0, 0, 2, 0, 2,
     0, 2, 2, 1, 2, 1, 0, 2, 1, 2, 0, 1, 0, 2, 2, 0, 0, 2, 1, 0, 1, 2, 1, 0, 1, 0, 1, 0, 2, 1, 1, 2, 2, 2, 1,
     2, 2, 0, 1, 0, 1, 1, 2, 0, 0, 2, 2, 1, 1, 0, 2, 2, 2, 0, 2, 1, 2, 1, 1, 1, 2, 1, 0, 2, 2, 2, 0, 2, 1, 0,
     2, 0, 1, 2, 1, 0, 2, 0, 0, 2, 1, 0, 1, 2, 0, 2, 0, 0, 1, 0, 2, 1, 0, 1, 1, 0, 2, 0, 2, 0, 0, 2, 0, 0, 1,
     2, 2, 1, 0, 0, 0, 0, 2, 2, 2, 0, 1, 1, 2, 0, 2, 2, 2, 1, 2, 2, 2, 2, 1, 0, 2, 2, 0, 0, 1, 0, 2, 1},
    {0, 1, 0, 1, 2, 0, 2, 0, 0, 2, 2, 1, 1, 0, 1, 1, 0, 0, 2, 1, 2, 1, 0, 0, 0, 2, 1, 1, 2, 2, 2, 1, 2, 2, 1,
     1, 0, 1, 1, 2, 0, 0, 0, 2, 1, 0, 0, 2, 2, 2, 1, 2, 1, 0, 1, 1, 2, 2, 2, 0, 2, 2, 2, 0, 2, 1, 1, 1, 0, 0,
     2, 1, 0, 2, 1, 2, 2, 2, 1, 1, 0, 0, 0, 2, 0, 1, 2, 2, 1, 2, 2, 2, 0, 1, 0, 2, 0, 0, 0, 1, 1, 2, 1, 2, 2,
     0, 1, 1, 1, 2, 0, 1, 0, 2, 2, 2, 1, 1, 2, 0, 1, 2, 1, 2, 2, 2, 0, 2, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 2, 1,
     0, 0, 0, 0, 0, 2, 2, 0, 0, 1, 2, 0, 0, 2, 2, 0, 1, 2, 2, 0, 2, 0, 2, 0, 2, 0, 2, 2, 0, 1, 2, 1, 2, 1, 2,
     0, 0, 2, 0, 1, 1, 2, 1, 1, 2, 0, 0, 1, 2, 2, 0, 0, 0, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 0, 0, 0, 2, 2, 0,
     1, 1, 1, 1, 1, 2, 2, 0, 2, 2, 1, 0, 0, 1, 1, 2, 0, 0, 1, 1, 1, 0, 1, 2, 2, 2, 2, 1, 1, 2, 0, 1, 2},
    {1, 0, 2, 2, 0, 2, 0, 0, 1, 2, 0, 1, 0, 0, 1, 0, 2, 2, 0, 0, 1, 0, 0, 0, 2, 1, 0, 1, 2, 0, 0, 2, 2, 1, 0,
     2, 1, 0, 2, 0, 2, 1, 1, 0, 0, 0, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 2, 2, 0, 1, 2, 0, 0, 2, 0, 0, 1, 2,
     0, 0, 2, 0, 0, 0, 2, 2, 0, 2, 0, 0, 0, 1, 2, 2, 0, 0, 1, 0, 1, 1, 2, 2, 2, 1, 2, 0, 1, 0, 2, 1, 1, 2, 0,
     1, 0, 1, 2, 0, 1, 0, 2, 0, 1, 1, 1, 0, 0, 1, 2, 2, 1, 2, 1, 2, 2, 0, 2, 2, 0, 0, 2, 1, 0, 2, 0, 0, 0, 1,
     0, 1, 0, 0, 2, 0, 1, 1, 0, 1, 2, 0, 1, 0, 1, 2, 0, 0, 1, 0, 0, 1, 1, 1, 0, 2, 2, 0, 0, 0, 1, 1, 2, 1, 1,
     0, 1, 1, 1, 1, 2, 0, 0, 1, 0, 0, 1, 0, 1, 2, 2, 2, 0, 0, 0, 0, 1, 1, 2, 1, 1, 1, 1, 0, 1, 1, 2, 0, 0, 2,
     0, 2, 0, 0, 2, 2, 2, 0, 0, 2, 1, 0, 0, 2, 2, 1, 1, 0, 1, 0, 1, 1, 2, 1, 2, 1, 0, 2, 1, 0, 2, 0, 1},
    {2, 2, 0, 0, 0, 0, 2, 1, 0, 2, 2, 1, 1, 0, 2, 1, 0, 0, 1, 1, 2, 1, 1, 0, 0, 1, 0, 1, 2, 0, 0, 1, 2, 0, 0,
     1, 1, 0, 2, 2, 2, 0, 2, 2, 1, 0, 1, 1, 2, 1, 0, 0, 1, 1, 2, 0, 2, 0, 2, 1, 0, 1, 2, 2, 1, 1, 2, 2, 0, 2,
     1, 2, 0, 2, 0, 1, 2, 0, 2, 2, 1, 1, 1, 1, 0, 0, 1, 0, 1, 2, 2, 0, 2, 2, 0, 0, 1, 1, 2, 2, 0, 0, 0, 1, 2,
     1, 2, 1, 2, 1, 1, 1, 2, 1, 1, 2, 1, 2, 0, 2, 1, 0, 0, 0, 0, 1, 1, 1, 2, 0, 1, 2, 0, 1, 1, 1, 1, 2, 0, 0,
     0, 0, 2, 1, 0, 1, 2, 2, 1, 0, 2, 1, 0, 2, 1, 2, 0, 1, 0, 0, 0, 0, 0, 2, 1, 0, 1, 0, 2, 0, 0, 2, 1, 0, 2,
     2, 2, 2, 0, 0, 1, 0, 0, 1, 2, 0, 1, 1, 2, 0, 0, 0, 2, 0, 0, 2, 2, 2, 2, 1, 2, 0, 0, 0, 2, 2, 0, 2, 0, 1,
     2, 1, 2, 2, 0, 0, 1, 1, 0, 1, 1, 0, 2, 1, 2, 1, 0, 0, 0, 0, 1, 0, 2, 2, 2, 1, 2, 0, 1, 0, 2, 1, 2},
    {2, 0, 1, 0, 1, 2, 0, 2, 0, 2, 2, 1, 1, 1, 0, 1, 1, 2, 0, 1, 2, 2, 2, 0, 0, 2, 2, 0, 0, 2, 1, 1, 1, 0, 2,
     0, 1, 0, 1, 1, 2, 2, 1, 2, 1, 1, 1, 0, 2, 1, 0, 0, 2, 0, 2, 2, 1, 0, 0, 1, 1, 0, 2, 0, 1, 1, 1, 0, 1, 0,
     1, 2, 1, 2, 1, 2, 0, 2, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 2, 0, 1, 0, 0, 2, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 2,
     0, 0, 0, 2, 1, 0, 0, 1, 2, 0, 1, 2, 1, 0, 1, 0, 2, 0, 0, 0, 1, 2, 2, 2, 2, 2, 0, 1, 1, 2, 2, 1, 0, 0, 1,
     2, 2, 2, 1, 0, 1, 1, 0, 2, 2, 1, 2, 1, 2, 0, 0, 1, 1, 1, 0, 2, 1, 1, 2, 2, 1, 1, 2, 1, 0, 1, 0, 1, 0, 2,
     0, 0, 2, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 2, 0, 0, 1, 1, 1, 0, 2, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0,
     0, 0, 0, 2, 1, 2, 2, 1, 0, 0, 0, 2, 1, 2, 0, 0, 1, 1, 2, 2, 1, 2, 1, 2, 2, 1, 2, 1, 0, 0, 2, 1, 0},
    {0, 0, 2, 2, 1, 1, 1, 0, 1, 2, 2, 2, 1, 2, 2, 2, 0, 1, 2, 1, 2, 0, 0, 1, 1, 2, 0, 1, 1, 1, 2, 2, 1, 2, 2,
     0, 2, 1, 1, 1, 0, 0, 0, 2, 0, 2, 1, 2, 2, 2, 2, 2, 0, 2, 2, 2, 0, 1, 0, 0, 1, 0, 0, 2, 1, 2, 1, 0, 0, 0,
     0, 1, 1, 2, 2, 2, 1, 2, 0, 1, 1, 2, 1, 1, 2, 0, 1, 0, 2, 2, 0, 0, 0, 2, 0, 1, 2, 1, 0, 1, 1, 2, 0, 1, 0,
     1, 2, 2, 0, 2, 2, 0, 1, 1, 1, 2, 2, 0, 0, 0, 2, 2, 1, 1, 1, 2, 1, 1, 2, 2, 1, 2, 2, 1, 0, 0, 0, 1, 0, 0,
     0, 0, 1, 1, 2, 1, 0, 0, 2, 0, 1, 1, 2, 0, 2, 1, 1, 0, 1, 2, 2, 2, 1, 2, 1, 1, 0, 1, 2, 1, 2, 0, 2, 0, 1,
     0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 2, 0, 2, 1, 2, 1, 0, 1, 2,
     0, 2, 2, 2, 2, 2, 2, 1, 0, 1, 0, 2, 0, 0, 0, 2, 1, 2, 2, 2, 2, 0, 1, 2, 1, 2, 0, 1, 0, 1, 2, 0, 1},
    {1, 1, 0, 1, 1, 1, 0, 0, 2, 1, 2, 0, 0, 1, 2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 0, 2, 0, 0, 1, 0, 1, 0, 1, 0, 1,
     0, 2, 0, 1, 2, 1, 2, 1, 2, 2, 2, 1, 0, 1, 1, 2, 1, 0, 1, 2, 1, 2, 0, 2, 0, 2, 2, 1, 1, 1, 1, 1, 1, 2, 0,
     1, 2, 2, 0, 0, 0, 1, 2, 0, 0, 2, 2, 1, 1, 1, 2, 0, 2, 0, 2, 1, 2, 2, 1, 2, 1, 1, 2, 2, 2, 0, 0, 0, 2, 0,
     0, 1, 1, 1, 1, 1, 2, 0, 0, 2, 1, 1, 0, 0, 1, 2, 2, 0, 1, 1, 1, 2, 0, 2, 2, 2, 2, 2, 1, 1, 2, 1, 0, 2, 0,
     0, 2, 2, 0, 0, 2, 0, 2, 0, 0, 2, 0, 1, 0, 0, 2, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0,
     1, 1, 0, 1, 2, 2, 0, 0, 1, 1, 0, 0, 1, 2, 2, 1, 2, 1, 0, 2, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 1, 0, 2,
     0, 0, 0, 0, 1, 2, 0, 2, 2, 0, 1, 0, 1, 2, 0, 1, 0, 0, 2, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 2, 2, 2, 0},
};

static const int sbox_lookup[27] = {6,  25, 17, 5,  15, 10, 4,  20, 24, 0,  1, 2,  9, 22,
                                    26, 18, 16, 14, 3,  13, 23, 7,  11, 12, 8, 21, 19};

static const int shift_rows_param[3] = {0, 1, 2};

static const int shift_lanes_param[27] = {19, 13, 21, 10, 24, 15, 2,  9,  3, 14, 0,  6,  5, 1,
                                          25, 22, 23, 20, 7,  17, 26, 12, 8, 18, 16, 11, 4};

void print_troika_slice(trit_t *state, int slice) {
  fprintf(stderr, "#### Slice %i ####\n", slice);
  for (int row = 0; row < ROWS; ++row) {
    for (int column = 0; column < COLUMNS; ++column) {
      fprintf(stderr, "%i ", state[slice * SLICES + row * COLUMNS + column]);
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "------------------\n");
  for (int i = 0; i < COLUMNS; i++) {
    fprintf(stderr, "%i ",
            (state[slice * SLICES + 0 * COLUMNS + i] + state[slice * SLICES + 1 * COLUMNS + i] +
             state[slice * SLICES + 2 * COLUMNS + i]) %
                3);
  }
  fprintf(stderr, "\n");
}

void print_troika_state(trit_t *state) {
  // fprintf(stderr, "Troika State:\n");

  for (int slice = 0; slice < SLICES; ++slice) {
    print_troika_slice(state, slice);
  }
}

void sub_trytes(trit_t *state) {
  int sbox_idx;

  for (sbox_idx = 0; sbox_idx < NUM_SBOXES; ++sbox_idx) {
    tryte_t sbox_input = 9 * state[3 * sbox_idx] + 3 * state[3 * sbox_idx + 1] + state[3 * sbox_idx + 2];
    tryte_t sbox_output = sbox_lookup[sbox_input];
    state[3 * sbox_idx + 2] = sbox_output % 3;
    sbox_output /= 3;
    state[3 * sbox_idx + 1] = sbox_output % 3;
    sbox_output /= 3;
    state[3 * sbox_idx] = sbox_output % 3;
  }
}

void shift_rows(trit_t *state) {
  int slice, row, col, old_idx, new_idx;

  trit_t new_state[STATESIZE];

  for (slice = 0; slice < SLICES; ++slice) {
    for (row = 0; row < ROWS; ++row) {
      for (col = 0; col < COLUMNS; ++col) {
        old_idx = SLICESIZE * slice + COLUMNS * row + col;
        new_idx = SLICESIZE * slice + COLUMNS * row + (col + 3 * shift_rows_param[row]) % COLUMNS;
        new_state[new_idx] = state[old_idx];
      }
    }
  }

  memcpy(state, new_state, STATESIZE);
}

void shift_lanes(trit_t *state) {
  int slice, row, col, old_idx, new_idx, new_slice;

  trit_t new_state[STATESIZE];

  for (slice = 0; slice < SLICES; ++slice) {
    for (row = 0; row < ROWS; ++row) {
      for (col = 0; col < COLUMNS; ++col) {
        old_idx = SLICESIZE * slice + COLUMNS * row + col;
        new_slice = (slice + shift_lanes_param[col + COLUMNS * row]) % SLICES;
        new_idx = SLICESIZE * (new_slice) + COLUMNS * row + col;
        new_state[new_idx] = state[old_idx];
      }
    }
  }

  memcpy(state, new_state, STATESIZE);
}

void add_column_parity(trit_t *state) {
  int slice, row, col, col_sum, idx, sum_to_add;

  trit_t parity[SLICES * COLUMNS];

  // First compute parity for each column
  for (slice = 0; slice < SLICES; ++slice) {
    for (col = 0; col < COLUMNS; ++col) {
      col_sum = 0;
      for (row = 0; row < ROWS; ++row) {
        col_sum += state[SLICESIZE * slice + COLUMNS * row + col];
      }
      parity[COLUMNS * slice + col] = col_sum % 3;
    }
  }

  // Add parity
  for (slice = 0; slice < SLICES; ++slice) {
    for (row = 0; row < ROWS; ++row) {
      for (col = 0; col < COLUMNS; ++col) {
        idx = SLICESIZE * slice + COLUMNS * row + col;
        sum_to_add =
            parity[(col - 1 + 9) % 9 + COLUMNS * slice] + parity[(col + 1) % 9 + COLUMNS * ((slice + 1) % SLICES)];
        state[idx] = (state[idx] + sum_to_add) % 3;
      }
    }
  }
}

void add_round_constant(trit_t *state, int round) {
  int slice, col, idx;

  for (slice = 0; slice < SLICES; ++slice) {
    for (col = 0; col < COLUMNS; ++col) {
      idx = SLICESIZE * slice + col;
      state[idx] = (state[idx] + round_constants[round][slice * COLUMNS + col]) % 3;
    }
  }
}

void troika_permutation(trit_t *state, unsigned long long num_rounds) {
  unsigned long long round;

  assert(num_rounds <= NUM_ROUNDS);

  // PrintTroikaState(state);
  for (round = 0; round < num_rounds; round++) {
    sub_trytes(state);
    shift_rows(state);
    shift_lanes(state);
    add_column_parity(state);
    add_round_constant(state, round);
  }
  // PrintTroikaState(state);
}

void troika_absorb(trit_t *state, unsigned int rate, const trit_t *message, unsigned long long message_length,
                   unsigned long long num_rounds) {
  unsigned long long trit_idx;

  while (message_length >= rate) {
    // Copy message block over the state
    for (trit_idx = 0; trit_idx < rate; ++trit_idx) {
      state[trit_idx] = message[trit_idx];
    }
    troika_permutation(state, num_rounds);
    message_length -= rate;
    message += rate;
  }

  // Pad last block
  trit_t last_block[rate];
  memset(last_block, 0, rate);

  // Copy over last incomplete message block
  for (trit_idx = 0; trit_idx < message_length; ++trit_idx) {
    last_block[trit_idx] = message[trit_idx];
  }

  // Apply padding
  last_block[trit_idx] = PADDING;

  // Insert last message block
  for (trit_idx = 0; trit_idx < rate; ++trit_idx) {
    state[trit_idx] = last_block[trit_idx];
  }
}

void troika_squeeze(trit_t *hash, unsigned long long hash_length, unsigned int rate, trit_t *state,
                    unsigned long long num_rounds) {
  unsigned long long trit_idx;
  while (hash_length >= rate) {
    troika_permutation(state, num_rounds);
    // Extract rate output
    for (trit_idx = 0; trit_idx < rate; ++trit_idx) {
      hash[trit_idx] = state[trit_idx];
    }
    hash += rate;
    hash_length -= rate;
  }

  // Check if there is a last incomplete block
  if (hash_length % rate) {
    troika_permutation(state, num_rounds);
    for (trit_idx = 0; trit_idx < hash_length; ++trit_idx) {
      hash[trit_idx] = state[trit_idx];
    }
  }
}

void troika(trit_t *out, unsigned long long outlen, const trit_t *in, unsigned long long inlen) {
  troika_var_rounds(out, outlen, in, inlen, NUM_ROUNDS);
}

void troika_var_rounds(trit_t *out, unsigned long long outlen, const trit_t *in, unsigned long long inlen,
                       unsigned long long num_rounds) {
  trit_t state[STATESIZE];

  memset(state, 0, STATESIZE);
  troika_absorb(state, TROIKA_RATE, in, inlen, num_rounds);
  troika_squeeze(out, outlen, TROIKA_RATE, state, num_rounds);
}
