/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xxlist.h"

static xxelem xxDATANULL = (xxelem)0;
/*static int xxinitialized=0;*/

int xxlistnull(xxelem e) {return e == xxDATANULL;}

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEFAULTALLOC 16
#define ALLOCINCR 16

XXlist* xxlistnew(void)
{
  XXlist* l;
/*
  if(!xxinitialized) {
    memset((void*)&xxDATANULL,0,sizeof(xxelem));
    xxinitialized = 1;
  }
*/
  l = (XXlist*)malloc(sizeof(XXlist));
  if(l) {
    l->alloc=0;
    l->length=0;
    l->content=NULL;
  }
  return l;
}

int
xxlistfree(XXlist* l)
{
  if(l) {
    l->alloc = 0;
    if(l->content != NULL) {free(l->content); l->content = NULL;}
    free(l);
  }
  return TRUE;
}

int
xxlistsetalloc(XXlist* l, unsigned int sz)
{
  xxelem* newcontent;
  if(l == NULL) return FALSE;
  if(sz <= 0) {sz = (l->length?2*l->length:DEFAULTALLOC);}
  if(l->alloc >= sz) {return TRUE;}
  newcontent=(xxelem*)calloc(sz,sizeof(xxelem));
  if(l->alloc > 0 && l->length > 0 && l->content != NULL) {
    memcpy((void*)newcontent,(void*)l->content,sizeof(xxelem)*l->length);
  }
  if(l->content != NULL) free(l->content);
  l->content=newcontent;
  l->alloc=sz;
  return TRUE;
}

int
xxlistsetlength(XXlist* l, unsigned int sz)
{
  if(l == NULL) return FALSE;
  if(sz > l->alloc && !xxlistsetalloc(l,sz)) return FALSE;
  l->length = sz;
  return TRUE;
}

xxelem
xxlistget(XXlist* l, unsigned int index)
{
  if(l == NULL || l->length == 0) return xxDATANULL;
  if(index >= l->length) return xxDATANULL;
  return l->content[index];
}

int
xxlistset(XXlist* l, unsigned int index, xxelem elem)
{
  if(l == NULL) return FALSE;
  if(index >= l->length) return FALSE;
  l->content[index] = elem;
  return TRUE;
}

/* Insert at position i of l; will push up elements i..|seq|. */
int
xxlistinsert(XXlist* l, unsigned int index, xxelem elem)
{
  int i; /* do not make unsigned */
  if(l == NULL) return FALSE;
  if(index > l->length) return FALSE;
  xxlistsetalloc(l,0);
  for(i=(int)l->length;i>index;i--) l->content[i] = l->content[i-1];
  l->content[index] = elem;
  l->length++;
  return TRUE;
}

int
xxlistpush(XXlist* l, xxelem elem)
{
  if(l == NULL) return FALSE;
  if(l->length >= l->alloc) xxlistsetalloc(l,0);
  l->content[l->length] = elem;
  l->length++;
  return TRUE;
}

xxelem
xxlistpop(XXlist* l)
{
  if(l == NULL || l->length == 0) return xxDATANULL;
  l->length--;  
  return l->content[l->length];
}

xxelem
xxlisttop(XXlist* l)
{
  if(l == NULL || l->length == 0) return xxDATANULL;
  return l->content[l->length - 1];
}

xxelem
xxlistremove(XXlist* l, unsigned int i)
{
  unsigned int len;
  xxelem elem;
  if(l == NULL || (len=l->length) == 0) return xxDATANULL;
  if(i >= len) return xxDATANULL;
  elem = l->content[i];
  for(i+=1;i<len;i++) l->content[i-1] = l->content[i];
  l->length--;
  return elem;  
}

/* Duplicate and return the content (null terminate) */
xxelem*
xxlistdup(XXlist* l)
{
    xxelem* result = (xxelem*)malloc(sizeof(xxelem)*(l->length+1));
    memcpy((void*)result,(void*)l->content,sizeof(xxelem)*l->length);
    result[l->length] = (xxelem)0;
    return result;
}

int
xxlistcontains(XXlist* list, xxelem elem)
{
    unsigned int i;
    for(i=0;i<xxlistlength(list);i++) {
	if(elem == xxlistget(list,i)) return 1;
    }
    return 0;
}

/* Remove element by value; only removes first encountered */
int
xxlistelemremove(XXlist* l, xxelem elem)
{
  unsigned int len;
  unsigned int i;
  int found = 0;
  if(l == NULL || (len=l->length) == 0) return xxDATANULL;
  for(i=0;i<xxlistlength(l);i++) {
    xxelem candidate = l->content[i];
    if(elem == candidate) {
      for(i+=1;i<len;i++) l->content[i-1] = l->content[i];
      l->length--;
      found = 1;
      break;
    }
  }
  return found;
}




/* Extends xxlist to include a unique operator 
   which remove duplicate values; NULL values removed
   return value is always 1.
*/

int
xxlistunique(XXlist* list)
{
    unsigned int i,j,k,len;
    xxelem* content;
    if(list == NULL || list->length == 0) return 1;
    len = list->length;
    content = list->content;
    for(i=0;i<len;i++) {
        for(j=i+1;j<len;j++) {
	    if(content[i] == content[j]) {
		/* compress out jth element */
                for(k=j+1;k<len;k++) content[k-1] = content[k];	
		len--;
	    }
	}
    }
    list->length = len;
    return 1;
}

XXlist*
xxlistclone(XXlist* list)
{
    XXlist* clone = xxlistnew();
    *clone = *list;
    clone->content = xxlistdup(list);
    return clone;
}
