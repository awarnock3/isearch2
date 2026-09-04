/*
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * Author:	Ray Larson, ray@sherlock.berkeley.edu
 *		School of Library and Information Studies, UC Berkeley
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**************************************************************************
* MemCNTL.c - This module handles all allocation and de-allocation of
* memory
**************************************************************************/
// ISEARCH2-CLEANUP: processed 2026-08-06
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <stdio.h>
#include <stdlib.h>
#include <new>
#include "gdt.h"
#include "memcntl.hxx"
/* includes the MemBlock structure declaration */

/**************************************************************************/
/* AllocSafe - Allocate memory placing all allocated blocks into a linked */
/* list of MemBlocks                                                      */
/**************************************************************************/
char *AllocSafe(struct MemBlock **base,INT4 size,INT4 flags,INT4 type)
{
  struct MemBlock *block;
  int i;
  char *mem;

  // BUGFIX #2: was plain `new`, which throws std::bad_alloc on failure
  // instead of returning nullptr -- making the `if (block)`/`if (mem)`
  // checks below (and the "not enough memory" diagnostics they guard)
  // permanently unreachable dead code, and letting an exception escape
  // this extern "C" function instead of the graceful failure this code,
  // and its callers in src/marclib.cxx (which check AllocSafe's return
  // value), were written to expect. `nothrow` restores that contract.
  block = new (std::nothrow) MemBlock;
  if (block)
    { /* store the block in a pushdown stack */
      if (*base == nullptr)
         {*base = block;
          block->nextmem = nullptr;
         }
      else { block->nextmem = *base;
             *base = block;
           }
      block->memtype = type;
      block->memsize = size;
      block->data = new (std::nothrow) char[size];
      mem = block->data;
      if (mem)
         {if (flags & MEMF_CLEAR)
             for(i=0;i<size;i++) *mem++ = '\0'; /* zero out the memory */
          return(block->data);
         }
      else {
         // BUGFIX #2: previously left `block` linked into the list with
         // a null `data` on this failure path, a zombie node later
         // traversal (e.g. FreeSafe) would have to contend with. `block`
         // is always inserted at the head just above, so unlinking it
         // here is just restoring *base to what it was before this call.
         *base = block->nextmem;
         delete block;
         fprintf(stderr,"memcntl: Not enough memory for new data");
         return(nullptr);
       }
     }
  else { fprintf(stderr,"memcntl: Not enough memory for new control structure");
         return(nullptr);
       }
}


/**************************************************************************/
/* FreeSafe - free the data and memblock associated with the supplied     */
/* pointer, and patch up the memblock list                                */
/**************************************************************************/
int FreeSafe(struct MemBlock **base,char *mem,int flag)
{
  struct MemBlock *prev, *curr, *next;

  /* if nothing is allocated , just return */
  if (*base == nullptr) return (0);
  if (flag == 0 && mem == nullptr) return (0);

  prev = *base;
  curr = *base;

  do { next = curr->nextmem;

       if (flag) /* free all memory */
         {
	   delete [] curr->data;
	   delete curr;
           curr = next;
         }
       else
         {
           if (curr->data == mem)
             { if (curr == *base) *base = next;
               else prev->nextmem = next;
	     delete [] curr->data;
	     delete curr;
               return (0);
             }
           else
             { prev = curr;
               curr = next;
             }
         }
      } while (curr);

  // BUGFIX #3: was missing -- after the flag!=0 ("free everything") loop
  // above deletes every node, *base still pointed at the first (now
  // deleted) block instead of nullptr, a dangling pointer. Confirmed
  // real with a standalone repro: free-all, allocate a new block (which
  // silently links its ->nextmem to the dangling value while patching
  // *base back to something valid), then free-all again -- the second
  // pass eventually reaches the dangling pointer, reads its ->nextmem
  // (heap-use-after-free), and deletes it a second time (double-free).
  // Not triggered by any real caller today (the only call site in this
  // tree always passes flag=0), but a real bug in the flag!=0 path
  // regardless. Fixed by nulling *base once every node is gone -- the
  // flag==0 (single-node) path already correctly patches *base/
  // prev->nextmem itself and returns early, so this only affects the
  // free-everything case.
  if (flag) {
    *base = nullptr;
  }

  return(0);
}

