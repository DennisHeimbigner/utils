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

EXTERNC int xxlistnull(void*);

typedef struct XXlist {
  unsigned long alloc;
  unsigned long length;
  void** content;
} XXlist;

EXTERNC XXlist* xxlistnew(void);
EXTERNC int xxlistfree(XXlist*);
EXTERNC int xxlistsetalloc(XXlist*,unsigned long);
EXTERNC int xxlistsetlength(XXlist*,unsigned long);

/* Set the ith element */
EXTERNC int xxlistset(XXlist*,unsigned long,void*);
/* Get value at position i */
EXTERNC void* xxlistget(XXlist*,unsigned long);/* Return the ith element of l */
/* Insert at position i; will push up elements i..|seq|. */
EXTERNC int xxlistinsert(XXlist*,unsigned long,void*);
/* Remove element at position i; will move higher elements down */
EXTERNC void* xxlistremove(XXlist* l, unsigned long i);

/* Tail operations */
EXTERNC int xxlistpush(XXlist*,void*); /* Add at Tail */
EXTERNC void* xxlistpop(XXlist*);
EXTERNC void* xxlisttop(XXlist*);

/* Duplicate and return the content (null terminate) */
EXTERNC void** xxlistdup(XXlist*);

/* Look for value match */
EXTERNC int xxlistcontains(XXlist*, void*);

/* Remove element by value; only removes first encountered */
EXTERNC int xxlistelemremove(XXlist* l, void* elem);

/* remove duplicates */
EXTERNC int xxlistunique(XXlist*);

/* Create a clone of a list */
EXTERNC XXlist* xxlistclone(XXlist*);

/* Following are always "in-lined"*/
#define xxlistclear(l) xxlistsetlength((l),0)
#define xxlistextend(l,len) xxlistsetalloc((l),(len)+(l->alloc))
#define xxlistcontents(l)  ((l)==NULL?NULL:(l)->content)
#define xxlistlength(l)  ((l)==NULL?0:(l)->length)

#endif /*XXLIST_H*/
