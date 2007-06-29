/****************************************************************************

ZmodF_mul-profile.c

Profiling for ZmodF_mul

Copyright (C) 2007, William Hart and David Harvey

*****************************************************************************/

#include "profiler-main.h"
#include "ZmodF_mul.h"
#include "flint.h"
#include <string.h>
#include <math.h>


/*
This is a helper function used by the other sampler functions below.
*/
void sample_ZmodF_mul_helper(ZmodF_mul_info_t info, unsigned long n,
                             unsigned long count)
{
   mp_limb_t* x1 = (mp_limb_t*) malloc((n+1) * sizeof(mp_limb_t));
   mp_limb_t* x2 = (mp_limb_t*) malloc((n+1) * sizeof(mp_limb_t));
   mp_limb_t* x3 = (mp_limb_t*) malloc((n+1) * sizeof(mp_limb_t));

   profiler_random_limbs(x1, n);
   x1[n] = 0;
   profiler_random_limbs(x2, n);
   x2[n] = 0;
   
   prof_start();

   for (unsigned long i = 0; i < count; i++)
      ZmodF_mul_info_mul(info, x3, x1, x2);

   prof_stop();

   free(x3);
   free(x2);
   free(x1);
}


// ============================================================================


void sample_ZmodF_mul_plain(unsigned long n, void* arg, unsigned long count)
{
   ZmodF_mul_info_t info;
   ZmodF_mul_info_init_plain(info, n, 0);
   sample_ZmodF_mul_helper(info, n, count);
   ZmodF_mul_info_clear(info);
}


char* profDriverString_ZmodF_mul_plain(char* params)
{
   return
   "ZmodF_mul using plain algorithm.\n"
   "Parameters: n_min, n_max, n_skip.\n";
}


char* profDriverDefaultParams_ZmodF_mul_plain()
{
   return "1 1000 1";
}


void profDriver_ZmodF_mul_plain(char* params)
{
   unsigned long n_min, n_max, n_skip;
   sscanf(params, "%ld %ld %ld", &n_min, &n_max, &n_skip);

   prof1d_set_sampler(sample_ZmodF_mul_plain);
   
   for (unsigned long n = n_min; n <= n_max; n += n_skip)
      prof1d_sample(n, NULL);
}



// ============================================================================


void sample_ZmodF_mul_threeway(unsigned long n, void* arg, unsigned long count)
{
   ZmodF_mul_info_t info;
   ZmodF_mul_info_init_threeway(info, n, 0);
   sample_ZmodF_mul_helper(info, n, count);
   ZmodF_mul_info_clear(info);
}


char* profDriverString_ZmodF_mul_threeway(char* params)
{
   return
   "ZmodF_mul using threeway algorithm.\n"
   "Parameters: n_min, n_max, n_skip.\n"
   "Note: n not divisible by 3 are skipped.\n";
}


char* profDriverDefaultParams_ZmodF_mul_threeway()
{
   return "1 1000 1";
}


void profDriver_ZmodF_mul_threeway(char* params)
{
   unsigned long n_min, n_max, n_skip;
   sscanf(params, "%ld %ld %ld", &n_min, &n_max, &n_skip);

   prof1d_set_sampler(sample_ZmodF_mul_threeway);
   
   // round up n_min so we start on a permissible value
   while (n_min % 3)
      n_min++;
   
   for (unsigned long n = n_min; n <= n_max; n += n_skip)
   {
      if (n % 3 == 0)
         prof1d_sample(n, NULL);
   }
}


// ============================================================================


/*
arg should point to an unsigned long, which is the transform depth
*/
void sample_ZmodF_mul_negacyclic(unsigned long n, void* arg,
                                 unsigned long count)
{
   unsigned long depth = * (unsigned long*) arg;
   ZmodF_mul_info_t info;
   ZmodF_mul_info_init_negacyclic(info, n, depth, 0);
   sample_ZmodF_mul_helper(info, n, count);
   ZmodF_mul_info_clear(info);
}


char* profDriverString_ZmodF_mul_negacyclic(char* params)
{
   return
   "ZmodF_mul using negacyclic algorithm.\n"
   "Parameters: depth, n_min, n_max, n_skip.\n"
   "Note: those n not supported for the given depth are skipped.\n";
}


char* profDriverDefaultParams_ZmodF_mul_negacyclic()
{
   return "4 1 1000 1";
}


void profDriver_ZmodF_mul_negacyclic(char* params)
{
   unsigned long depth, n_min, n_max, n_skip;
   sscanf(params, "%ld %ld %ld %ld", &depth, &n_min, &n_max, &n_skip);

   prof1d_set_sampler(sample_ZmodF_mul_negacyclic);

   // round up n_min so we start on a permissible value
   while ((n_min * FLINT_BITS) & ((1 << depth) - 1))
      n_min++;
   
   for (unsigned long n = n_min; n <= n_max; n += n_skip)
   {
      if (((n * FLINT_BITS) & ((1 << depth) - 1)) == 0)
         prof1d_sample(n, &depth);
   }
}



// ============================================================================


/*
arg should point to an unsigned long, which is the transform depth
*/
void sample_ZmodF_mul_negacyclic2(unsigned long n, void* arg,
                                  unsigned long count)
{
   unsigned long depth = * (unsigned long*) arg;
   ZmodF_mul_info_t info;
   ZmodF_mul_info_init_negacyclic2(info, n, depth, 0);
   sample_ZmodF_mul_helper(info, n, count);
   ZmodF_mul_info_clear(info);
}


char* profDriverString_ZmodF_mul_negacyclic2(char* params)
{
   return
   "ZmodF_mul using 2nd negacyclic algorithm.\n"
   "Parameters: depth, n_min, n_max, n_skip.\n"
   "Note: those n not supported for the given depth are skipped.\n";
}


char* profDriverDefaultParams_ZmodF_mul_negacyclic2()
{
   return "4 1 1000 1";
}


void profDriver_ZmodF_mul_negacyclic2(char* params)
{
   unsigned long depth, n_min, n_max, n_skip;
   sscanf(params, "%ld %ld %ld %ld", &depth, &n_min, &n_max, &n_skip);

   prof1d_set_sampler(sample_ZmodF_mul_negacyclic2);

   // round up n_min so we start on a permissible value
   while ((n_min * FLINT_BITS) & ((1 << depth) - 1))
      n_min++;
   
   for (unsigned long n = n_min; n <= n_max; n += n_skip)
   {
      if (((n * FLINT_BITS) & ((1 << depth) - 1)) == 0)
         prof1d_sample(n, &depth);
   }
}


// ============================================================================


void sample_ZmodF_mul_auto(unsigned long n, void* arg, unsigned long count)
{
   ZmodF_mul_info_t info;
   ZmodF_mul_info_init(info, n, 0);
   sample_ZmodF_mul_helper(info, n, count);
   ZmodF_mul_info_clear(info);
}


char* profDriverString_ZmodF_mul_auto(char* params)
{
   return
   "ZmodF_mul using automatically selected algorithm.\n"
   "Parameters: n_min, n_max, n_skip.\n";
}


char* profDriverDefaultParams_ZmodF_mul_auto()
{
   return "1 1000 1";
}


void profDriver_ZmodF_mul_auto(char* params)
{
   unsigned long n_min, n_max, n_skip;
   sscanf(params, "%ld %ld %ld", &n_min, &n_max, &n_skip);

   prof1d_set_sampler(sample_ZmodF_mul_auto);
   
   for (unsigned long n = n_min; n <= n_max; n += n_skip)
      prof1d_sample(n, NULL);
}



// end of file ****************************************************************
