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

memory-manager.c: FLINT-wide memory management

Copyright (C) 2007, William Hart and David Harvey

*****************************************************************************/

#include <stdlib.h>
#include <gmp.h>
#include <string.h>
#include <stdio.h>
#include "flint.h"
#include "memory-manager.h"

#define DEBUG 0 // Switches to debugging memory allocators
#define DEBUG_PRINT 0 // Prints info about all allocations and releases

#if DEBUG

/*-----------------------------------------------------------------------------------------------
 
    Debug version of stack based memory managers
    
------------------------------------------------------------------------------------------------*/

void * mempts[200000];
unsigned long upto = 0;

void * flint_stack_alloc(unsigned long length)
{
#if DEBUG_PRINT
   printf("Allocating %ld limbs\n", length);
#endif
   if (upto == 200000) 
   {
      printf("Error: no free stack nodes in flint_stack_alloc\n");
      abort();         
   }
   unsigned long * block = malloc((length+101)*sizeof(unsigned long));
   if (block == NULL)
   {
      printf("Error: unable to allocate memory in flint_stack_alloc\n");
      abort();         
   }
   mempts[upto] = (void*) block;
   upto++;
   block[0] = length;
   unsigned long i;
   for (i = 0; i < 100; i++)
      block[length+i+1] = 0;
   return (void*) (block+1);
}

void * flint_stack_alloc_bytes(unsigned long bytes)
{
   unsigned long length = ((bytes-1)>>FLINT_LG_BYTES_PER_LIMB)+1;
#if DEBUG_PRINT
   printf("Allocating %ld limbs\n", length);
#endif
   if (upto == 200000) 
   {
      printf("Error: no free stack nodes in flint_stack_alloc_bytes\n");
      abort();         
   }
   unsigned long * block = malloc(sizeof(unsigned long)*(length + 101));
   if (block == NULL)
   {
      printf("Error: unable to allocate memory in flint_stack_alloc_bytes\n");
      abort();         
   }
   mempts[upto] = (void*) block;
   upto++;
   block[0] = length;
   unsigned long i;
   for (i = 0; i < 100; i++)
      block[length+i+1] = 0L;
   return (void*) (block+1);
}

void flint_stack_release()
{
   if (upto == 0) 
   {
      printf("Error: attempt to free unallocated block in flint_stack_release\n");
      abort();         
   }
   upto--;
   unsigned long * block = mempts[upto];
   unsigned long length = block[0];
#if DEBUG_PRINT
   printf("Releasing %ld limbs\n", length);
#endif
   unsigned long i;
   for (i = 0; i < 100; i++)
      if (block[length+i+1] != 0L) 
      {
         printf("Error: Block overrun detected by stack memory allocator!!\n");
         abort();
      }
   free(mempts[upto]);
}

void * flint_stack_alloc_small(unsigned long length)
{
   return flint_stack_alloc(length);
}

void flint_stack_release_small(void)
{
   flint_stack_release();
}

void flint_stack_cleanup(void)
{
   if (upto) printf("Error: stack allocator detected mismatch on cleanup!\n"); 
}

#else

/*
   Stack based memory manager to allocate an array of limbs of the given length. 
   It returns a (void*) which needs to be typecast to the required object, 
   e.g. mp_limb_t* or mp_limb_t**, etc.
   
   Limbs must be released in the reverse order to that in which they were allocated. 
*/

#define EXPIRE_AFTER 3 // 1/4-th number of allocs before expiring unused allocated space
#define RESALLOC 100 //allocate this many mp_limb_t's at once to save on overheads

typedef struct limb_mem_t //record for managing all allocations of limbs of a certain bitsize
{
   unsigned long remaining; //number of remaining limbs which are available for allocation
   unsigned long length; //total length of array of limbs this record manages
   mp_limb_t* point; //pointer to the next available mp_limb_t
   int expire; //how long till we expire
   int allocated; //1 if some of the space is allocated, 0 otherwise (will not expire if allocated)
   struct limb_mem_t* next; //next record
   struct limb_mem_t* prev; //previous record
} limb_mem_t;

typedef struct limb_memp_t //record for managing a particular allocation of memory for a limb array
{
   limb_mem_t* point; //which record controls this allocation
   unsigned long length; //how many limbs allocated
} limb_memp_t;

THREAD limb_mem_t* head_mpn = NULL; //start of doubly linked list of records
THREAD limb_mem_t* last_mpn = NULL; //last record in linked list 
THREAD limb_memp_t* top_mpn = NULL; //top of stack of limb_memp_t's
THREAD limb_memp_t* reservoir_mpn; //array of preallocated limb_memp_t's
THREAD unsigned int rescount_mpn = 0; //counter for which limb_memp_t we are upto in the reservoir_mpn
   
// todo: deal with possible out of memory situation when allocating

void* flint_stack_alloc(unsigned long length)
{
   static THREAD limb_mem_t* curr;
   static THREAD limb_mem_t* temp;
   static THREAD int initialised = 0; //has limb_alloc been initialised
   static THREAD unsigned int currentalloc = 0; //total number of limb_memp_t's in reservoir_mpn
   static THREAD limb_memp_t* tempres;
   static THREAD int check=0;
   void* alloc_d;
   
   check++;
   //allocate another block of limb_memp_t's if none are currently allocated, or the reservoir_mpn is depleted
   if (rescount_mpn==currentalloc) // need more limb_memp_t's
   {
      if (!initialised) 
      {
         reservoir_mpn = (limb_memp_t*)malloc(RESALLOC*sizeof(limb_memp_t));
         rescount_mpn=0;
         initialised = 1;
         currentalloc = RESALLOC;
      } else
      {
         //copy old reservoir_mpn into larger one
         tempres = reservoir_mpn;
         reservoir_mpn = (limb_memp_t*)malloc((currentalloc+RESALLOC)*sizeof(limb_memp_t));  
         memcpy(reservoir_mpn,tempres,currentalloc*sizeof(limb_memp_t)); 
         currentalloc+=RESALLOC;
         //free old reservoir_mpn
         free(tempres);  
      }       
   }
   
   curr = head_mpn;
   if (curr != NULL)
   {
      do 
      {
         //search for data of requested size
         if ((curr->remaining >= length) && (curr->remaining < 2*length))//appropriate unallocated space found, so allocate it
         {
             alloc_d = (void*)curr->point;
             curr->point+=length;
             curr->remaining-=length;
             curr->allocated=1;
             top_mpn=&reservoir_mpn[rescount_mpn];
             top_mpn->point=curr;
             top_mpn->length=length;
             
             //check if any remaining nodes have expired, expire them and return
             if ((check&3)==0)
             {
               do 
               {
                 if (!curr->allocated) 
                 {
                    curr->expire--;
                    if (curr->expire == 0) 
                    {
                       free(curr->point);
                       if (curr==last_mpn) last_mpn = curr->prev;
                       else curr->next->prev = curr->prev;
                       if (curr==head_mpn) head_mpn=curr->next;
                       else (curr->prev->next = curr->next);
                       temp=curr;
                       curr = curr->next;
                       free(temp);
                    } else curr = curr->next;
                 } else curr = curr->next;
               } while (curr != NULL);
             }
             rescount_mpn++;
             return alloc_d;
         }
         //update expiry information for curr if necessary and possibly expire space
         if (((check&3)==0)&&(!curr->allocated))
         {
            curr->expire--;
            if (curr->expire == 0) 
            {
               free(curr->point);
               if (curr==last_mpn) last_mpn = curr->prev;
               else curr->next->prev = curr->prev;
               if (curr==head_mpn) head_mpn=curr->next;
               else (curr->prev->next = curr->next);
               temp=curr;
               curr = curr->next;
               free(temp);
            } else curr = curr->next;
         } else curr = curr->next;
      } while (curr != NULL);
      //Nothing suitable found, so initialise data of the requested type
      //attach to last_mpn->next
      alloc_d = malloc(length*sizeof(mp_limb_t));
      //set up the new record, and last_mpn to point to this new record
      last_mpn->next = (limb_mem_t*) malloc(sizeof(limb_mem_t));
      last_mpn->next->prev = last_mpn;
      last_mpn=last_mpn->next;
      last_mpn->point = (mp_limb_t*)alloc_d+length;
      last_mpn->next = NULL;
      last_mpn->remaining=0;
      last_mpn->allocated=1;
      last_mpn->length=length;
      top_mpn=&reservoir_mpn[rescount_mpn];
      top_mpn->point=last_mpn;
      top_mpn->length=length;
      rescount_mpn++;
      return alloc_d;
   } 
   /*first time anything has been allocated
     so do the actual allocation of limbs*/
   //set up the new record, and head_mpn and last_mpn to point to this single new record
   alloc_d = malloc(length*sizeof(mp_limb_t));
   head_mpn = (limb_mem_t*) malloc(sizeof(limb_mem_t));
   head_mpn->point = (mp_limb_t*)alloc_d+length;
   head_mpn->next = NULL;
   head_mpn->prev = NULL;
   head_mpn->remaining=0;
   head_mpn->allocated=1;
   head_mpn->length=length;
   last_mpn = head_mpn;
   top_mpn=&reservoir_mpn[rescount_mpn];
   top_mpn->point=head_mpn;
   top_mpn->length=length;
   rescount_mpn++;
   return alloc_d;
}

void* flint_stack_alloc_bytes(unsigned long bytes)
{
   return flint_stack_alloc((bytes-1)/FLINT_BYTES_PER_LIMB+1);
}


void flint_stack_release()
{
    unsigned long length = top_mpn->length;
    
    //adjust record to reflect the fact that the limbs have been released back to the stack
    top_mpn->point->point-=length;
    top_mpn->point->remaining+=length;
    //if no limbs of that size are allocated any more set them to expire if not eventually used
    if (top_mpn->point->remaining == top_mpn->point->length) 
    {
       top_mpn->point->allocated=0;
       top_mpn->point->expire=EXPIRE_AFTER;
    }
    //release limb_memp_t back into reservoir_mpn
    top_mpn--;
    rescount_mpn--;
}


/*-----------------------------------------------------------------------------------------------*/

#define FLINT_SMALL_BLOCK_SIZE 10000L

THREAD mp_limb_t * block_ptr = NULL;
THREAD unsigned long block_left = 0;
   
void * flint_stack_alloc_small(unsigned long length)
{
   if (length + 1L > block_left) // not enough space left, allocate a new block
   {
      if (length + 3L > FLINT_SMALL_BLOCK_SIZE)
      {
         printf("Error: attempt to allocate %ld limbs in small stack memory manager!\n", length);
         abort();
      }
      if (block_ptr == NULL)
      {
         block_ptr = (mp_limb_t *) flint_heap_alloc(FLINT_SMALL_BLOCK_SIZE);
         block_left = FLINT_SMALL_BLOCK_SIZE - 2;
         block_ptr[0] = 0;
         block_ptr[1] = (unsigned long) NULL;
         block_ptr += 2;
      } else
      {
         mp_limb_t * temp = (mp_limb_t *) flint_heap_alloc(FLINT_SMALL_BLOCK_SIZE);
         temp[0] = block_left;
         temp[1] = (unsigned long) block_ptr; 
         block_ptr = temp + 2;
         block_left = FLINT_SMALL_BLOCK_SIZE - 2;
      }
   }
   
   block_ptr[length] = length;
   block_ptr += (length+1L); 
   block_left -= (length+1L);
   return (void *) (block_ptr - (length + 1L)); 
}

void flint_stack_release_small(void)
{
   if (block_left == FLINT_SMALL_BLOCK_SIZE - 2)
   {
      block_ptr -= 2;
      block_left = block_ptr[0];
      mp_limb_t * temp = block_ptr;
      block_ptr = (mp_limb_t *) block_ptr[1]; 
      flint_heap_free(temp);           
   } 
   
   block_ptr--;
   unsigned long temp = (*block_ptr);
   block_left += (temp+1);
   block_ptr -= temp;
}

void flint_stack_cleanup()
{
   limb_mem_t* curr = head_mpn;
   limb_mem_t* temp;
   
   if (curr != NULL)
   {
      do 
      {
         if (curr->allocated) 
         {
            printf("Warning: FLINT stack memory allocation cleanup detected mismatched allocation/releases\n"); 
         } 
         free(curr->point);
         if (curr==last_mpn) last_mpn = curr->prev;
         else curr->next->prev = curr->prev;
         if (curr==head_mpn) head_mpn=curr->next;
         else (curr->prev->next = curr->next);
         temp=curr;
         curr = curr->next;
         free(temp);
      } while (curr != NULL);
      free(reservoir_mpn);
   }
   
   if (block_ptr != NULL)
   {
      if (block_left != FLINT_SMALL_BLOCK_SIZE - 2)
      {
         printf("Warning: FLINT small stack memory allocator detected mismatched alloc/release\n");
         while (block_left != FLINT_SMALL_BLOCK_SIZE - 2) flint_stack_release_small();
      }  
      
      block_ptr -= 2;
      flint_heap_free(block_ptr);           
   } 
}

#endif

/*-----------------------------------------------------------------------------------------------*/

void flint_memory_failure(void)
{
   printf("Error: unable to alloc/realloc memory\n");
   abort();
}
  
#if DEBUG

void* flint_heap_alloc(unsigned long limbs)
{
#if DEBUG_PRINT
   printf("Allocating %ld limbs on heap\n", limbs);
#endif
   unsigned long * buf = malloc((101+limbs) * sizeof(mp_limb_t));
   if (!buf)
      flint_memory_failure();
   buf[0] = limbs;
   unsigned long i;
   for (i = 0; i < 100; i++)
      buf[limbs+i+1] = 0;
   return (void*) (buf+1);
}

void* flint_heap_alloc_bytes(unsigned long bytes)
{
   unsigned long limbs = ((bytes-1)>>FLINT_LG_BYTES_PER_LIMB)+1;
#if DEBUG_PRINT
   printf("Allocating %ld limbs on heap\n", limbs);
#endif
   unsigned long * buf = malloc((101+limbs) * sizeof(mp_limb_t));
   if (!buf)
      flint_memory_failure();
   buf[0] = limbs;
   unsigned long i;
   for (i = 0; i < 100; i++)
      buf[limbs+i+1] = 0;
   return (void*) (buf+1);
}

void* flint_heap_realloc(void * block_void, unsigned long limbs)
{
   unsigned long * block = (unsigned long *) block_void;
   block--;
   unsigned long length = block[0];
   unsigned long i;
   for (i = 0; i < 100; i++)
      if (block[length+i+1] != 0L) 
      {
         printf("Error: Block overrun detected by heap memory (re)allocator!!\n");
         abort();
      }
#if DEBUG_PRINT
   printf("Reallocing from %ld to %ld limbs on heap\n", length, limbs);
#endif
   unsigned long* buf = realloc(block, (limbs+101) * sizeof(mp_limb_t));
   if (!buf)
      flint_memory_failure();
   buf[0] = limbs;
   unsigned long i;
   for (i = 0; i < 100; i++)
      buf[limbs+i+1] = 0;
   return (void*) (buf+1);
}

void* flint_heap_realloc_bytes(void * block_void, unsigned long bytes)
{
   unsigned long * block = (unsigned long *) block_void;
   unsigned long limbs = ((bytes-1)>>FLINT_LG_BYTES_PER_LIMB)+1;
#if DEBUG_PRINT
   printf("Reallocing to %ld limbs on heap\n", limbs);
#endif
   unsigned long* buf = realloc(block-1, (limbs+101) * sizeof(mp_limb_t));
   if (!buf)
      flint_memory_failure();
   buf[0] = limbs;
   unsigned long i;
   for (i = 0; i < 100; i++)
      buf[limbs+i+1] = 0;
   return (void*) (buf+1);
}

void flint_heap_free(void * block_void)
{
   unsigned long * block = (unsigned long *) block_void;
   block--;
   unsigned long length = block[0];
#if DEBUG_PRINT
   printf("Releasing %ld limbs from heap\n", length);
#endif
   unsigned long i;
   for (i = 0; i < 100; i++)
      if (block[length+i+1] != 0L) 
      {
         printf("Error: Block overrun detected by heap memory allocator!!\n");
         abort();
      }
   free(block);
}

#else

void* flint_heap_alloc(unsigned long limbs)
{
   void* buf = malloc(limbs * sizeof(mp_limb_t));
   if (!buf)
      flint_memory_failure();
   return buf;
}

void* flint_heap_alloc_bytes(unsigned long bytes)
{
   void* buf = malloc(bytes);
   if (!buf)
      flint_memory_failure();
   return buf;
}

void* flint_heap_realloc(void* block, unsigned long limbs)
{
   void* buf = realloc(block, limbs * sizeof(mp_limb_t));
   if (!buf)
      flint_memory_failure();
   return buf;
}

void* flint_heap_realloc_bytes(void* block, unsigned long bytes)
{
   void* buf = realloc(block, bytes);
   if (!buf)
      flint_memory_failure();
   return buf;
}

void flint_heap_free(void* block)
{
   free(block);
}

#endif
