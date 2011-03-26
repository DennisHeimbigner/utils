/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

#ifndef XXBYTES_H
#define XXBYTES_H 1

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__) || defined(__CPLUSPLUS)
#define externC extern "C"
#else
#define externC extern
#endif

typedef struct XXbytes {
  int nonextendible; /* 1 => fail if an attempt is made to extend this buffer*/
  unsigned int alloc;
  unsigned int length;
  char* content;
} XXbytes;

externC XXbytes* xxbytesnew(void);
externC void xxbytesfree(XXbytes*);
externC int xxbytessetalloc(XXbytes*,unsigned int);
externC int xxbytessetlength(XXbytes*,unsigned int);
externC int xxbytesfill(XXbytes*, char fill);

/* Produce a duplicate of the contents*/
externC char* xxbytesdup(XXbytes*);
/* Extract the contents and leave buffer empty */
externC char* xxbytesextract(XXbytes*);

/* Return the ith byte; -1 if no such index */
externC int xxbytesget(XXbytes*,unsigned int);
/* Set the ith byte */
externC int xxbytesset(XXbytes*,unsigned int,char);

/* Append one byte */
externC int xxbytesappend(XXbytes*,char); /* Add at Tail */
/* Append n bytes */
externC int xxbytesappendn(XXbytes*,void*,unsigned int); /* Add at Tail */

/* Concatenate a null-terminated string to the end of the buffer */
externC int xxbytescat(XXbytes*,char*);
/* Set the contents of the buffer; mark the buffer as non-extendible */
externC int xxbytessetcontents(XXbytes*, char*, unsigned int);

/* Null terminate the byte string without extending its length */
externC int xxbytesnull(XXbytes*);

/* Following are always "in-lined"*/
#define xxbyteslength(bb) ((bb)?(bb)->length:0U)
#define xxbytesalloc(bb) ((bb)?(bb)->alloc:0U)
#define xxbytescontents(bb) ((bb && bb->content)?(bb)->content:(char*)"")
#define xxbytesextend(bb,len) xxbytessetalloc((bb),(len)+(bb->alloc))
#define xxbytesclear(bb) ((bb)?(bb)->length=0:0U)
#define xxbytesavail(bb,n) ((bb)?((bb)->alloc - (bb)->length) >= (n):0U)

#endif /*XXBYTES_H*/
