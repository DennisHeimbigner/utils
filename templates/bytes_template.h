/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

#ifndef XXBYTES_H
#define XXBYTES_H 1

typedef struct XXbytes {
  int nonextendible; /* 1 => fail if an attempt is made to extend this buffer*/
  unsigned int alloc;
  unsigned int length;
  char* content;
} XXbytes;

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__) || defined(__CPLUSPLUS)
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

EXTERNC XXbytes* xxbytesnew(void);
EXTERNC void xxbytesfree(XXbytes*);
EXTERNC int xxbytessetalloc(XXbytes*,unsigned int);
EXTERNC int xxbytessetlength(XXbytes*,unsigned int);
EXTERNC int xxbytesfill(XXbytes*, char fill);

/* Produce a duplicate of the contents*/
EXTERNC char* xxbytesdup(XXbytes*);
/* Extract the contents and leave buffer empty */
EXTERNC char* xxbytesextract(XXbytes*);

/* Return the ith byte; -1 if no such index */
EXTERNC int xxbytesget(XXbytes*,unsigned int);
/* Set the ith byte */
EXTERNC int xxbytesset(XXbytes*,unsigned int,char);

/* Append one byte */
EXTERNC int xxbytesappend(XXbytes*,char); /* Add at Tail */
/* Append n bytes */
EXTERNC int xxbytesappendn(XXbytes*,const void*,unsigned int); /* Add at Tail */

/* Null terminate the byte string without extending its length (for debugging) */
EXTERNC int xxbytesnull(XXbytes*);

/* Concatenate a null-terminated string to the end of the buffer */
EXTERNC int xxbytescat(XXbytes*,const char*);

/* Set the contents of the buffer; mark the buffer as non-extendible */
EXTERNC int xxbytessetcontents(XXbytes*, char*, unsigned int);

/* Following are always "in-lined"*/
#define xxbyteslength(bb) ((bb)!=NULL?(bb)->length:0U)
#define xxbytesalloc(bb) ((bb)!=NULL?(bb)->alloc:0U)
#define xxbytescontents(bb) (((bb)!=NULL && (bb)->content!=NULL)?(bb)->content:(char*)"")
#define xxbytesextend(bb,len) xxbytessetalloc((bb),(len)+(bb->alloc))
#define xxbytesclear(bb) ((bb)!=NULL?(bb)->length=0:0U)
#define xxbytesavail(bb,n) ((bb)!=NULL?((bb)->alloc - (bb)->length) >= (n):0U)

#endif /*XXBYTES_H*/
