/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */
#ifndef XXLIST_H
#define XXLIST_H 1

/* Define the type of the elements in the list*/

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

typedef unsigned long xxelem;

EXTERNC int xxlistnull(xxelem);

typedef struct XXlist {
  unsigned int alloc;
  unsigned int length;
  xxelem* content;
} XXlist;

EXTERNC XXlist* xxlistnew(void);
EXTERNC int xxlistfree(XXlist*);
EXTERNC int xxlistsetalloc(XXlist*,unsigned int);
EXTERNC int xxlistsetlength(XXlist*,unsigned int);

/* Set the ith element */
EXTERNC int xxlistset(XXlist*,unsigned int,xxelem);
/* Get value at position i */
EXTERNC xxelem xxlistget(XXlist*,unsigned int);/* Return the ith element of l */
/* Insert at position i; will push up elements i..|seq|. */
EXTERNC int xxlistinsert(XXlist*,unsigned int,xxelem);
/* Remove element at position i; will move higher elements down */
EXTERNC xxelem xxlistremove(XXlist* l, unsigned int i);

/* Tail operations */
EXTERNC int xxlistpush(XXlist*,xxelem); /* Add at Tail */
EXTERNC xxelem xxlistpop(XXlist*);
EXTERNC xxelem xxlisttop(XXlist*);

/* Duplicate and return the content (null terminate) */
EXTERNC xxelem* xxlistdup(XXlist*);

/* Look for value match */
EXTERNC int xxlistcontains(XXlist*, xxelem);

/* Remove element by value; only removes first encountered */
EXTERNC int xxlistelemremove(XXlist* l, xxelem elem);

/* remove duplicates */
EXTERNC int xxlistunique(XXlist*);

/* Create a clone of a list */
EXTERNC XXlist* xxlistclone(XXlist*);

/* Following are always "in-lined"*/
#define xxlistclear(l) xxlistsetlength((l),0U)
#define xxlistextend(l,len) xxlistsetalloc((l),(len)+(l->alloc))
#define xxlistcontents(l)  ((l)==NULL?NULL:(l)->content)
#define xxlistlength(l)  ((l)==NULL?0U:(l)->length)

#endif /*XXLIST_H*/
