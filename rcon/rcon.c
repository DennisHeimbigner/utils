/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information.
*/
/*$Id$*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "rcon.h"

static int rcondebug = 0;

#define ALLOCINCR 16
/* mnemonic */
#define OPTIONAL 1

#define INDENTCHUNK "  "

static int initialized = 0;

static char hexchars[17] = "01234567890abcdef";

static char delims[] = "{}[]:,;";

/* Token types */
#define LBRACE '{'
#define RBRACE '}'
#define LBRACK '['
#define RBRACK ']'
#define COMMA ','
#define COLON ':'
#define SEMICOLON ';'
#define _ILLEGAL rcon_unknown
#define _STRING rcon_string
#define _NUMBER rcon_number
#define _TRUE rcon_true
#define _FALSE rcon_false
#define _NULL rcon_null

/* For debugging */
enum nonterms { _value=0, _map=1, _array=2, _pair=3, _const=4};
static char* nontermnames[] = {"value","map","array","pair","const"};

#define ENTER(proc) {if(rcondebug) trace(proc,0,0);}
#define LEAVE(proc,tf) {if(rcondebug) trace(proc,1,tf);}

#define FAIL(lexer,msg) do {(lexer)->errmsg = (msg); goto fail;} while(0)

/* Define static pre-defined nodes */
#define CONSTNODE(name) static rconnode rcon_constant_##name = {rcon_const,rcon_##name}; rconnode* rcon_const_##name = &rcon_constant_##name

CONSTNODE(true);
CONSTNODE(false);
CONSTNODE(null);

/**************************************************/

typedef struct  rconlist {
    rconnode** contents;
    size_t len; /* |text| */
    size_t alloc; /* |text| */
} rconlist;

struct rcontext {
    char* text;
    size_t len; /* |text| */
    size_t alloc; /* |text| */
    int pushback[2]; /* max pushback needed */
};

typedef struct rconlexer {
    FILE* input;
    int token;    
    struct rcontext text;
    int pushedback; /* 1=>keep current token */
    int lineno;
    int charno;
    char* errmsg;
} rconlexer;

/****************************************/
static int value(rconlexer* lexer, rconnode** nodep);
static int map(rconlexer* lexer, rconnode** nodep, int);
static int pair(rconlexer* lexer, rconnode** nodep);
static int array(rconlexer* lexer, rconnode** nodep);
static int makeconst(rconlexer* lexer, rconnode** nodep);
static rconnode* createrconnode(rconlexer* lexer,rconclass);

static int nexttoken(rconlexer* lexer);
static void pushtoken(rconlexer* lexer);
static unsigned int tohex(int c);
static void dumptoken(rconlexer* lexer, int);
static int removeescapes(rconlexer* lexer);

static void textclear(struct rcontext* lexer);
static int textadd(struct rcontext* text, int c);
static int textterminate(struct rcontext* text);

static void pushback(rconlexer* lexer, int c);
static int unpush(rconlexer* lexer);
static int readc(rconlexer* lexer);

static int listadd(rconlist* list, rconnode* node);
static void listclear(rconlist* list);

static void trace(enum nonterms nt, int leave, int ok);

/**************************************************/
int
rcon(FILE* src, rconnode** nodep, rconerror* err)
{
    rconlexer lexer;
    rconnode* node = NULL;
    int token;

    memset((void*)&lexer,0,sizeof(rconlexer));

    if(!initialized) {
	rcon_constant_true.constvalue = "true";
	rcon_constant_true.constvalue = "false";
	rcon_constant_true.constvalue = "null";
	initialized = 1;
    }
    lexer.input = src;
    lexer.lineno = 1;
    lexer.charno = 1;
    lexer.errmsg = NULL;

    token = nexttoken(&lexer);    

    /* Make braces optional at top level */
    if(token == LBRACE) {
        if(!map(&lexer,&node,!OPTIONAL)) goto fail;
    } else {
	pushtoken(&lexer);
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
    if(node) rconnodefree(node);
    return 0;
}

static int
value(rconlexer* lexer, rconnode** nodep)
{
    rconnode* node = NULL;
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
map(rconlexer* lexer, rconnode** nodep, int optional)
{
    rconnode* node = NULL;
    rconnode* subnode = NULL;
    struct rconlist list = {NULL,0,0};
    int token;

    ENTER(_map);
    node = createrconnode(lexer,rcon_map);
    for(;;) {
        token = nexttoken(lexer);
	if(optional && token == RBRACE)
	    FAIL(lexer,"brace mismatch");
	else if(optional && token == EOF) goto done;
	else if(!optional && token == EOF)
	    FAIL(lexer,"unclosed map");
	else if(!optional && token == RBRACE) goto done;
	if(token == COMMA || token == SEMICOLON) continue;
	pushtoken(lexer);
        if(!pair(lexer,&subnode)) goto fail;
	if(!listadd(&list,subnode)) goto fail;
    }
done:
    node->list.values = list.contents;
    node->list.nvalues = list.len;
    if(nodep) *nodep = node;
    LEAVE(_map,1);
    return 1;
fail:
    listclear(&list);
    if(subnode != NULL) rconnodefree(subnode);
    if(node != NULL) rconnodefree(node);
    LEAVE(_map,0);
    return 0;
}

static int
pair(rconlexer* lexer, rconnode** nodep)
{
    rconnode* node = NULL;
    int token = 0;

    ENTER(_pair);
    node = createrconnode(lexer,rcon_pair);
    if(!node) goto fail;
    if(!makeconst(lexer,&node->pair.key))
	FAIL(lexer,"map key is not a string or word");
    if(node->pair.key->constclass != rcon_string)
	FAIL(lexer,"map key is not a string or word");
    token = nexttoken(lexer);	
    if(token != ':') goto fail;
    if(!value(lexer,&node->pair.value))
	FAIL(lexer,"invalid map value");
    if(nodep) *nodep = node;
    LEAVE(_pair,1);
    return 1;
fail:
    if(node != NULL) rconnodefree(node);
    LEAVE(_pair,0);
    return 0;
}

static int
array(rconlexer* lexer, rconnode** nodep)
{
    rconnode* subnode = NULL;
    struct rconlist list = {NULL,0,0};
    rconnode* node = NULL;

    ENTER(_array);

    node = createrconnode(lexer,rcon_array);
    for(;;) {
        int token = nexttoken(lexer);
	if(token == EOF)
	    FAIL(lexer,"unclosed array");
	if(token == RBRACK) goto done;
	if(token == COMMA || token == SEMICOLON) continue;
	pushtoken(lexer);
        if(!value(lexer,&subnode)) goto fail;
	if(!listadd(&list,subnode)) goto fail;
    }
done:
    node->list.values = list.contents;
    node->list.nvalues = list.len;
    if(nodep) *nodep = node;
    LEAVE(_array,1);
    return 1;
fail:
    listclear(&list);
    if(subnode != NULL) rconnodefree(subnode);
    if(node != NULL) rconnodefree(node);
    LEAVE(_array,0);
    return 0;
}

static int
makeconst(rconlexer* lexer, rconnode** nodep)
{
    rconnode* node = NULL;
    int token;
    ENTER(_const);
    token = nexttoken(lexer);
    switch (token) {
    case _STRING:
    case _NUMBER:
	node = createrconnode(lexer,rcon_const);
	if(node == NULL) goto fail;
	node->constclass = token;
	node->constvalue = strdup(lexer->text.text);
	break;
    case _TRUE:
	node = &rcon_constant_true;
	break;
    case _FALSE:
	node = &rcon_constant_false;
	break;
    case _NULL:
	node = &rcon_constant_null;
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

static rconnode*
createrconnode(rconlexer* lexer, rconclass cls)
{
     rconnode* node = (rconnode*)malloc(sizeof(rconnode));
     if(node != NULL) memset((void*)node,0,sizeof(rconnode));
     node->nodeclass = cls;
     return node;
}

/****************************************/

#ifdef IGNORE
static int
peek(rconlexer* lexer)
{
    int token = nexttoken(lexer);
    pushtoken(lexer);
    return token;
}
#endif

static void
pushtoken(rconlexer* lexer)
{
    lexer->pushedback = 1;
    if(rcondebug > 1)
	dumptoken(lexer,1);
}

static int
nexttoken(rconlexer* lexer)
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
    if(rcondebug > 1)
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
dumptoken(rconlexer* lexer, int pushed)
{
    fprintf(stderr,"%s : %d = |%s|\n",
	(pushed?"PUSHED":"TOKEN"),
	lexer->token,lexer->text.text);
}


static void
textclear(struct rcontext* txt)
{
    if(txt->len > 0) memset(txt->text,0,txt->alloc);
    txt->len = 0;
}

static int
textterminate(struct rcontext* text)
{
    return textadd(text,'\0');
}

static int
textadd(struct rcontext* text, int c)
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
pushback(rconlexer* lexer, int c)
{
    if(lexer->text.pushback[0] == 0) lexer->text.pushback[0] = c;
    else lexer->text.pushback[1] = c;
}

static int
unpush(rconlexer* lexer)
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
readc(rconlexer* lexer)
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
removeescapes(rconlexer* lexer)
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
            case '/': *q++ = '/'; break; /* RCON requires */
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
listadd(rconlist* list, rconnode* node)
{
    if(list->len >= list->alloc) {
        if(list->alloc == 0) {
	    list->contents = (rconnode**)malloc(sizeof(rconnode)*ALLOCINCR);
	    if(list->contents == NULL) return 0;
	    list->alloc = ALLOCINCR;
	    list->len = 0;
	} else {
	    list->contents = (rconnode**)realloc((void*)list->contents,sizeof(rconnode)*(list->alloc+ALLOCINCR));
	    if(list->contents == NULL) return 0;
	    list->alloc += ALLOCINCR;
	}
    }
    list->contents[list->len++] = node;
    return 1;    
}

static void
listclear(rconlist* list)
{
    if(list->contents != NULL) free(list->contents);
}

void
rconnodefree(rconnode* node)
{
    int i;
    if(node == NULL) return;
    switch (node->nodeclass) {
    case rcon_map:
	for(i=0;i<node->list.nvalues;i++)
	    rconnodefree(node->list.values[i]);
	break;
    case rcon_array:
	for(i=0;i<node->list.nvalues;i++)
	    rconnodefree(node->list.values[i]);
	break;

    case rcon_pair:
	free(node->pair.key);
	if(node->pair.value != NULL) rconnodefree(node->pair.value);
	break;

    case rcon_const:
        switch (node->constclass) {
        case rcon_string:
        case rcon_number:
	    if(node->constvalue) free(node->constvalue);
	    break;
        case rcon_true:
        case rcon_false:
        case rcon_null:
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
stringify(char* s, struct rcontext* tmp)
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
isoneline(rconnode* node)
{
    int i;
    for(i=0;i<node->list.nvalues;i++) {
	rconclass cl;
	rconnode* member;
	member = node->list.values[i];
	if(node->nodeclass == rcon_map) {
    	    cl = member->pair.value->nodeclass;
	} else if(node->nodeclass == rcon_array) {
    	    cl = member->nodeclass;
	} else return 0;
        if(cl == rcon_array || cl == rcon_map) return 0;
    }
    return 1;    
}

static void
rcondumpr(rconnode* node, FILE* f, struct rcontext* tmp, int depth, int meta)
{
    int i;
    int oneline;
    int endpoint;
    int lparen, rparen;
    char* tag = NULL;

    switch (node->nodeclass) {
    case rcon_map:
	{lparen = LBRACE; rparen = RBRACE; tag = "<map>";}
	/* fall thru */
    case rcon_array:
	if(tag == NULL)
	    {lparen = LBRACK; rparen = RBRACK; tag = "<array>";}
	oneline = isoneline(node);
	indent(f,depth);
	if(meta) fputs(tag,f);
	if(meta || depth > 0) fputc(lparen,f);
	endpoint = node->list.nvalues - 1;
	for(i=0;i<=endpoint;i++) {
	    rconnode* member = node->list.values[i];
	    if(i>0) fputs(" ",f);
	    rcondumpr(member,f,tmp,depth+1,meta);
	    if(i<endpoint && !oneline) {fputs("\n",f); indent(f,depth);}
	}
	if(!oneline) {
	    if(i > 0) fputs("\n",f);
	    indent(f,depth);
	}
	if(meta || depth > 0) fputc(rparen,f);
	fputs("\n",f);
	break;

    case rcon_pair:
        rcondumpr(node->pair.key,f,tmp,depth+1,meta);
	fputs(" : ",f);
        rcondumpr(node->pair.value,f,tmp,depth+1,meta);
	break;

    case rcon_const:
        switch (node->constclass) {
        case rcon_string:
	    if(meta) fputs("<string>",f);
	    stringify(node->constvalue,tmp);
	    textterminate(tmp);
	    if(isword(tmp->text))
	        fprintf(f,"%s",tmp->text);
	    else
		fprintf(f,"\"%s\"",tmp->text);
	    break;
        case rcon_number:
	    if(meta) fputs("<number>",f);
	    fprintf(f,"%s",node->constvalue);
	    break;
        case rcon_true:
	    if(meta) fputs("<true>",f);
	    fputs("true",f);
	    break;
        case rcon_false:
	    if(meta) fputs("<false>",f);
	    fputs("false",f);
	    break;
        case rcon_null:
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
rcondump(rconnode* node, FILE* f)
{
    struct rcontext tmp = {NULL,0,0};
    textclear(&tmp);
    rcondumpr(node,f,&tmp,0,0);
}

void
rcondumpmeta(rconnode* node, FILE* f)
{
    struct rcontext tmp = {NULL,0,0};
    textclear(&tmp);
    rcondumpr(node,f,&tmp,0,1);
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

/**************************************************/

rconnode*
rconlookup(rconnode* node, char* key)
{
    int i;
    if(node->nodeclass != rcon_map) return NULL;
    for(i=0;i<node->list.nvalues;i++) {
	rconnode* pair = node->list.values[i];
	if(strcmp(pair->pair.key->constvalue,key)==0)
	    return pair->pair.value;
    }
    return NULL;
}

rconnode*
rconget(rconnode* node, int index)
{
    int i;
    if(node->nodeclass == rcon_map || node->nodeclass == rcon_array) {
        if(index < 0 || index >= node->list.nvalues) return NULL;
	return node->list.values[i];	
    }
    return NULL;
}


/**************************************************/
/* Provide support for url matching */

static int
urlmatch(char* pattern, char* url)
{
    if(strcmp(pattern,"*") == 0) return 1;
    if(strncmp(url,pattern,strlen(pattern))==0) return 1;
    return 0;
}

static void
insert(rconnode** matches, int len, rconnode* pair)
{
    int i,j;
    /* handle initial case separately */
    if(len > 0) {
        /* sort lexically as determined by strcmp */
        for(i=0;i<len;i++) {
	    if(strcmp(matches[i]->pair.key->constvalue,
                      pair->pair.key->constvalue) > 0) {
	        for(j=(len-1);j>=i;j--) matches[j+1] = matches[j]; 
	        matches[i] = pair;
   	        return;
	    }
	}
    }
    matches[len] = pair; /* default: add at end */
}

static int
collectsortedmatches(char* url, rconnode* map, rconnode*** matchesp)
{
    int i,j;
    int nvalues = map->list.nvalues;
    rconnode* star;
    rconnode** matches = NULL;
    matches = (rconnode**)malloc(sizeof(rconnode*)*(nvalues+1));
    assert(map->nodeclass == rcon_map);
    for(j=0,i=0;i<nvalues;i++) {
	rconnode* pair = map->list.values[i];
	if(urlmatch(pair->pair.key->constvalue,url)) {
	    insert(matches,j,pair);
	    j++;
	}
    }
    /* Add "*" key at end */
    star = rconlookup(map,"*");
    if(star) matches[j++] = star;
    /* Add whole map at end */
    matches[j++] = map;
    if(matchesp) *matchesp = matches;
    return j;
}

int
rconurlmatch(rconnode* map, char* url, rconnode*** matchp)
{
    int matchcount = 0;
    rconnode** matches = NULL;
    assert(map->nodeclass == rcon_map);
    matchcount = collectsortedmatches(url,map,&matches);
    if(matchp) *matchp = matches;
    return matchcount;
}
