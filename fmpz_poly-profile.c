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
/****************************************************************************

fmpz_poly-profile.c

Profiling for fmpz_poly

Copyright (C) 2007, William Hart and David Harvey

*****************************************************************************/

#include <string.h>
#include <math.h>
#include <gmp.h>
#include "profiler-main.h"
#include "flint.h"
#include "memory-manager.h"
#include "fmpz_poly.h"
#include "mpz_poly.h"
#include "test-support.h"

//=============================================================================

// whether to generate signed or unsigned random polys
#define SIGNS 0


unsigned long randint(unsigned long randsup) 
{
    static unsigned long randval = 4035456057U;
    randval = ((unsigned long)randval*1025416097U+286824428U)%(unsigned long)4294967291U;
    
    return (unsigned long)randval%randsup;
}

void randpoly(mpz_poly_t pol, unsigned long length, unsigned long maxbits)
{
   unsigned long bits;
   mpz_t temp;
   mpz_init(temp);
   
   mpz_poly_zero(pol);
   
   unsigned long i;
   for (i = 0; i < length; i++)
   {
       bits = maxbits;
       if (bits == 0) mpz_set_ui(temp,0);
       else 
       {
          mpz_urandomb(temp, randstate, bits);
			 if (randint(2)) mpz_clrbit(temp, bits-1);
#if SIGNS
          if (randint(2)) mpz_neg(temp,temp);
#endif
       }
       mpz_poly_set_coeff(pol, i, temp);
   }
   
   mpz_clear(temp);
} 

// ============================================================================


/*
Calls prof2d_sample(length, bits, NULL) for all length, bits combinations
such that length*bits < max_bits, with length and bits spaced out by
the given ratio
 */
void run_triangle(unsigned long max_bits, double ratio)
{
   int max_iter = (int) ceil(log((double) max_bits) / log(ratio));

   unsigned long last_length = 0;
   unsigned long i;
   for (i = 0; i <= max_iter; i++)
   {
      unsigned long length = (unsigned long) floor(pow(ratio, i));
      if (length != last_length)
      {
         last_length = length;

         unsigned long last_bits = 0;
         unsigned long j;
         for (j = 0; j <= max_iter; j++)
         {
            unsigned long bits = (unsigned long) floor(pow(ratio, j));
            if (bits != last_bits)
            {
               last_bits = bits;

               if (bits * length < max_bits)
                  prof2d_sample(length, bits, NULL);
            }
         }
      }
   }
}

void run_square_triangle(unsigned long max_bits, double ratio)
{
   int max_iter = (int) ceil(log((double) (max_bits/20)) / log(ratio));

   unsigned long last_length = 0;
   unsigned long i;
   for (i = 0; i <= max_iter; i++)
   {
      unsigned long length = (unsigned long) 20*floor(pow(ratio, i));
      if (length != last_length)
      {
         last_length = length;

         unsigned long last_bits = 0;
         unsigned long j;
         for (j = 0; j <= max_iter; j++)
         {
            unsigned long bits = (unsigned long) 20*floor(pow(ratio, j));
            if (bits != last_bits)
            {
               last_bits = bits;

               if (bits * length * bits * length < 1000000000UL)
                  prof2d_sample(length, bits, NULL);
            }
         }
      }
   }
}


// ============================================================================


void sample_fmpz_poly_mul_KS(unsigned long length, unsigned long bits,
                             void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3;
   mpz_poly_t r_poly, r_poly2;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
  
   _fmpz_poly_stack_init(poly1, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly2, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly3, 2*length-1, (output_bits-1)/FLINT_BITS+1);
   
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
      }
      prof_start();
      _fmpz_poly_mul_KS(poly3, poly1, poly2, 0);
      prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   
   _fmpz_poly_stack_clear(poly3);
   _fmpz_poly_stack_clear(poly2);
   _fmpz_poly_stack_clear(poly1);
   
}


char* profDriverString_fmpz_poly_mul_KS(char* params)
{
   return "fmpz_poly_mul_KS over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_mul_KS()
{
   return "16000000 1.2";
}


void profDriver_fmpz_poly_mul_KS(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);

   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_mul_KS);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}

// ============================================================================


void sample_fmpz_poly_mul_KS_trunc(unsigned long length, unsigned long bits,
                             void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3;
   mpz_poly_t r_poly, r_poly2;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
  
   _fmpz_poly_stack_init(poly1, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly2, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly3, 2*length-1, (output_bits-1)/FLINT_BITS+1);
   
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
      }
      prof_start();
      _fmpz_poly_mul_KS_trunc(poly3, poly1, poly2, length, 0);
      prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   
   _fmpz_poly_stack_clear(poly3);
   _fmpz_poly_stack_clear(poly2);
   _fmpz_poly_stack_clear(poly1);
   
}


char* profDriverString_fmpz_poly_mul_KS_trunc(char* params)
{
   return "fmpz_poly_mul_KS_trunc over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_mul_KS_trunc()
{
   return "16000000 1.2";
}


void profDriver_fmpz_poly_mul_KS_trunc(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);

   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_mul_KS_trunc);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}


// ****************************************************************************

void sample_fmpz_poly_mul_SS(unsigned long length, unsigned long bits,
                             void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3;
   mpz_poly_t r_poly, r_poly2;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
  
   _fmpz_poly_stack_init(poly1, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly2, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly3, 2*length-1, (output_bits-1)/FLINT_BITS+1);
    
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
      }
       prof_start();
       _fmpz_poly_mul_SS(poly3, poly1, poly2);
       prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   
   _fmpz_poly_stack_clear(poly3);
   _fmpz_poly_stack_clear(poly2);
   _fmpz_poly_stack_clear(poly1);
   
}


char* profDriverString_fmpz_poly_mul_SS(char* params)
{
   return "fmpz_poly_mul_SS over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_mul_SS()
{
   return "16000000 1.2";
}

void profDriver_fmpz_poly_mul_SS(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);
   
   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_mul_SS);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}

// ****************************************************************************

void sample_fmpz_poly_mul_SS_trunc(unsigned long length, unsigned long bits,
                             void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3;
   mpz_poly_t r_poly, r_poly2;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
  
   _fmpz_poly_stack_init(poly1, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly2, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly3, 2*length-1, (output_bits-1)/FLINT_BITS+1);
    
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
      }
       prof_start();
       _fmpz_poly_mul_SS_trunc(poly3, poly1, poly2, length);
       prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   
   _fmpz_poly_stack_clear(poly3);
   _fmpz_poly_stack_clear(poly2);
   _fmpz_poly_stack_clear(poly1);
   
}


char* profDriverString_fmpz_poly_mul_SS_trunc(char* params)
{
   return "fmpz_poly_mul_SS_trunc over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_mul_SS_trunc()
{
   return "16000000 1.2";
}

void profDriver_fmpz_poly_mul_SS_trunc(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);
   
   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_mul_SS_trunc);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}


// ============================================================================


void sample_fmpz_poly_mul_karatsuba(unsigned long length, unsigned long bits,
                                    void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3;
   mpz_poly_t r_poly, r_poly2;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
  
   _fmpz_poly_stack_init(poly1, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly2, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly3, 2*length-1, (output_bits-1)/FLINT_BITS+1);
    
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
      }
       prof_start();
       _fmpz_poly_mul_karatsuba(poly3, poly1, poly2);
       prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   
   _fmpz_poly_stack_clear(poly3);
   _fmpz_poly_stack_clear(poly2);
   _fmpz_poly_stack_clear(poly1);
   
}


char* profDriverString_fmpz_poly_mul_karatsuba(char* params)
{
   return "fmpz_poly_mul_karatsuba over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_mul_karatsuba()
{
   return "300000 1.2";
}

void profDriver_fmpz_poly_mul_karatsuba(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);

   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_mul_karatsuba);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}

// ============================================================================


void sample_fmpz_poly_mul_karatsuba_trunc(unsigned long length, unsigned long bits,
                                    void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3;
   mpz_poly_t r_poly, r_poly2;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
  
   _fmpz_poly_stack_init(poly1, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly2, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly3, 2*length-1, (output_bits-1)/FLINT_BITS+1);
    
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
      }
       prof_start();
       _fmpz_poly_mul_karatsuba_trunc(poly3, poly1, poly2, length);
       prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   
   _fmpz_poly_stack_clear(poly3);
   _fmpz_poly_stack_clear(poly2);
   _fmpz_poly_stack_clear(poly1);
   
}


char* profDriverString_fmpz_poly_mul_karatsuba_trunc(char* params)
{
   return "fmpz_poly_mul_karatsuba_trunc over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_mul_karatsuba_trunc()
{
   return "300000 1.2";
}

void profDriver_fmpz_poly_mul_karatsuba_trunc(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);

   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_mul_karatsuba_trunc);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}


// ============================================================================


char* profDriverString_fmpz_poly_mul_karatsuba_len(char* params)
{
   return "fmpz_poly_mul_karatsuba over various lengths with fixed bitsize.\n"
   "Parameters are: none.";
}

char* profDriverDefaultParams_fmpz_poly_mul_karatsuba_len()
{
   return "";
}

void profDriver_fmpz_poly_mul_karatsuba_len(char* params)
{
   unsigned long max_bits;
   double ratio;

   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_mul_karatsuba);
   unsigned long length;
   for (length = 1; length < 128; length++)
      prof2d_sample(length, 100, NULL);
 
   test_support_cleanup();
}


// ============================================================================


void sample_fmpz_poly_mul(unsigned long length, unsigned long bits,
                          void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3;
   mpz_poly_t r_poly, r_poly2;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
  
   _fmpz_poly_stack_init(poly1, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly2, length, (bits-1)/FLINT_BITS+1);
   _fmpz_poly_stack_init(poly3, 2*length-1, (output_bits-1)/FLINT_BITS+1);
   
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
      }
       prof_start();
       _fmpz_poly_mul(poly3, poly1, poly2);
       prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   
   _fmpz_poly_stack_clear(poly3);
   _fmpz_poly_stack_clear(poly2);
   _fmpz_poly_stack_clear(poly1);
   
}




char* profDriverString_fmpz_poly_mul(char* params)
{
   return "fmpz_poly_mul over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}
 
char* profDriverDefaultParams_fmpz_poly_mul()
{
   return "8000000 1.2";
}
 
void profDriver_fmpz_poly_mul(char* params)
{
   unsigned long max_bits;
   double ratio;
    
   sscanf(params, "%ld %lf", &max_bits, &ratio);
 
   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_mul);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}




char* profDriverString_fmpz_poly_mul_specific(char* params)
{
   return "fmpz_poly_mul for a specific length and bitsize.\n"
   "Parameters are: length, bitsize.";
}

char* profDriverDefaultParams_fmpz_poly_mul_specific()
{
   return "1024 1024";
}

void profDriver_fmpz_poly_mul_specific(char* params)
{
   unsigned long length;
   unsigned long bits;
   
   sscanf(params, "%ld %ld", &length, &bits);

   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_mul);
   prof2d_sample(length, bits, NULL);
   test_support_cleanup();
}



// ============================================================================

void sample_fmpz_poly_div_mulders(unsigned long length, unsigned long bits,
                                    void* arg, unsigned long count)
{
                                    
   mpz_poly_t test_poly, test_poly2;
   fmpz_poly_t test_mpn_poly, test_mpn_poly2, test_mpn_poly3, test_mpn_poly4;
   unsigned long bits2, length2, r_count;
   
   mpz_poly_init(test_poly); 
   mpz_poly_init(test_poly2); 
   
   if (count >= 10000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 4;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   fmpz_poly_init2(test_mpn_poly, 1, (bits-1)/FLINT_BITS+1);
   fmpz_poly_init2(test_mpn_poly2, 1, (bits2-1)/FLINT_BITS+1);

   length2 = length;
   bits2 = bits;
      
   unsigned long count1;
   for (count1 = 0; count1 < count ; count1++)
   {            
      if (count1 % r_count == 0)
      {
         do {
            randpoly(test_poly, length, bits); 
            fmpz_poly_realloc(test_mpn_poly, length);
            mpz_poly_to_fmpz_poly(test_mpn_poly, test_poly);
            _fmpz_poly_normalise(test_mpn_poly);
         } while (test_mpn_poly->length == 0);
      
         randpoly(test_poly2, length2, bits2);     
         fmpz_poly_realloc(test_mpn_poly2, length2);
         mpz_poly_to_fmpz_poly(test_mpn_poly2, test_poly2);
      }
          
      fmpz_poly_init(test_mpn_poly3);    
      fmpz_poly_mul(test_mpn_poly3, test_mpn_poly, test_mpn_poly2);
      
      fmpz_poly_init(test_mpn_poly4);
      
      prof_start();
      unsigned long i;
      for (i = 0; i < r_count; i++)
         fmpz_poly_div_mulders(test_mpn_poly4, test_mpn_poly3, test_mpn_poly);
      prof_stop();
      count1+=(r_count-1);
           
      fmpz_poly_clear(test_mpn_poly3);
      fmpz_poly_clear(test_mpn_poly4);
   }
   
   fmpz_poly_clear(test_mpn_poly);
   fmpz_poly_clear(test_mpn_poly2);

   mpz_poly_clear(test_poly);
   mpz_poly_clear(test_poly2);
}


char* profDriverString_fmpz_poly_div_mulders(char* params)
{
   return "fmpz_poly_div_mulders over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_div_mulders()
{
   return "1000000 1.2";
}

void profDriver_fmpz_poly_div_mulders(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);

   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_div_mulders);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}

// ============================================================================

void sample_fmpz_poly_div(unsigned long length, unsigned long bits,
                                    void* arg, unsigned long count)
{
                                    
   mpz_poly_t test_poly, test_poly2;
   fmpz_poly_t test_mpn_poly, test_mpn_poly2, test_mpn_poly3, test_mpn_poly4;
   unsigned long bits2, length2, r_count;
   
   mpz_poly_init(test_poly); 
   mpz_poly_init(test_poly2); 
   
   if (count >= 10000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 4;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   fmpz_poly_init(test_mpn_poly);
   fmpz_poly_init(test_mpn_poly2);
   fmpz_poly_init(test_mpn_poly3);    
   fmpz_poly_init(test_mpn_poly4);
           
   length2 = length;
   bits2 = bits;
      
   unsigned long count1;
   for (count1 = 0; count1 < count ; count1++)
   {            
      if (count1 % r_count == 0)
      {
         do {
            randpoly(test_poly, length, bits); 
            fmpz_poly_realloc(test_mpn_poly, length);
            mpz_poly_to_fmpz_poly(test_mpn_poly, test_poly);
            _fmpz_poly_normalise(test_mpn_poly);
         } while (test_mpn_poly->length == 0);
      
         randpoly(test_poly2, length2, bits2);     
         fmpz_poly_realloc(test_mpn_poly2, length2);
         mpz_poly_to_fmpz_poly(test_mpn_poly2, test_poly2);

			fmpz_poly_mul(test_mpn_poly3, test_mpn_poly, test_mpn_poly2);
      }
                
      prof_start();
         fmpz_poly_div(test_mpn_poly4, test_mpn_poly3, test_mpn_poly);
      prof_stop();     
   }
   
   fmpz_poly_clear(test_mpn_poly3);
   fmpz_poly_clear(test_mpn_poly4);
   fmpz_poly_clear(test_mpn_poly);
   fmpz_poly_clear(test_mpn_poly2);

   mpz_poly_clear(test_poly);
   mpz_poly_clear(test_poly2);
}


char* profDriverString_fmpz_poly_div(char* params)
{
   return "fmpz_poly_div over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_div()
{
   return "1000000 1.2";
}

void profDriver_fmpz_poly_div(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);

   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_div);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}

//============================================================================

void sample_fmpz_poly_divexact(unsigned long length, unsigned long bits,
                                    void* arg, unsigned long count)
{
                                    
   mpz_poly_t test_poly, test_poly2;
   fmpz_poly_t test_mpn_poly, test_mpn_poly2, test_mpn_poly3, test_mpn_poly4;
   unsigned long bits2, length2, r_count;
   
   mpz_poly_init(test_poly); 
   mpz_poly_init(test_poly2); 
   
   if (count >= 10000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 4;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   fmpz_poly_init(test_mpn_poly);
   fmpz_poly_init(test_mpn_poly2);
   fmpz_poly_init(test_mpn_poly3);    
   fmpz_poly_init(test_mpn_poly4);
           
   length2 = length;
   bits2 = bits;
      
   unsigned long count1;
   for (count1 = 0; count1 < count ; count1++)
   {            
      if (count1 % r_count == 0)
      {
         do {
            randpoly(test_poly, length, bits); 
            fmpz_poly_realloc(test_mpn_poly, length);
            mpz_poly_to_fmpz_poly(test_mpn_poly, test_poly);
            _fmpz_poly_normalise(test_mpn_poly);
         } while (test_mpn_poly->length == 0);
      
         randpoly(test_poly2, length2, bits2);     
         fmpz_poly_realloc(test_mpn_poly2, length2);
         mpz_poly_to_fmpz_poly(test_mpn_poly2, test_poly2);

			fmpz_poly_mul(test_mpn_poly3, test_mpn_poly, test_mpn_poly2);
      }
                
      prof_start();
         fmpz_poly_divexact(test_mpn_poly4, test_mpn_poly3, test_mpn_poly);
      prof_stop();     
   }
   
   fmpz_poly_clear(test_mpn_poly3);
   fmpz_poly_clear(test_mpn_poly4);
   fmpz_poly_clear(test_mpn_poly);
   fmpz_poly_clear(test_mpn_poly2);

   mpz_poly_clear(test_poly);
   mpz_poly_clear(test_poly2);
}


char* profDriverString_fmpz_poly_divexact(char* params)
{
   return "fmpz_poly_divexact over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_divexact()
{
   return "1000000 1.2";
}

void profDriver_fmpz_poly_divexact(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);

   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_divexact);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}

//============================================================================

void sample_fmpz_poly_gcd_subresultant(unsigned long length, unsigned long bits,
                             void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3, poly4;
   mpz_poly_t r_poly, r_poly2, r_poly3;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_init(r_poly3); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
   mpz_poly_realloc(r_poly3, length);
  
   fmpz_poly_init(poly1);
   fmpz_poly_init(poly2);
   fmpz_poly_init(poly3);
   fmpz_poly_init(poly4);
    
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
         randpoly(r_poly3, length, bits);
         mpz_poly_to_fmpz_poly(poly3, r_poly3);
         fmpz_poly_mul(poly1, poly1, poly3);
         fmpz_poly_mul(poly2, poly2, poly3);
      }
       prof_start();
       fmpz_poly_gcd_subresultant(poly4, poly1, poly2);
       prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   mpz_poly_clear(r_poly3);
   
   fmpz_poly_clear(poly4);
   fmpz_poly_clear(poly3);
   fmpz_poly_clear(poly2);
   fmpz_poly_clear(poly1);
   
}


char* profDriverString_fmpz_poly_gcd_subresultant(char* params)
{
   return "fmpz_poly_gcd_subresultant over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_gcd_subresultant()
{
   return "400000 1.2";
}

void profDriver_fmpz_poly_gcd_subresultant(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);
   
   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_gcd_subresultant);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}

//============================================================================

void sample_fmpz_poly_gcd_modular(unsigned long length, unsigned long bits,
                             void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3, poly4;
   mpz_poly_t r_poly, r_poly2, r_poly3;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_init(r_poly3); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
   mpz_poly_realloc(r_poly3, length);
  
   fmpz_poly_init(poly1);
   fmpz_poly_init(poly2);
   fmpz_poly_init(poly3);
   fmpz_poly_init(poly4);
    
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
         randpoly(r_poly3, length, bits);
         mpz_poly_to_fmpz_poly(poly3, r_poly3);
         fmpz_poly_mul(poly1, poly1, poly3);
         fmpz_poly_mul(poly2, poly2, poly3);
      }
       prof_start();
       fmpz_poly_gcd_modular(poly4, poly1, poly2, 0, 0);
       prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   mpz_poly_clear(r_poly3);
   
   fmpz_poly_clear(poly4);
   fmpz_poly_clear(poly3);
   fmpz_poly_clear(poly2);
   fmpz_poly_clear(poly1);
   
}

char* profDriverString_fmpz_poly_gcd_modular(char* params)
{
   return "fmpz_poly_gcd_modular over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_gcd_modular()
{
   return "100000 1.2";
}

void profDriver_fmpz_poly_gcd_modular(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);
   
   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_gcd_modular);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}

//============================================================================

void sample_fmpz_poly_gcd_heuristic(unsigned long length, unsigned long bits,
                             void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3, poly4;
   mpz_poly_t r_poly, r_poly2, r_poly3;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_init(r_poly3); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
   mpz_poly_realloc(r_poly3, length);
  
   fmpz_poly_init(poly1);
   fmpz_poly_init(poly2);
   fmpz_poly_init(poly3);
   fmpz_poly_init(poly4);
    
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
         randpoly(r_poly3, length, bits);
         mpz_poly_to_fmpz_poly(poly3, r_poly3);
         fmpz_poly_mul(poly1, poly1, poly3);
         fmpz_poly_mul(poly2, poly2, poly3);
      }
       prof_start();
       if (!fmpz_poly_gcd_heuristic(poly4, poly1, poly2, 0, 0))
          fmpz_poly_gcd_modular(poly4, poly1, poly2, 0, 0);
       prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   mpz_poly_clear(r_poly3);
   
   fmpz_poly_clear(poly4);
   fmpz_poly_clear(poly3);
   fmpz_poly_clear(poly2);
   fmpz_poly_clear(poly1);
   
}

char* profDriverString_fmpz_poly_gcd_heuristic(char* params)
{
   return "fmpz_poly_gcd_heuristic over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_gcd_heuristic()
{
   return "100000 1.2";
}

void profDriver_fmpz_poly_gcd_heuristic(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);
   
   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_gcd_heuristic);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}

//============================================================================

void sample_fmpz_poly_gcd(unsigned long length, unsigned long bits,
                             void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3, poly4;
   mpz_poly_t r_poly, r_poly2, r_poly3;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_init(r_poly3); 
   mpz_poly_realloc(r_poly, length);
   mpz_poly_realloc(r_poly2, length);
   mpz_poly_realloc(r_poly3, length);
  
   fmpz_poly_init(poly1);
   fmpz_poly_init(poly2);
   fmpz_poly_init(poly3);
   fmpz_poly_init(poly4);
    
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, length, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
         randpoly(r_poly3, length, bits);
         mpz_poly_to_fmpz_poly(poly3, r_poly3);
         fmpz_poly_mul(poly1, poly1, poly3);
         fmpz_poly_mul(poly2, poly2, poly3);
      }
       prof_start();
       fmpz_poly_gcd(poly4, poly1, poly2);
       prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   mpz_poly_clear(r_poly3);
   
   fmpz_poly_clear(poly4);
   fmpz_poly_clear(poly3);
   fmpz_poly_clear(poly2);
   fmpz_poly_clear(poly1);
   
}


char* profDriverString_fmpz_poly_gcd(char* params)
{
   return "fmpz_poly_gcd over various lengths and various bit sizes.\n"
   "Parameters are: max bitsize; ratio between consecutive lengths/bitsizes.";
}

char* profDriverDefaultParams_fmpz_poly_gcd()
{
   return "100000 1.2";
}

void profDriver_fmpz_poly_gcd(char* params)
{
   unsigned long max_bits;
   double ratio;

   sscanf(params, "%ld %lf", &max_bits, &ratio);
   
   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_gcd);
   run_triangle(max_bits, ratio);
   test_support_cleanup();
}

//=============================================================================

void sample_fmpz_poly_compose(unsigned long length, unsigned long bits,
                          void* arg, unsigned long count)
{
   unsigned long m = ceil_log2(length);
   unsigned long output_bits = 2*bits+m;
   
   fmpz_poly_t poly1, poly2, poly3;
   mpz_poly_t r_poly, r_poly2;  
   
   mpz_poly_init(r_poly); 
   mpz_poly_init(r_poly2); 
   mpz_poly_realloc(r_poly, bits);
   mpz_poly_realloc(r_poly2, length);
  
   fmpz_poly_init(poly1);
   fmpz_poly_init(poly2);
   fmpz_poly_init(poly3);
   
   unsigned long r_count;    // how often to generate new random data
   
   if (count >= 1000) r_count = 100;
   else if (count >= 100) r_count = 10;
   else if (count >= 20) r_count = 5;
   else if (count >= 8) r_count = 2;
   else r_count = 1;
   
   unsigned long i;
   for (i = 0; i < count; i++)
   {
      if (i%r_count == 0)
      {
         randpoly(r_poly, bits, bits);
         mpz_poly_to_fmpz_poly(poly1, r_poly);
         randpoly(r_poly2, length, bits);
         mpz_poly_to_fmpz_poly(poly2, r_poly2);
      }
       prof_start();
       fmpz_poly_compose(poly3, poly2, poly1);
       prof_stop();
   }
   
   mpz_poly_clear(r_poly);
   mpz_poly_clear(r_poly2);
   
   fmpz_poly_clear(poly3);
   fmpz_poly_clear(poly2);
   fmpz_poly_clear(poly1);
}




char* profDriverString_fmpz_poly_compose(char* params)
{
   return "fmpz_poly_compose over various lengths and various bit sizes.\n"
   "Parameters are: max length/bitsize; ratio between consecutive lengths/bitsizes.";
}
 
char* profDriverDefaultParams_fmpz_poly_compose()
{
   return "1280 2.0";
}
 
void profDriver_fmpz_poly_compose(char* params)
{
   unsigned long max_bits;
   double ratio;
    
   sscanf(params, "%ld %lf", &max_bits, &ratio);
 
   test_support_init();
   prof2d_set_sampler(sample_fmpz_poly_compose);
   run_square_triangle(max_bits, ratio);
   test_support_cleanup();
}



// end of file ****************************************************************
