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

 Command-line profiling utility

 (C) 2007 William Hart and David Harvey

******************************************************************************/

#ifdef __cplusplus
 extern "C" {
#endif
 
#include "profiler.h"


// A function that takes one/two coordinates, an implementation-defined
// argument, and an iteration count
// (i.e. the type of function that is getting profiled).
typedef void (*prof1d_Sampler_t)(unsigned long x, void* arg,
                                 unsigned long count);
typedef void (*prof2d_Sampler_t)(unsigned long x, unsigned long y, void* arg,
                                 unsigned long count);


// A function that runs a bunch of profiles
typedef void (*prof_Driver_t)(char* params);


// A function that returns a string (the description of the target)
typedef char* (*prof_DriverString_t)(char* params);

// A function that returns a string (the default parameters for this target)
typedef char* (*prof_DriverDefaultParams_t)();


void prof2d_set_sampler(prof2d_Sampler_t sampler);
void prof2d_sample(unsigned long x, unsigned long y, void* arg);
void prof1d_set_sampler(prof1d_Sampler_t sampler);
void prof1d_sample(unsigned long x, void* arg);

void prof_start();
void prof_stop();


/*
Generates count random unsigned limbs, stores them at output
*/
void profiler_random_limbs(unsigned long* output, unsigned long count);


/* ============================================================================

   Imported data from the auto-generated table file
   
   (See make-profile-tables.py.)
   
=============================================================================*/

// name of module being profiled.
extern char* prof_module_name;


extern int prof_target_count;

extern char* prof_target_name[];
extern prof_Driver_t prof_Driver_list[];
extern prof_DriverString_t prof_DriverString_list[];
extern prof_DriverDefaultParams_t prof_DriverDefaultParams_list[];

#ifdef __cplusplus
 }
#endif
 
// end of file ****************************************************************
