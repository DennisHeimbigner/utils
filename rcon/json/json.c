/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information.
*/
/*$Id$*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "json.h"
#include "ConvertUTF.h"

#define ALLOCINCR 16
/* mnemonic */
#define OPTIONAL 1

#define INDENTCHUNK "  "

static int jsondebug = 0;

static char hexchars[17] = "01234567890abcdef";

static char delims[] = "{}[]:,";

/* Token types */
#define LBRACE '{'
#define RBRACE '}'
#define LBRACK '['
#define RBRACK ']'
#define COMMA ','
#define COLON ':'
#define _ILLEGAL json_unknown
#define _STRING json_string
#define _NUMBER json_number
#define _TRUE json_true
#define _FALSE json_false
#define _NULL json_null

/* For debugging */
enum nonterms { _value=0, _map=1, _array=2, _pair=3, _const=4};
static char* nontermnames[] = {"value","map","array","pair","const"};

#define ENTER(proc) {if(jsondebug) trace(proc,0,0);}
#define LEAVE(proc,tf) {if(jsondebug) trace(proc,1,tf);}

#define FAIL(lexer,msg) do {(lexer)->errmsg = (msg); goto fail;} while(0)

/* Define static pre-defined nodes */
#define CONSTNODE(name) static jsonnode json_constant_##name = {json_const,json_##name,#name,{NULL,NULL},{0,NULL}}

CONSTNODE(true);
CONSTNODE(false);
CONSTNODE(null);

typedef struct  jsonlist {
    jsonnode** contents;
    size_t len; /* |text| */
    size_t alloc; /* |text| */
} jsonlist;

struct jsontext {
    char* text;
    size_t len; /* |text| */
    size_t alloc; /* |text| */
    int pushback[2]; /* max pushback needed */
};

typedef struct jsonlexer {
    FILE* input;
    int token;    
    struct jsontext text;
    int pushedback; /* 1=>keep current token */
    int lineno;
    int charno;
    char* errmsg;
} jsonlexer;

/****************************************/
static int value(jsonlexer* lexer, jsonnode** nodep);
static int map(jsonlexer* lexer, jsonnode** nodep, int);
static int pair(jsonlexer* lexer, jsonnode** nodep);
static int array(jsonlexer* lexer, jsonnode** nodep);
static int makeconst(jsonlexer* lexer, jsonnode** nodep);
static jsonnode* createjsonnode(jsonlexer* lexer,jsonclass);

static int nexttoken(jsonlexer* lexer);
static void pushtoken(jsonlexer* lexer);
static unsigned int tohex(int c);
static void dumptoken(jsonlexer* lexer, int);
static int removeescapes(jsonlexer* lexer);

static void textclear(struct jsontext* lexer);
static int textadd(struct jsontext* text, int c);
static int textterminate(struct jsontext* text);

static void pushback(jsonlexer* lexer, int c);
static int unpush(jsonlexer* lexer);
static int readc(jsonlexer* lexer);

static int listadd(jsonlist* list, jsonnode* node);
static void listclear(jsonlist* list);

static void trace(enum nonterms nt, int leave, int ok);

/**************************************************/
int
json(FILE* src, jsonnode** nodep, jsonerror* err)
{
    jsonlexer lexer;
    jsonnode* node = NULL;
    int token;
    memset((void*)&lexer,0,sizeof(jsonlexer));
    lexer.input = src;
    lexer.lineno = 1;
    lexer.charno = 1;
    lexer.errmsg = NULL;

    token = nexttoken(&lexer);    
    /* Make braces optional at top level */
    if(token == LBRACE || token == LBRACK) {
	pushtoken(&lexer);
        if(!value(&lexer,&node)) goto fail;
    } else {
        if(!map(&lexer,&node,OPTIONAL)) goto fail;	    
    }
    if(nodep) *nodep = node;
    return 1;
fail:
    if(err) {
	err->lineno = lexer.lineno;
	err->charno = lexer.charno-1;
	err->errmsg = lexer.errmsg;
    }
    if(node) jsonnodefree(node);
    return 0;
}

static int
value(jsonlexer* lexer, jsonnode** nodep)
{
    jsonnode* node = NULL;
    int token = 0;

    ENTER(_value);

    token = nexttoken(lexer);
    switch (lexer->token) {
    case LBRACE:
	if(!map(lexer,&node,!OPTIONAL)) goto fail;
	break;
    case LBRACK:
	if(!array(lexer,&node)) goto fail;
	break;
    case _STRING:
    case _NUMBER:
    case _NULL:
    case _TRUE:
    case _FALSE:
	pushtoken(lexer);
	if(!makeconst(lexer,&node)) goto fail;
	break;
    case _ILLEGAL:
    default: goto fail;
    }
    if(nodep) *nodep = node;

    LEAVE(_value,1);
    return 1;
fail:
    LEAVE(_value,0);
    return 0;
}

static int
map(jsonlexer* lexer, jsonnode** nodep, int optional)
{
    jsonnode* node = NULL;
    jsonnode* subnode = NULL;
    struct jsonlist list = {NULL,0,0};
    int first;
    int token;

    ENTER(_map);
    node = createjsonnode(lexer,json_map);
    for(first=1;;first=0) {
        token = nexttoken(lexer);
	if(optional && token == EOF) goto done;
	else if(optional && token == RBRACE)
	    FAIL(lexer,"extra map close brace");
	else if(!optional && token == EOF)
	    FAIL(lexer,"unclosed map");
	else if(!optional && token == RBRACE) goto done;
	if(!first && token != ',') goto fail;
	else if(first) pushtoken(lexer);
        if(!pair(lexer,&subnode)) goto fail;
	if(!listadd(&list,subnode)) goto fail;
    }
done:
    node->compound.values = list.contents;
    node->compound.nvalues = list.len;
    if(nodep) *nodep = node;
    LEAVE(_map,1);
    return 1;
fail:
    listclear(&list);
    if(subnode != NULL) jsonnodefree(subnode);
    if(node != NULL) jsonnodefree(node);
    LEAVE(_map,0);
    return 0;
}

static int
pair(jsonlexer* lexer, jsonnode** nodep)
{
    jsonnode* node = NULL;
    int token = 0;

    ENTER(_pair);
    node = createjsonnode(lexer,json_pair);
    if(!node) goto fail;
    if(!makeconst(lexer,&node->pair.key))
	FAIL(lexer,"map key is not a string");
    if(node->pair.key->constclass != json_string)
	FAIL(lexer,"map key is not a string");
    token = nexttoken(lexer);	
    if(token != ':') goto fail;
    if(!value(lexer,&node->pair.value))
	FAIL(lexer,"invalid map value");
    if(nodep) *nodep = node;
    LEAVE(_pair,1);
    return 1;
fail:
    if(node != NULL) jsonnodefree(node);
    LEAVE(_pair,0);
    return 0;
}

static int
array(jsonlexer* lexer, jsonnode** nodep)
{
    jsonnode* subnode = NULL;
    struct jsonlist list = {NULL,0,0};
    jsonnode* node = NULL;
    int first;

    ENTER(_array);

    node = createjsonnode(lexer,json_array);
    for(first = 1;;first=0) {
        int token = nexttoken(lexer);
	if(token == EOF)
	    FAIL(lexer,"unclosed array");
	if(token == RBRACK) goto done;
	if(!first && token != ',') goto fail;
	else if(first) pushtoken(lexer);
        if(!value(lexer,&subnode)) goto fail;
	if(!listadd(&list,subnode)) goto fail;
    }
done:
    node->compound.values = list.contents;
    node->compound.nvalues = list.len;
    if(nodep) *nodep = node;
    LEAVE(_array,1);
    return 1;
fail:
    listclear(&list);
    if(subnode != NULL) jsonnodefree(subnode);
    if(node != NULL) jsonnodefree(node);
    LEAVE(_array,0);
    return 0;
}

static int
makeconst(jsonlexer* lexer, jsonnode** nodep)
{
    jsonnode* node = NULL;
    int token;
    ENTER(_const);
    token = nexttoken(lexer);
    switch (token) {
    case _STRING:
    case _NUMBER:
	node = createjsonnode(lexer,json_const);
	if(node == NULL) goto fail;
	node->constclass = token;
	node->constvalue = strdup(lexer->text.text);
	break;
    case _TRUE:
	node = &json_constant_true;
	break;
    case _FALSE:
	node = &json_constant_false;
	break;
    case _NULL:
	node = &json_constant_null;
	break;
    default: abort();
    }        
    if(nodep) *nodep = node;
    LEAVE(_const,1);
    return 1;
fail:
    LEAVE(_const,0);
    return 0;
}

static jsonnode*
createjsonnode(jsonlexer* lexer, jsonclass cls)
{
     jsonnode* node = (jsonnode*)malloc(sizeof(jsonnode));
     if(node != NULL) memset((void*)node,0,sizeof(jsonnode));
     node->jclass = cls;
     return node;
}

/****************************************/

#ifdef IGNORE
static int
peek(jsonlexer* lexer)
{
    int token = nexttoken(lexer);
    pushtoken(lexer);
    return token;
}
#endif

static void
pushtoken(jsonlexer* lexer)
{
    lexer->pushedback = 1;
    if(jsondebug > 1)
	dumptoken(lexer,1);
}

static int
nexttoken(jsonlexer* lexer)
{
    int token;
    int c;
    if(lexer->pushedback)
	{token = lexer->token; lexer->pushedback = 0; goto done;}
    token = 0;
    textclear(&lexer->text);
    while(token==0) {
	c=readc(lexer);
	lexer->charno++;
	if(c == EOF) {
	    token = EOF;
	    lexer->charno--;
	    break;
	} else if(c == '\n') {
	    lexer->lineno++;
	    lexer->charno = 1;
	} else if(c == '/') { 
	    c = readc(lexer);
	    if(c == '/') {/* single line comment */
	        while((c=readc(lexer)) != EOF) {if(c == '\n') break;}
	    } else {
		pushback(lexer,c); c = '/';
	    }
	}
	if(c <= ' ' || c == '\177') {
	    /* ignore */
	} else if(strchr(delims,c) != NULL) {
	    textadd(&lexer->text,c);
	    token = c;
	} else if(c == '"') {
	    int more = 1;
	    while(more) {
		c = readc(lexer);
		switch (c) {
		case EOF: goto fail;
		case '"': more=0; break;
		case '\\':
		    textadd(&lexer->text,c);
		    c=readc(lexer);
		    textadd(&lexer->text,c);		    
		    break;
		default: textadd(&lexer->text,c);
		}
	    }
	    if(!removeescapes(lexer)) goto fail;
	    token=_STRING;
	} else { /* Treat like a string without quotes */
	    textadd(&lexer->text,c);
	    while((c=readc(lexer))) {
		if(c == '/' || c <= ' ' || c == '\177') {pushback(lexer,c); break;}
		else if(strchr(delims,c) != NULL) {pushback(lexer,c); break;}
		textadd(&lexer->text,c);
	    }
	    if(!removeescapes(lexer)) goto fail;
	    /* check for keyword */
	    if(strcmp(lexer->text.text,"true") == 0) {
		token = _TRUE;
	    } else if(strcmp(lexer->text.text,"false") == 0) {
		token = _FALSE;
	    } else if(strcmp(lexer->text.text,"null") == 0) {
		token = _NULL;
	    } else { /* See if this looks like a number */
		double d;
		if(sscanf(lexer->text.text,"%lg",&d) == 1)
		    token = _NUMBER;
		else
		    token = _STRING;
	    }
	}
    }
done:
    lexer->token = token;
    if(jsondebug > 1)
	dumptoken(lexer,0);
    return token;
fail:
    return EOF;
}

static unsigned int
tohex(int c)
{
    if(c >= 'a' && c <= 'f') return (c - 'a') + 0xa;
    if(c >= 'A' && c <= 'F') return (c - 'A') + 0xa;
    if(c >= '0' && c <= '9') return (c - '0');
    return 0;
}

static void
dumptoken(jsonlexer* lexer, int pushed)
{
    fprintf(stderr,"%s : %d = |%s|\n",
	(pushed?"PUSHED":"TOKEN"),
	lexer->token,lexer->text.text);
}


static void
textclear(struct jsontext* txt)
{
    if(txt->len > 0) memset(txt->text,0,txt->alloc);
    txt->len = 0;
}

static int
textterminate(struct jsontext* text)
{
    return textadd(text,'\0');
}

static int
textadd(struct jsontext* text, int c)
{
    if(text->len >= text->alloc) {
        if(text->alloc == 0) {
	    text->text = (char*)malloc(ALLOCINCR+1);
	    if(text->text == NULL) return 0;
	    text->alloc = ALLOCINCR;
	    text->len = 0;
	} else {
	    text->text = (char*)realloc((void*)text->text,text->alloc+ALLOCINCR+1);
	    if(text->text == NULL) return 0;
	    text->alloc += ALLOCINCR;
	}
	text->text[text->alloc] = '\0';
    }
    text->text[text->len++] = c;
    return 1;    
}

static void
pushback(jsonlexer* lexer, int c)
{
    if(lexer->text.pushback[0] == 0) lexer->text.pushback[0] = c;
    else lexer->text.pushback[1] = c;
}

static int
unpush(jsonlexer* lexer)
{
    int c = '\0';
    int i;
    for(i=1;i>=0;i--) {
        if(lexer->text.pushback[i] != 0) {
	    c = lexer->text.pushback[i];
	    lexer->text.pushback[i] = '\0';
	    break;
	}
    }
    return c;
}

static int
readc(jsonlexer* lexer)
{
    int c = 0;

    c = unpush(lexer);
    if(c == 0) c = getc(lexer->input);
    return c;
}

/* Convert the characters in lexer->text.text to
   remove escapes. Assumes that all escapes are smaller
   than the unescaped value.
*/
static int
removeescapes(jsonlexer* lexer)
{
    char* p = lexer->text.text;
    char* q = p;
    int cp;
    while((cp=*p++)) {
        switch (cp) {
	case '\\':
	    cp=*p++;
	    switch (cp) {
	    case '\0': *q++ = cp; goto done;
            case 'r': *q++ = '\r'; break;
            case 'n': *q++ = '\n'; break;
            case 'f': *q++ = '\f'; break;
            case 't': *q++ = '\t'; break;
            case 'b': *q++ = '\b'; break;
            case '/': *q++ = '/'; break; /* JSON requires */
            case 'u': {
                unsigned int d[4];
                ConversionResult status;
                UTF8 ss8[4];
                UTF8* s8 = ss8;
                UTF16  ss16[1]; /* for storing the utf16 string */
                UTF16* s16 = ss16; /* for storing the utf16 string */
                UTF8* tmp8 = ss8; /* for storing the utf16 string */
                size_t len8 = 0;
                int i;
                for(i=0;i<4;i++) {
                    if((cp = *p++) == '\0') goto fail;
                    d[i] = tohex(cp);
                }
                /* Convert to a sequence of utf-8 characters */
                ss16[0] = (d[0]<<12)|(d[1]<<8)|(d[2]<<4)|(d[3]);
                status = ConvertUTF16toUTF8((const UTF16**)&s16,s16+1,
                                    &tmp8, tmp8+1,
                                    lenientConversion);
                if(status != conversionOK)  goto fail;
                /* Get the length of the utf8 string */
                len8 = (tmp8 - s8);
		assert(len8 <= 4);
                for(i=0;i<len8;i++) *q++ = s8[i];
            } break;

            case 'x': {
                unsigned int d[2];
                int i;
                for(i=0;i<2;i++) {
                    if((cp = *p++) == '\0') goto fail;
                    d[i] = tohex(cp);
                }
                /* Convert to a sequence of utf-8 characters */
                cp = (d[0]<<4)|d[1];
		*q++ = cp;
            } break;

            default: break;

            }
            break;
        }
    }
done:
    return 1;
fail:
    return 0;
}


static int
listadd(jsonlist* list, jsonnode* node)
{
    if(list->len >= list->alloc) {
        if(list->alloc == 0) {
	    list->contents = (jsonnode**)malloc(sizeof(jsonnode)*ALLOCINCR);
	    if(list->contents == NULL) return 0;
	    list->alloc = ALLOCINCR;
	    list->len = 0;
	} else {
	    list->contents = (jsonnode**)realloc((void*)list->contents,sizeof(jsonnode)*(list->alloc+ALLOCINCR));
	    if(list->contents == NULL) return 0;
	    list->alloc += ALLOCINCR;
	}
    }
    list->contents[list->len++] = node;
    return 1;    
}

static void
listclear(jsonlist* list)
{
    if(list->contents != NULL) free(list->contents);
}

void
jsonnodefree(jsonnode* node)
{
    int i;
    if(node == NULL) return;
    switch (node->jclass) {
    case json_map: /* fall thru */
    case json_array:
	for(i=0;i<node->compound.nvalues;i++)
	    jsonnodefree(node->compound.values[i]);
	break;

    case json_pair:
	free(node->pair.key);
	if(node->pair.value != NULL) jsonnodefree(node->pair.value);
	break;

    case json_const:
        switch (node->constclass) {
        case json_string:
        case json_number:
	    if(node->constvalue) free(node->constvalue);
	    break;
        case json_true:
        case json_false:
        case json_null:
	    break;
        default: abort();
	} break;
	break;

    default: abort();
    }
}

static void
indent(FILE* f, int depth)
{
#ifdef IGNORE
    while(depth--) fputs(INDENTCHUNK,f);
#endif
}

static void
stringify(char* s, struct jsontext* tmp)
{
    char* p = s;
    int c;
    textclear(tmp);
    while((c=*p++)) {
	if(c == '"' || c < ' ' || c >= '\177') {
	    textadd(tmp,'\\');
	    switch (c) {
	    case '"': textadd(tmp,'"'); break;
	    case '\r': textadd(tmp,'r'); break;
	    case '\n': textadd(tmp,'r'); break;
	    case '\t': textadd(tmp,'r'); break;
	    default:
	        textadd(tmp,'x');
	        textadd(tmp,hexchars[(c & 0xf0)>>4]);
	        textadd(tmp,hexchars[c & 0x0f]);
		break;
	    }
	} else 
	    textadd(tmp,c);
    }
}

static int
isword(char* s)
{
    char* p = s;
    int c;
    while((c=*p++)) {
	if(strchr(delims,c) != NULL
	   || c == '/' || c <= ' ' || c >= '\177') return 0;
    }
    return 1;
}

static int
isoneline(jsonnode* node)
{
    int i;
    for(i=0;i<node->compound.nvalues;i++) {
	jsonclass cl;
	jsonnode* member;
	member = node->compound.values[i];
	if(node->jclass == json_map) {
    	    cl = member->pair.key->jclass;
	} else {
	    assert(node->jclass == json_array);
    	    cl = member->jclass;
	}
        if(cl == json_array || cl == json_map) return 0;
	
    }
    return 1;    
}

static void
jsondumpr(jsonnode* node, FILE* f, struct jsontext* tmp, int depth, int meta)
{
    int i;
    int oneline;
    int endpoint;
    int lparen, rparen;
    char* tag = NULL;

    switch (node->jclass) {
    case json_map:
	{lparen = LBRACE; rparen = RBRACE; tag = "<map>";}
	/* fall thru */
    case json_array:
	if(tag == NULL)
	    {lparen = LBRACK; rparen = RBRACK; tag = "<array>";}
	oneline = isoneline(node);
	indent(f,depth);
	if(meta) fputs(tag,f);
	fputc(lparen,f);
	endpoint = node->compound.nvalues - 1;
	for(i=0;i<=endpoint;i++) {
	    jsonnode* member = node->compound.values[i];
	    if(i>0) fputs(" ",f);
	    jsondumpr(member,f,tmp,depth+1,meta);
	    if(i<endpoint) fputs(",",f);
	    if(i<endpoint && !oneline) {fputs("\n",f); indent(f,depth);}
	}
	if(!oneline) {
	    if(i > 0) fputs("\n",f);
	    indent(f,depth);
	}
	fputc(rparen,f);
	fputs("\n",f);
	break;

    case json_pair:
        jsondumpr(node->pair.key,f,tmp,depth+1,meta);
	fputs(" : ",f);
        jsondumpr(node->pair.value,f,tmp,depth+1,meta);
	break;

    case json_const:
        switch (node->constclass) {
        case json_string:
	    if(meta) fputs("<string>",f);
	    stringify(node->constvalue,tmp);
	    textterminate(tmp);
	    if(isword(tmp->text))
	        fprintf(f,"%s",tmp->text);
	    else
		fprintf(f,"\"%s\"",tmp->text);
	    break;
        case json_number:
	    if(meta) fputs("<number>",f);
	    fprintf(f,"%s",node->constvalue);
	    break;
        case json_true:
	    if(meta) fputs("<true>",f);
	    fputs("true",f);
	    break;
        case json_false:
	    if(meta) fputs("<false>",f);
	    fputs("false",f);
	    break;
        case json_null:
	    if(meta) fputs("<null>",f);
	    fputs("null",f);
	    break;
        default: abort();
	}
	break;

    default: abort();
    }
}

void
jsondump(jsonnode* node, FILE* f)
{
    struct jsontext tmp = {NULL,0,0};
    textclear(&tmp);
    jsondumpr(node,f,&tmp,0,0);
}

void
jsondumpmeta(jsonnode* node, FILE* f)
{
    struct jsontext tmp = {NULL,0,0};
    textclear(&tmp);
    jsondumpr(node,f,&tmp,0,1);
}

static void
trace(enum nonterms nt, int leave, int ok)
{
    if(!leave) {
	fprintf(stderr,"enter: %s\n",nontermnames[(int)nt]);
    } else {/* leave */
	fprintf(stderr,"leave: %s : %s\n",nontermnames[(int)nt],
		(ok?"succeed":"fail"));
    }
}
