/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information.
*/
/*$Id$*/

#ifndef _JSON_H_
#define _JSON_H_ 1

/*
This json parser extends json in the following ways:
- "object" is referred to here as "map" 
- if the first token is not "{" or "]", then
  implicit "{...}" are added around the input.
- the quotes are notneeded for any string that
  does not contain white space or any of "[]{}:,/" characters.
  If such a string parses as a C number (using "%lg" scanf format),
  then it will be returned as a number token.
- javascript/java "//...\n" comments are supported.
- \xdd constants in strings are accepted.
*/

typedef enum jsonclass {
    json_unknown = 0, json_map = 1, json_array = 2,
    json_pair = 3, json_const = 4,
    json_string=5, json_number=6, json_true=7, json_false=8, json_null=9
} jsonclass;

typedef struct jsonnode {
    jsonclass jclass;
    jsonclass constclass; /*json_string .. json_null*/
    char* constvalue; /* jclass == json_const */
    struct {
        struct jsonnode* key;
        struct jsonnode* value;
    } pair; /* jclass == json_pair */
    struct {
        size_t nvalues;
        struct jsonnode** values;
    } compound ; /* jclass == json_map || json_array */
} jsonnode;

typedef struct jsonerror {
    int lineno;
    int charno;
    char* errmsg; /* may be null */
} jsonerror;

extern int json(FILE* src, jsonnode** nodep, jsonerror*);
extern void jsonnodefree(jsonnode* node);
extern void jsondump(jsonnode* node, FILE* f);
extern void jsondumpmeta(jsonnode* node, FILE* f);

#endif /*_JSON_H_*/
