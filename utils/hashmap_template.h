/*********************************************************************
 *   Copyright 1993, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/
#ifndef XXHASHMAP_H
#define XXHASHMAP_H 1

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
#define externC extern "C"
#else
#define externC extern
#endif

#include "xxlist.h"

/* Define the type of the elements in the hashmap*/
typedef unsigned long xxhashid;

externC int xxhashnull(xxelem);

typedef struct XXhashmap {
  int alloc;
  int size; /* # of pairs still in table*/
  XXlist** table;
} XXhashmap;

externC XXhashmap* xxhashnew(void);
externC XXhashmap* xxhashnew0(int);
externC int xxhashfree(XXhashmap*);

/* Insert a (xxxxhashid,xxelem) pair into the table*/
/* Fail if already there*/
externC int xxhashinsert(XXhashmap*, xxhashid xxhash, xxelem value);

/* Insert a (xxhashid,xxelem) pair into the table*/
/* Overwrite if already there*/
externC int xxhashreplace(XXhashmap*, xxhashid xxhash, xxelem value);

/* lookup a xxhashid and return found/notfound*/
externC int xxhashlookup(XXhashmap*, xxhashid xxhash, xxelem* valuep);

/* lookup a xxhashid and return 0 or the value*/
externC xxelem xxhashget(XXhashmap*, xxhashid xxhash);

/* remove a xxhashid*/
externC int xxhashremove(XXhashmap*, xxhashid xxhash);

/* Return the ith pair; order is completely arbitrary*/
/* Can be expensive*/
externC int xxhashith(XXhashmap*, int i, xxhashid*, xxelem*);

externC int xxhashkeys(XXhashmap* hm, xxhashid** keylist);

/* return the # of pairs in table*/
#define xxhashsize(hm) ((hm)?(hm)->size:0)

#endif /*XXHASHMAP_H*/

