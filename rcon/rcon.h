/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information.
*/
/*$Id$*/

#ifndef _RCON_H_
#define _RCON_H_ 1

/*
This rcon parser is a modified version of json.
The modifications are as follows:
- the character set is utf8.
- "object" is referred to here as "map" 
- if the first token is not "{", then
  implicit "{...}" are added around the input.
- Commas are not used.
- a new format for strings (called words)
  has been added in which the quotes
  are not required. The word must
  have the following properties:
  - it does not contain white space or any of the characers: []{}:/
  - If a word parses as a C number (using "%lg" scanf format),
    then it will be returned as a number token.
- "//...\n" style comments are supported.
- \xdd constants in strings are accepted.
- \udddd constants are not.
*/

typedef enum rconclass {
rcon_unknown = 0,
rcon_map = 1,
rcon_array = 2,
rcon_pair = 3,
rcon_const = 4,
/* Following are used to define constant classes */
rcon_true=5,
rcon_false=6,
rcon_null=7,
rcon_number=8,
rcon_string=9
} rconclass;

/* rconnode is technically the union of several kinds of nodes:
   pairs, constants maps, and arrays; the last two are unified as list.
   Since the node size is so small, we do not bother with a formal union{...}.
*/
typedef struct rconnode {
    rconclass nodeclass;
    rconclass constclass; /*rcon_string .. rcon_null*/
        char* constvalue; /* jclass == rcon_const */
        struct {
            struct rconnode* key;
            struct rconnode* value;
        } pair; /* jclass == rcon_pair */
        struct {
            size_t nvalues;
            struct rconnode** values; /* set of pairs */
        } list ; /* jclass == rcon_map|rcon_array */
} rconnode;

typedef struct rconerror {
    int lineno;
    int charno;
    char* errmsg; /* may be null */
} rconerror;

extern rconnode* rcon_const_true;
extern rconnode* rcon_const_false;
extern rconnode* rcon_const_null;

extern int rcon(FILE* src, rconnode** nodep, rconerror*);
extern void rconnodefree(rconnode* node);
extern void rcondump(rconnode* node, FILE* f);
extern void rcondumpmeta(rconnode* node, FILE* f);

extern rconnode* rconlookup(rconnode* node, char* key);
extern rconnode* rconget(rconnode* node, int index);

extern int rconurlmatch(rconnode* map, char* url, rconnode*** matchp);

#endif /*_RCON_H_*/
