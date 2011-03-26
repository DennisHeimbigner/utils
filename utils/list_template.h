/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */
#ifndef XXLIST_H
#define XXLIST_H 1

/* Define the type of the elements in the list*/

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
#define externC extern "C"
#else
#define externC extern
#endif

typedef unsigned long xxelem;

externC int xxlistnull(xxelem);

typedef struct XXlist {
  unsigned int alloc;
  unsigned int length;
  xxelem* content;
} XXlist;

externC XXlist* xxlistnew(void);
externC int xxlistfree(XXlist*);
externC int xxlistsetalloc(XXlist*,unsigned int);
externC int xxlistsetlength(XXlist*,unsigned int);

/* Set the ith element */
externC int xxlistset(XXlist*,unsigned int,xxelem);
/* Get value at position i */
externC xxelem xxlistget(XXlist*,unsigned int);/* Return the ith element of l */
/* Insert at position i; will push up elements i..|seq|. */
externC int xxlistinsert(XXlist*,unsigned int,xxelem);
/* Remove element at position i; will move higher elements down */
externC xxelem xxlistremove(XXlist* l, unsigned int i);

/* Tail operations */
externC int xxlistpush(XXlist*,xxelem); /* Add at Tail */
externC xxelem xxlistpop(XXlist*);
externC xxelem xxlisttop(XXlist*);

/* Duplicate and return the content (null terminate) */
externC xxelem* xxlistdup(XXlist*);

/* Look for value match */
externC int xxlistcontains(XXlist*, xxelem);

/* remove duplicates */
externC int xxlistunique(XXlist*);

/* Create a clone of a list */
externC XXlist* xxlistclone(XXlist*);

/* Following are always "in-lined"*/
#define xxlistclear(l) xxlistsetlength((l),0U)
#define xxlistextend(l,len) xxlistsetalloc((l),(len)+(l->alloc))
#define xxlistcontents(l) ((l)->content)
#define xxlistlength(l)  ((l)?(l)->length:0U)

#endif /*XXLIST_H*/

