/*********************************************************************
 *   Copyright 1993, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/
#ifndef NCHASHMAP_H
#define NCHASHMAP_H 1

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
extern "C" {
#endif

#include "nclist.h"

/* Define the type of the hash key in the hashmap*/
typedef unsigned int nchashid;

/*
This hash table implementation
uses a collision chain overflow mechanism.
The actual table is fixed size at creation.
Basically we keep alloc entry structs,
where each entry contains the hashid plus
a pointer to the data.
*/

typedef struct NChashmap {
  size_t alloc; /* # of chains */
  size_t size; /* # of pairs still in table*/
  struct NCentry {
    nchashid id; /* all entries in this chain have same id */
    NClist* chain; /* chain of collisions for this entry */
  }* table; /*|table| == alloc*/
} NChashmap;

/* Return error codes */
typedef enum NChasherr {
NCH_OK=0,
NCH_ENOMEM=(-1), /*Out of memory */
NCH_EINVAL=(-2), /*Malformed table */
NCH_EINDEX=(-3), /*No more chains */
} NChasherr;

/* Create a table with some fixed size */
extern NChasherr nchashnew(unsigned int,NChashmap**);

/* Reclaim table */
extern NChasherr nchashfree(NChashmap*);

/* Insert a (ncnchashid,void*) pair into the table;
   duplicates are the caller's worry =>
   the collision chain may have duplicate entries.
*/
extern NChasherr nchashinsert(NChashmap*, nchashid nchash, void* value);

/* lookup a nchashid and return NULL | list of matching data */
extern NChasherr nchashlookup(NChashmap*, nchashid nchash, NClist** datap);

/* remove a (nchashid,value) pair*/
extern NChasherr nchashremove(NChashmap*, nchashid nchash, void* value);

/* Return the ith collision chain; order is completely arbitrary.
   Can be expensive.
   Return NCH_EINDEX to tell client to stop.
*/
extern NChasherr nchashith(NChashmap*, int i, NClist** chainp);

/* return the # of pairs in table*/
#define nchashsize(hm) ((hm)?(hm)->size:0)

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
}
#endif

#endif /*NCHASHMAP_H*/

