/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xxbytes.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEFAULTALLOC 1024
#define ALLOCINCR 1024

static int xxbytesdebug = 1;

static long
xxbytesfail(void)
{
    fflush(stdout);
    fprintf(stderr,"bytebuffer failure\n");
    fflush(stderr);
    if(xxbytesdebug) abort();
    return FALSE;
}

XXbytes*
xxbytesnew(void)
{
  XXbytes* bb = (XXbytes*)malloc(sizeof(XXbytes));
  if(bb == NULL) return (XXbytes*)xxbytesfail();
  bb->alloc=0;
  bb->length=0;
  bb->content=NULL;
  bb->nonextendible = 0;
  return bb;
}

int
xxbytessetalloc(XXbytes* bb, unsigned int sz)
{
  char* newcontent;
  if(bb == NULL) return xxbytesfail();
  if(sz <= 0) {sz = (bb->alloc?2*bb->alloc:DEFAULTALLOC);}
  if(bb->alloc >= sz) return TRUE;
  if(bb->nonextendible) return xxbytesfail();
  newcontent=(char*)calloc(sz,sizeof(char));
  if(newcontent == NULL) return FALSE;
  if(bb->alloc > 0 && bb->length > 0 && bb->content != NULL) {
    memcpy((void*)newcontent,(void*)bb->content,sizeof(char)*bb->length);
  }
  if(bb->content != NULL) free(bb->content);
  bb->content=newcontent;
  bb->alloc=sz;
  return TRUE;
}

void
xxbytesfree(XXbytes* bb)
{
  if(bb == NULL) return;
  if(!bb->nonextendible && bb->content != NULL) free(bb->content);
  free(bb);
}

int
xxbytessetlength(XXbytes* bb, unsigned int sz)
{
  if(bb == NULL) return xxbytesfail();
  if(sz > bb->alloc) {if(!xxbytessetalloc(bb,sz)) return xxbytesfail();}
  bb->length = sz;
  return TRUE;
}

int
xxbytesfill(XXbytes* bb, char fill)
{
  unsigned int i;
  if(bb == NULL) return xxbytesfail();
  for(i=0;i<bb->length;i++) bb->content[i] = fill;
  return TRUE;
}

int
xxbytesget(XXbytes* bb, unsigned int index)
{
  if(bb == NULL) return -1;
  if(index >= bb->length) return -1;
  return bb->content[index];
}

int
xxbytesset(XXbytes* bb, unsigned int index, char elem)
{
  if(bb == NULL) return xxbytesfail();
  if(index >= bb->length) return xxbytesfail();
  bb->content[index] = elem;
  return TRUE;
}

int
xxbytesappend(XXbytes* bb, char elem)
{
  if(bb == NULL) return xxbytesfail();
  /* We need space for the char + null */
  if(bb->length >= bb->alloc) {
	if(!ncbytessetalloc(bb,0)) return ncbytesfail();
  }
  bb->content[bb->length] = elem;
  bb->length++;
  bb->content[bb->length] = '\0';
  return TRUE;
}

/* This assumes s is a null terminated string*/
int
xxbytescat(XXbytes* bb, char* s)
{
    xxbytesappendn(bb,(void*)s,strlen(s)+1); /* include trailing null*/
    /* back up over the trailing null*/
    if(bb->length == 0) return xxbytesfail();
    bb->length--;
    return 1;
}

int
xxbytesappendn(XXbytes* bb, void* elem, unsigned int n)
{
  if(bb == NULL || elem == NULL) return xxbytesfail();
  if(n == 0) {n = strlen((char*)elem);}
  while(!xxbytesavail(bb,n)) {
    if(!xxbytessetalloc(bb,0)) return xxbytesfail();
  }
  memcpy((void*)&bb->content[bb->length],(void*)elem,n);
  bb->length += n;
  return TRUE;
}

int
xxbytesprepend(XXbytes* bb, char elem)
{
  int i; /* do not make unsigned */
  if(bb == NULL) return xxbytesfail();
  if(bb->length >= bb->alloc) if(!xxbytessetalloc(bb,0)) return xxbytesfail();
  /* could we trust memcpy? instead */
  for(i=bb->alloc;i>=1;i--) {bb->content[i]=bb->content[i-1];}
  bb->content[0] = elem;
  bb->length++;
  return TRUE;
}

char*
xxbytesdup(XXbytes* bb)
{
    char* result = (char*)malloc(bb->length+1);
    memcpy((void*)result,(const void*)bb->content,bb->length);
    result[bb->length] = '\0'; /* just in case it is a string*/
    return result;
}

char*
xxbytesextract(XXbytes* bb)
{
    char* result = bb->content;
    bb->alloc = 0;
    bb->length = 0;
    bb->content = NULL;
    return result;
}

int
xxbytessetcontents(XXbytes* bb, char* contents, unsigned int alloc)
{
    if(bb == NULL) return xxbytesfail();
    xxbytesclear(bb);
    if(!bb->nonextendible && bb->content != NULL) free(bb->content);
    bb->content = contents;
    bb->length = 0;
    bb->alloc = alloc;
    bb->nonextendible = 1;
    return 1;
}

/* Null terminate the byte string without extending its length */
int
xxbytesnull(XXbytes* bb)
{
    xxbytesappend(bb,'\0');
    bb->length--;
    return 1;
}
