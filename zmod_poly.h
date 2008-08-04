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
/*****************************************************************************

   zmod_poly.h: Polynomials over (unsigned) long mod p, for p prime.
   
   Copyright (C) 2007, David Howden

*****************************************************************************/

#include <gmp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "flint.h"
#include "memory-manager.h"
#include "mpn_extras.h"
#include "long_extras.h"

#ifndef _ZMOD_POLY_H_
#define _ZMOD_POLY_H_

#ifdef __cplusplus
 extern "C" {
#endif

#define USE_MIDDLE_PRODUCT 0 // Middle product code currently has no proof 
                             // and so is not switched on by default

typedef struct
{
   unsigned long *coeffs;
   unsigned long alloc;
   unsigned long length;
   unsigned long p;
   double p_inv;
#if PREINV32
   uint32_t p32_inv;
#endif
} zmod_poly_struct;

typedef zmod_poly_struct zmod_poly_t[1];
typedef zmod_poly_struct* zmod_poly_p;

typedef struct
{
   unsigned long length2;
   unsigned long limbs2;
   F_mpn_precomp_t precomp;
} zmod_poly_precomp_struct;

typedef zmod_poly_precomp_struct zmod_poly_precomp_t[1];

/**
 * This is the data type for storing factors for a polynomial
 * It contains an array of polynomials <code>factors</code> that contains the factors of the polynomial.
 * The variable <code>alloc<code> is the number of factors that can be stored in total.
 * <code>num_factor</code> is the number of factors currently stored.
 */
typedef struct
{
	zmod_poly_t* factors;
	unsigned long * exponents;
	unsigned long alloc;
	unsigned long num_factors;
} zmod_poly_factor_struct;

/**
 * This is the data type actually used allowing us to pass the factor array by reference
 */
typedef zmod_poly_factor_struct zmod_poly_factor_t[1];

#define SWAP_ZMOD_POLY_PTRS(x, y)    \
do {                                \
   zmod_poly_p zzz_ptr = (x);        \
   (x) = (y);                       \
   (y) = zzz_ptr;                   \
} while (0);

// ------------------------------------------------------
// Initialisation and memory management

void zmod_poly_init(zmod_poly_t poly, unsigned long p);
void zmod_poly_init_precomp(zmod_poly_t poly, unsigned long p, double p_inv);
void zmod_poly_init2(zmod_poly_t poly, unsigned long p, unsigned long alloc);
void zmod_poly_init2_precomp(zmod_poly_t poly, unsigned long p, double p_inv, unsigned long alloc);
void zmod_poly_clear(zmod_poly_t poly);

void zmod_poly_realloc(zmod_poly_t poly, unsigned long alloc);
// _bits_ only applies to newly allocated coefficients, not existing ones...

// this non-inlined version REQUIRES that alloc > poly->alloc
void __zmod_poly_fit_length(zmod_poly_t poly, unsigned long alloc);

// this is arranged so that the initial comparison (very frequent) is inlined,
// but the actual allocation (infrequent) is not
static inline
void zmod_poly_fit_length(zmod_poly_t poly, unsigned long alloc)
{
   if (alloc > poly->alloc)
      __zmod_poly_fit_length(poly, alloc);
}

// ------------------------------------------------------
// Setting/retrieving coefficients

static inline
unsigned long zmod_poly_get_coeff_ui(zmod_poly_t poly, unsigned long n)
{
   if (n >= poly->length)
      return 0;
   return poly->coeffs[n];
}

static inline
unsigned long _zmod_poly_get_coeff_ui(zmod_poly_t poly, unsigned long n)
{
   return poly->coeffs[n];
}

void zmod_poly_set_coeff_ui(zmod_poly_t poly, unsigned long n, unsigned long c);

static inline
void _zmod_poly_set_coeff_ui(zmod_poly_t poly, unsigned long n, unsigned long c)
{
	poly->coeffs[n] = c;
}

// ------------------------------------------------------
// String conversions and I/O

int zmod_poly_from_string(zmod_poly_t poly, char* s);
char* zmod_poly_to_string(zmod_poly_t poly);
void zmod_poly_print(zmod_poly_t poly);
void zmod_poly_fprint(zmod_poly_t poly, FILE* f);
int zmod_poly_read(zmod_poly_t poly);
int zmod_poly_fread(zmod_poly_t poly, FILE* f);


// ------------------------------------------------------
// Length and degree

void __zmod_poly_normalise(zmod_poly_t poly);
int __zmod_poly_normalised(zmod_poly_t poly);
void zmod_poly_truncate(zmod_poly_t poly, unsigned long length);


static inline
unsigned long zmod_poly_length(zmod_poly_t poly)
{
   return poly->length;
}


static inline
long zmod_poly_degree(zmod_poly_t poly)
{
   return (long) poly->length - 1;
}


static inline
unsigned long zmod_poly_modulus(zmod_poly_t poly)
{
   return poly->p;
}

static inline
double zmod_poly_precomputed_inverse(zmod_poly_t poly)
{
   return poly->p_inv;
}


// ------------------------------------------------------
// Assignment

void _zmod_poly_set(zmod_poly_t res, zmod_poly_t poly);
void zmod_poly_set(zmod_poly_t res, zmod_poly_t poly);


static inline
void zmod_poly_zero(zmod_poly_t poly)
{
   poly->length = 0;
}


static inline
void zmod_poly_swap(zmod_poly_t poly1, zmod_poly_t poly2)
{
    unsigned long* temp_coeffs;
    unsigned long temp;
    double temp_p_inv;

    temp_coeffs = poly2->coeffs;
    poly2->coeffs = poly1->coeffs;
    poly1->coeffs = temp_coeffs;
    
    temp = poly1->alloc;
    poly1->alloc = poly2->alloc;
    poly2->alloc = temp;

    temp = poly1->length;
    poly1->length = poly2->length;
    poly2->length = temp;
    
    temp = poly1->p;
    poly1->p = poly2->p;
    poly2->p = temp;
    
    temp_p_inv = poly1->p_inv;
    poly1->p_inv = poly2->p_inv;
    poly2->p_inv = temp_p_inv;
}

/*
   Subpolynomials
*/

static inline 
void _zmod_poly_attach(zmod_poly_t output, zmod_poly_t input)
{
   output->length = input->length;
   output->coeffs = input->coeffs;
   output->p = input->p;
   output->p_inv = input->p_inv;
}

static inline 
void zmod_poly_attach(zmod_poly_t output, zmod_poly_t input)
{
   _zmod_poly_attach(output, input);
}

/*
   Attach input shifted right by n to output
*/

static inline 
void _zmod_poly_attach_shift(zmod_poly_t output, 
                   zmod_poly_t input, unsigned long n)
{
   if (input->length >= n) output->length = input->length - n;
   else output->length = 0;
   output->coeffs = input->coeffs + n;
   output->p = input->p;
   output->p_inv = input->p_inv;
}

static inline 
void zmod_poly_attach_shift(zmod_poly_t output, 
                   zmod_poly_t input, unsigned long n)
{
   _zmod_poly_attach_shift(output, input, n);
}

/*
   Attach input to first n coefficients of input
*/

static inline 
void _zmod_poly_attach_truncate(zmod_poly_t output, 
                     zmod_poly_t input, unsigned long n)
{
   if (input->length < n) output->length = input->length;
   else output->length = n;
   output->coeffs = input->coeffs;
   output->p = input->p;
   output->p_inv = input->p_inv;
   __zmod_poly_normalise(output);
}

static inline 
void zmod_poly_attach_truncate(zmod_poly_t output, 
                     zmod_poly_t input, unsigned long n)
{
   _zmod_poly_attach_truncate(output, input, n);
}


/*
   Comparison functions
*/

int zmod_poly_equal(zmod_poly_t poly1, zmod_poly_t poly2);

static inline
int zmod_poly_is_one(zmod_poly_t poly1)
{ 
   if ((poly1->length == 1) && (poly1->coeffs[0] == 1L)) return 1;
   return 0;
}

static inline
int zmod_poly_is_zero(zmod_poly_t poly1)
{ 
   return (poly1->length == 0L);
}

/*
   Reversal
*/

void _zmod_poly_reverse(zmod_poly_t output, zmod_poly_t input, unsigned long length);
void zmod_poly_reverse(zmod_poly_t output, zmod_poly_t input, unsigned long length);

/*
   Monic polys
*/

void zmod_poly_make_monic(zmod_poly_t output, zmod_poly_t pol);

/*
   Addition and subtraction
*/

void zmod_poly_add(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2);
void zmod_poly_add_without_mod(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2);
void zmod_poly_sub(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2);
void _zmod_poly_sub(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2);
void zmod_poly_neg(zmod_poly_t res, zmod_poly_t poly);


/*
   Shifting functions
*/

void zmod_poly_left_shift(zmod_poly_t res, zmod_poly_t poly, unsigned long k);
void zmod_poly_right_shift(zmod_poly_t res, zmod_poly_t poly, unsigned long k);

/*
   Polynomial multiplication
   
   All multiplication functions require that the modulus be no more than FLINT_BITS-1 bits
*/

void zmod_poly_mul(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2);
void zmod_poly_sqr(zmod_poly_t res, zmod_poly_t poly);

/* Requires that poly1 bits + poly2 bits + log_length is not greater than 2*FLINT_BITS */

void zmod_poly_mul_KS(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long bits_input);
void _zmod_poly_mul_KS(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long bits_input);

void zmod_poly_mul_KS_trunc(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long bits_input, unsigned long trunc);
void _zmod_poly_mul_KS_trunc(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long bits_input, unsigned long trunc);
void _zmod_poly_mul_KS_trunc_precomp(zmod_poly_t output, zmod_poly_p input1, zmod_poly_precomp_t pre, unsigned long bits_input, unsigned long trunc);
#if USE_MIDDLE_PRODUCT
void _zmod_poly_mul_KS_middle(zmod_poly_t output, zmod_poly_p input1, zmod_poly_p input2, unsigned long bits_input, unsigned long trunc);
void zmod_poly_mul_KS_middle(zmod_poly_t output, zmod_poly_p input1, zmod_poly_p input2, unsigned long bits_input, unsigned long trunc);
#endif

void _zmod_poly_mul_classical(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2);
void __zmod_poly_mul_classical_mod_last(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long bits);
void __zmod_poly_mul_classical_mod_throughout(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long bits);
void zmod_poly_mul_classical(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2);
void _zmod_poly_sqr_classical(zmod_poly_t res, zmod_poly_t poly);
void zmod_poly_sqr_classical(zmod_poly_t res, zmod_poly_t poly);

void _zmod_poly_mul_classical_trunc(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long trunc);
void __zmod_poly_mul_classical_trunc_mod_last(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long bits, unsigned long trunc);
void __zmod_poly_mul_classical_trunc_mod_throughout(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long bits, unsigned long trunc);
void zmod_poly_mul_classical_trunc(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long trunc);

void _zmod_poly_mul_classical_trunc_left(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long trunc);
void __zmod_poly_mul_classical_trunc_left_mod_last(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long bits, unsigned long trunc);
void __zmod_poly_mul_classical_trunc_left_mod_throughout(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long bits, unsigned long trunc);
void zmod_poly_mul_classical_trunc_left(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long trunc);

void zmod_poly_mul_trunc_n(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long trunc);
void zmod_poly_mul_trunc_left_n(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, unsigned long trunc);

void zmod_poly_mul_trunc_n_precomp_init(zmod_poly_precomp_t pre, zmod_poly_p input2, unsigned long bits_input, unsigned long length1);
void zmod_poly_precomp_clear(zmod_poly_precomp_t pre);
void zmod_poly_mul_trunc_n_precomp(zmod_poly_t output, zmod_poly_p input1, zmod_poly_precomp_t pre, unsigned long trunc);
#if USE_MIDDLE_PRODUCT
void _zmod_poly_mul_KS_middle_precomp(zmod_poly_t output, zmod_poly_p input1, zmod_poly_precomp_t pre, unsigned long bits_input, unsigned long trunc);
#endif
void _zmod_poly_mul_KS_precomp(zmod_poly_t output, zmod_poly_t input1, zmod_poly_precomp_t pre, unsigned long bits_input);
void zmod_poly_mul_precomp_init(zmod_poly_precomp_t pre, zmod_poly_t input2, unsigned long bits_input, unsigned long length1);
/*
	Bit packing functions
*/

unsigned long zmod_poly_bits(zmod_poly_t poly);
void _zmod_poly_bit_pack_mpn(mp_limb_t * res, zmod_poly_t poly, unsigned long bits, unsigned long length);
void _zmod_poly_bit_unpack_mpn(zmod_poly_t poly, mp_limb_t *mpn, unsigned long length, unsigned long bits);

void print_binary(unsigned long n, unsigned long len);
void print_binary2(unsigned long n, unsigned long len, unsigned long space_bit);

/*
   Scalar multiplication
*/

void _zmod_poly_scalar_mul(zmod_poly_t res, zmod_poly_t poly, unsigned long scalar);
void zmod_poly_scalar_mul(zmod_poly_t res, zmod_poly_t poly, unsigned long scalar);
void __zmod_poly_scalar_mul_without_mod(zmod_poly_t res, zmod_poly_t poly, unsigned long scalar);

/* 
   Division
*/

void zmod_poly_divrem_classical(zmod_poly_t Q, zmod_poly_t R, zmod_poly_t A, zmod_poly_t B);
void __zmod_poly_divrem_classical_mod_last(zmod_poly_t Q, zmod_poly_t R, zmod_poly_t A, zmod_poly_t B);
void zmod_poly_div_classical(zmod_poly_t Q, zmod_poly_t A, zmod_poly_t B);
void __zmod_poly_div_classical_mod_last(zmod_poly_t Q, zmod_poly_t A, zmod_poly_t B);
void zmod_poly_div_divconquer_recursive(zmod_poly_t Q, zmod_poly_t BQ, zmod_poly_t A, zmod_poly_t B);
void zmod_poly_divrem_divconquer(zmod_poly_t Q, zmod_poly_t R, zmod_poly_t A, zmod_poly_t B);
void zmod_poly_div_divconquer(zmod_poly_t Q, zmod_poly_t A, zmod_poly_t B);

/*
   Newton Inversion
*/

void zmod_poly_newton_invert_basecase(zmod_poly_t Q_inv, zmod_poly_t Q, unsigned long n);
void zmod_poly_newton_invert(zmod_poly_t Q_inv, zmod_poly_t Q, unsigned long n);

/*
   Newton Division
*/

void zmod_poly_div_series(zmod_poly_t Q, zmod_poly_t A, zmod_poly_t B, unsigned long n);
void zmod_poly_div_newton(zmod_poly_t Q, zmod_poly_t A, zmod_poly_t B);
void zmod_poly_divrem_newton(zmod_poly_t Q, zmod_poly_t R, zmod_poly_t A, zmod_poly_t B);

#define ZMOD_DIV_BASECASE_CUTOFF 64

static inline
void zmod_poly_divrem(zmod_poly_t Q, zmod_poly_t R, zmod_poly_t A, zmod_poly_t B)
{
   if ((B->length < ZMOD_DIV_BASECASE_CUTOFF) && (A->length < 2*ZMOD_DIV_BASECASE_CUTOFF))
   {
      zmod_poly_divrem_classical(Q, R, A, B);
      return;
   }

   zmod_poly_divrem_newton(Q, R, A, B);
}

static inline
void zmod_poly_div(zmod_poly_t Q, zmod_poly_t A, zmod_poly_t B)
{
   if ((B->length < ZMOD_DIV_BASECASE_CUTOFF) && (A->length < 2*ZMOD_DIV_BASECASE_CUTOFF))
   {
      zmod_poly_div_classical(Q, A, B);
      return;
   }
   
   zmod_poly_div_newton(Q, A, B);
}


/*
   Resultant
*/

unsigned long zmod_poly_resultant_euclidean(zmod_poly_t a, zmod_poly_t b);

static inline
unsigned long zmod_poly_resultant(zmod_poly_t a, zmod_poly_t b)
{
   return zmod_poly_resultant_euclidean(a, b);
}

/*
   GCD
*/

void zmod_poly_gcd(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2);
int zmod_poly_gcd_invert(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2);
void zmod_poly_xgcd(zmod_poly_t res, zmod_poly_t s, zmod_poly_t t, zmod_poly_t poly1, zmod_poly_t poly2);

/*
   Derivative
*/

void zmod_poly_derivative(zmod_poly_t x_primed, zmod_poly_t x);

/*
   Modular Arithmetic
*/

void zmod_poly_mulmod(zmod_poly_t res, zmod_poly_t poly1, zmod_poly_t poly2, zmod_poly_t f);
void zmod_poly_powmod(zmod_poly_t res, zmod_poly_t pol, long exp, zmod_poly_t f);

/*
   Factorisation
*/

/**
 * Initialises polynomial factor structure. 
 * @param arr		The array that will be storing the factors.
 */ 
void zmod_poly_factor_init(zmod_poly_factor_t fac);

/**
 * Frees up the memory being used by the factor array. After calling this init will need to be called in order to use the structure again.
 * @param arr		The array to be cleared.
 */
void zmod_poly_factor_clear(zmod_poly_factor_t fac);

/**
 * Adds a polynomial to the factor array. Automatically allocates new memory if needed. 
 * @param fac		Factor array to be augmented.
 * @param poly		Polynomial to be added to the array.
 */
void zmod_poly_factor_add(zmod_poly_factor_t fac, zmod_poly_t poly);

/**
 * Concatenates two polynomial factor structures together setting <code>res = {res, fac}</code>. 
 * Automatically makes res bigger if this is required.
 * @param res		The result of the concatenation and the first of the factor structures to be concatenated.
 * @param fac		The struct to be concatenated to res.
 */
void zmod_poly_factor_concat(zmod_poly_factor_t res, zmod_poly_factor_t fac);

/**
 * Prints the given list of factors to stdout using <code>zmod_poly_print()</code>, each one on a new line.
 * @param fac   	Factor structure to be printed.
 */
void zmod_poly_factor_print(zmod_poly_factor_t fac);	

/**
 * Multiply the exponents of the given factor struct to the given value
 * @param fac   	Factor structure to be raised to a power.
 * @param exp		Exponent to raise the factor to
 */
void zmod_poly_factor_pow(zmod_poly_factor_t fac, unsigned long exp);

/**
 * Computes the square free factorisation of the given polynomial.
 * @param f 		The polynomial to factorise.
 * @param fac		The factorisation into square free factors of <code>f</code>.
 */
void zmod_poly_factor_square_free(zmod_poly_factor_t res, zmod_poly_t f);

/**
 * Computes the factorisation of the given polynomial.
 * @param f 		The polynomial to factorise.
 * @param factors	The factorisation of <code>f</code>.
 */
void zmod_poly_factor_berlekamp(zmod_poly_factor_t factors, zmod_poly_t f);

#ifdef __cplusplus
 }
#endif

#endif /* _ZMOD_POLY_H_ */
