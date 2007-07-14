/*
   mpz_poly-tune
   
   Program for tuning the mpz_poly module.
   
   This program writes to standard output an automatically tuned version of
   mpz_poly-tuning.c.
   
   (If DEBUG is set, it also writes logging info to standard error.)
   
   (C) 2007 David Harvey and William Hart
*/

#include <stdio.h>
#include <math.h>
#include "flint.h"
#include "test-support.h"
#include "profiler.h"
#include "mpz_poly.h"
#include "mpz_poly-tuning.h"


#define DEBUG 1


typedef struct
{
   unsigned long length;
   unsigned long limbs;
   unsigned long crossover;
} sample_kara_t;


// arg should point to a sample_kara_t
void sample_kara(void* arg, unsigned long count)
{
   unsigned long length = ((sample_kara_t*) arg)->length;
   unsigned long limbs = ((sample_kara_t*) arg)->limbs;
   unsigned long crossover = ((sample_kara_t*) arg)->crossover;

   mpz_t* buf;
   buf = (mpz_t*) malloc(6 * length * sizeof(mpz_t));
   for (unsigned long i = 0; i < 6*length; i++)
      mpz_init2(buf[i], 3*limbs*FLINT_BITS);
      
   for (unsigned long i = 0; i < 2*length; i++)
      // (leave a few zero high bits to prevent carries in the multiplication)
      mpz_urandomb(buf[i], randstate, limbs*FLINT_BITS - FLINT_BITS/3);
      
   mpz_t* in1 = buf;
   mpz_t* in2 = in1 + length;
   mpz_t* out = in2 + length;
   mpz_t* scratch = out + length;

   // warm up
   for (unsigned long i = 0; i < count/4; i++)
      _mpz_poly_mul_kara_recursive(out, in1, length, in2, length,
                                   scratch, 1, crossover);

   // time it
   start_clock(0);
   for (unsigned long i = 0; i < count; i++)
      _mpz_poly_mul_kara_recursive(out, in1, length, in2, length,
                                   scratch, 1, crossover);
   stop_clock(0);

   for (unsigned long i = 0; i < 6*length; i++)
      mpz_clear(buf[i]);
   free(buf);
}


/*
   Compares:

   * one layer of karatubsa followed by classical multiplication, vs
   * straight classical multiplication,

   for a given polynomial length and coefficient size, using the
   _mpz_poly_mul_kara_recursive() function.
   
   Returns nonzero if the first strategy wins.
*/
int compare_kara(unsigned long length, unsigned long limbs, FILE* f)
{
   double time1, time2;
   
   sample_kara_t info;
   info.length = length;
   info.limbs = limbs;

   // try with one layer of karatsuba
   info.crossover = length*length;
   prof_repeat(&time1, NULL, sample_kara, &info);
   
   // try with plain classical
   info.crossover = 2*length*length;
   prof_repeat(&time2, NULL, sample_kara, &info);
   
#if DEBUG
   fprintf(f, "length = %ld, limbs = %ld, %s wins (%lf vs %lf)\n",
           length, limbs, (time1 < time2) ? "karatsuba" : "classical" ,
           FLINT_MIN(time1, time2), FLINT_MAX(time1, time2));
#endif

   return time1 < time2;
}



/*
Finds crossover length for switching from classical to karatsuba multiplication
for the given coefficient length.
*/
unsigned long crossover_kara(unsigned long limbs, FILE* f)
{
   for (unsigned long length = 2; ; length++)
      // if karatsuba seems to win, run it twice just to check
      if (compare_kara(length, limbs, f) && compare_kara(length, limbs, f))
         return length;
}


int main(int argc, char* argv[])
{
   FILE* fout = stdout;
   FILE* flog = stderr;

   test_support_init();

   fprintf(fout, "/*\n");
   fprintf(fout, "   Tuning values for mpz_poly module\n");
   fprintf(fout, "\n");
   fprintf(fout, "   Automatically generated by mpz_poly-tune program\n");
   fprintf(fout, "*/\n\n");
   fprintf(fout, "#include \"mpz_poly-tuning.h\"\n");
   fprintf(fout, "#include \"mpz_poly.h\"\n");
   fprintf(fout, "\n");
   fprintf(fout, "unsigned long mpz_poly_kara_crossover_table[] = {");
   fflush(fout);

   unsigned long limbs;
   for (limbs = 1; ; limbs++)
   {
      unsigned long crossover = crossover_kara(limbs, flog);
      if (crossover == 2)
         break;
      fprintf(fout, "%ld, ", crossover);
      fflush(fout);
   }
   fprintf(fout, "0};\n");
   fprintf(fout, "unsigned long mpz_poly_kara_crossover_table_size = %ld;\n",
           limbs);
   
   fprintf(fout, "\n\n");
   fprintf(fout, "// end of file *********************************\n");

   test_support_cleanup();
   return 0;
}



// end of file ****************************************************************