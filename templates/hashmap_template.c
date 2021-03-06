/*********************************************************************
 *   Copyright 2010, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xxhashmap.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEFAULTALLOC 31

XXhashmap* xxhashnew(void) {return xxhashnew0(DEFAULTALLOC);}

XXhashmap* xxhashnew0(int alloc)
{
  XXhashmap* hm;
  if(sizeof(xxhashid) != sizeof(void*)){
	fprintf(stderr,"xxhashmap: sizeof(xxhashid) != sizeof(void*)");
	abort();
  }
  hm = (XXhashmap*)malloc(sizeof(XXhashmap));
  if(!hm) return NULL;
  hm->alloc = alloc;
  hm->table = (NClist**)malloc(hm->alloc*sizeof(NClist*));
  if(!hm->table) {free(hm); return NULL;}
  memset((void*)hm->table,0,hm->alloc*sizeof(NClist*));
  return hm;
}

int
xxhashfree(XXhashmap* hm)
{
  if(hm) {
    int i;
    for(i=0;i<hm->alloc;i++) {
	if(hm->table[i] != NULL) xxlistfree(hm->table[i]);
    }
    free(hm->table);
    free(hm);
  }
  return TRUE;
}

/* Insert a <xxhashid,void*> pair into the table*/
/* Fail if already there*/
int
xxhashinsert(XXhashmap* hm, xxhashid hash, void* value)
{
    int i,offset,len;
    XXlist* seq;
    void** list;

    offset = (hash % hm->alloc);    
    seq = hm->table[offset];
    if(seq == NULL) {seq = xxlistnew(); hm->table[offset] = seq;}
    len = xxlistlength(seq);
    list = xxlistcontents(seq);
    for(i=0;i<len;i+=2,list+=2) {
	if(hash==(xxhashid)(*list)) return FALSE;
    }    
    xxlistpush(seq,(void*)hash);
    xxlistpush(seq,value);
    hm->size++;
    return TRUE;
}

/* Insert a <xxhashid,void*> pair into the table*/
/* Overwrite if already there*/
int
xxhashreplace(XXhashmap* hm, xxhashid hash, void* value)
{
    int i,offset,len;
    XXlist* seq;
    void** list;

    offset = (hash % hm->alloc);    
    seq = hm->table[offset];
    if(seq == NULL) {seq = xxlistnew(); hm->table[offset] = seq;}
    len = xxlistlength(seq);
    list = xxlistcontents(seq);
    for(i=0;i<len;i+=2,list+=2) {
	if(hash==(xxhashid)(*list)) {list[1] = value; return TRUE;}
    }    
    xxlistpush(seq,(void*)hash);
    xxlistpush(seq,value);
    hm->size++;
    return TRUE;
}

/* remove a xxhashid*/
/* return TRUE if found, false otherwise*/
int
xxhashremove(XXhashmap* hm, xxhashid hash)
{
    int i,offset,len;
    XXlist* seq;
    void** list;

    offset = (hash % hm->alloc);    
    seq = hm->table[offset];
    if(seq == NULL) return TRUE;
    len = xxlistlength(seq);
    list = xxlistcontents(seq);
    for(i=0;i<len;i+=2,list+=2) {
	if(hash==(xxhashid)(*list)) {
	    xxlistremove(seq,i+1);
	    xxlistremove(seq,i);
	    hm->size--;
	    if(xxlistlength(seq) == 0) {xxlistfree(seq); hm->table[offset] = NULL;}
	    return TRUE;
	}
    }    
    return FALSE;
}

/* lookup a xxhashid; return DATANULL if not found*/
/* (use hashlookup if the possible values include 0)*/
void*
xxhashget(XXhashmap* hm, xxhashid hash)
{
    void* value;
    if(!xxhashlookup(hm,hash,&value)) return NULL;
    return value;
}

int
xxhashlookup(XXhashmap* hm, xxhashid hash, void** valuep)
{
    int i,offset,len;
    XXlist* seq;
    void** list;

    offset = (hash % hm->alloc);    
    seq = hm->table[offset];
    if(seq == NULL) return TRUE;
    len = xxlistlength(seq);
    list = xxlistcontents(seq);
    for(i=0;i<len;i+=2,list+=2) {
	if(hash==(xxhashid)(*list)) {if(valuep) {*valuep = list[1]; return TRUE;}}
    }
    return FALSE;
}

/* Return the ith pair; order is completely arbitrary*/
/* Can be expensive*/
int
xxhashith(XXhashmap* hm, int index, xxhashid* hashp, void** elemp)
{
    int i;
    if(hm == NULL) return FALSE;
    for(i=0;i<hm->alloc;i++) {
	XXlist* seq = hm->table[i];
	int len = xxlistlength(seq) / 2;
	if(len == 0) continue;
	if((index - len) < 0) {
	    if(hashp) *hashp = (xxhashid)xxlistget(seq,index*2);
	    if(elemp) *elemp = xxlistget(seq,(index*2)+1);
	    return TRUE;
	}
	index -= len;
    }
    return FALSE;
}

/* Return all the keys; order is completely arbitrary*/
/* Can be expensive*/
int
xxhashkeys(XXhashmap* hm, xxhashid** keylist)
{
    int i,j,index;
    xxhashid* keys;
    if(hm == NULL) return FALSE;
    if(hm->size == 0) {
	keys = NULL;
    } else {
        keys = (xxhashid*)malloc(sizeof(xxhashid)*hm->size);
        for(index=0,i=0;i<hm->alloc;i++) {
 	    XXlist* seq = hm->table[i];
	    for(j=0;j<xxlistlength(seq);j+=2) {	
	        keys[index++] = (xxhashid)xxlistget(seq,j);
	    }
	}
    }
    if(keylist) {*keylist = keys;}
    return TRUE;
}

