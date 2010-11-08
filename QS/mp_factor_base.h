/*============================================================================

    This file is part of FLINT.

    FLINT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    FLINT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FLINT; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

===============================================================================*/
/******************************************************************************

 mp_factor_base.h
 Header file for factor_base.c.

 (C) 2006 William Hart

******************************************************************************/

#ifndef MP_FACTORBASE_H
#define MP_FACTORBASE_H

#include <gmp.h>

#include "mpQS.h"
#include "common.h"

#define KSMAX 1000

unsigned long num_FB_primes(unsigned long bits);

void sqrts_init(QS_t * qs_inf);

void sqrts_clear(void);

void compute_sizes(QS_t * qs_inf);

void sizes_clear(void);

//Knuth-Schroeppel multipliers and a macro to count them

static const unsigned long multipliers[] = {1, 2, 3, 5, 6, 7, 10, 11, 13, 14, 
                                            15, 17, 19, 21, 22, 23, 26, 29, 
                                            30, 31, 33, 34, 35, 37, 38, 41, 
                                            42, 43, 47};

#define NUMMULTS (sizeof(multipliers)/sizeof(unsigned long))
#define max_mult_size 6 // number of bits of maximum multiplier

void primes_clear(void);

void primes_init(QS_t * qs_inf);

unsigned long knuth_schroeppel(QS_t * qs_inf);

unsigned long compute_factor_base(QS_t * qs_inf);

#endif
