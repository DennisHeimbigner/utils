/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */
/* Hex digits */
static char hexdigits[] = "0123456789abcdefABCDEF";

typdef enum jsontoken {
JSON_NONE=0,
JSON_STRING=1,
JSON_NUMBER=2,
JSON_TRUE=5,
JSON_FALSE=6,
JSON_NULL=7;
JSON_NULL=7;
} jsontoken;

static char wordchar[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-.";

static char singlechar[] = "{}[]:,";

typedef struct jsonstate {
    /* Lexer state */
    OCbytes* text; /* of current token */
    jsontoken token;    
    char* input; /* complete json text */
    char* next; /* next char in input */
    int lineno;
} jsonstate;


int
jsonlex(jsonstate* state)
{
    int token;
    int c;
    unsigned int i;
    char* p=state->next;
    char* tmp;

    token = 0;
    ocbytesclear(state->text);
    /* invariant: p always points to current char */
    for(p=state->next;token==0&&(c=*p);p++) {
	if(c == '\n') {
	    state->lineno++;
	} else if(c <= ' ' || c == '\177') {
	    /* ignore */
	} else if(c == '#') {
	    /* single line comment */
	    while((c=*(++p))) {if(c == '\n') break;}
	} else if(strchr(delims,c) != NULL) {
	    ocbytesappend(state->text,c); token = c;
	} else if(c == '"') {
	    int more = 1;
	    while(more && (c=*(++p))) {
		switch (c) {
		case '"': more=0; break;
		case '\\':
		    c=*(++p);
		    switch (c) {
		    case 'r': c = '\r'; break;
		    case 'n': c = '\n'; break;
		    case 'f': c = '\f'; break;
		    case 't': c = '\t'; break;
		    case 'b': c = '\b'; break;
		    case '/': c = '/'; break;
		    case 'u': {
			unsigned int d1,d2,d3,d4;
			++p;
		        if(*p) d1 = tohex(*p++); else goto fail;
		        if(*p) d2 = tohex(*p++); else goto fail;
		        if(*p) d3 = tohex(*p++); else goto fail;
		        if(*p) d4 = tohex(*p++); else goto fail;
			/* Convert to a sequence of utf-8 characters */
			ConversionResult status;
			UTF8 ss8[8];
			UTF8* s8 = ss8;
			UTF16  ss16[1]; /* for storing the utf16 string */
			UTF16* s16 = ss16; /* for storing the utf16 string */
    			UTF16* tmp16; /* for storing the utf16 string */
    			tmp8 = s8;
			size_t len8;
			ss16[0] = (d1<<12)|(d2<<8)|(d3<<4)|(d4);
			status = ConvertUTF16toUTF8((const UTF16**)&s16,s16+1,
					    &tmp16, tmp16+1,
					    lenientConversion);
			if(status != conversionOK)  goto fail;
			/* Get the length of the utf8 string */
			len8 = (tmp8 - s8);
			ocbytesappendn(state->text,s8,len8);
		    } break;
		    default: break;
		    }
		    break;
		default: ocbytesappend(state->text,c);
		}
	    }
	    token=JS_STRING;
	} else if(strchr(wordchars,c)) {
	    /* we have a SCAN_WORD */
	    ocbytesappend(state,c);
	    while((c=*(++p))) {
		if(strchr(wordchars,c) == NULL) {p--; break;}
		ocbytesappend(state,c);
	    }
	    /* check for keyword */
	    token=SCAN_WORD; /* assume */
	    tmp = ocbytescontent(state->text);
	    if(strcmp(tmp,"true") == 0) {
		token = JS_TRUE;
	    } else if(strcmp(tmp,"false") == 0) {
		token = JS_FALSE;
	    } else if(strcmp(tmp,"null") == 0) {
		token = JS_NULL;
	    } else
		token = JS_NUMBER;
	} else { /* illegal */
	}
    }
    state->next = p;
    if(ocdebug >= 2)
	dumptoken(state);
    /*Put return value onto Bison stack*/
    if(ocbyteslength(state->text) == 0)
        *lvalp = NULL;
    else {
        *lvalp = ocbytesdup(state->text);
    }
    return token;      /* Return the type of the token.  */
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
dumptoken(jsonstate* state)
{
    fprintf(stderr,"TOKEN = |%s|\n",ocbytescontents(state->text));
}

/*
Simple lexer
*/

void
setwordchars(jsonstate* state, int kind)
{
    switch (kind) {
    case 0:
	state->worddelims = jsonworddelims;
	state->wordchars1 = jsonwordchars1;
	state->wordcharsn = jsonwordcharsn;
	break;
    case 1:
	state->worddelims = jsonworddelims;
	state->wordchars1 = jsonwordchars1;
	state->wordcharsn = daswordcharsn;
	break;
    case 2:
	state->worddelims = jsonworddelims;
	state->wordchars1 = cewordchars1;
	state->wordcharsn = cewordcharsn;
	break;
    default: break;
    }
}

void
jsonlexinit(char* input, jsonstate** statep)
{
    jsonstate* state = (jsonstate*)malloc(sizeof(jsonstate));
    if(statep) *statep = state;
    if(state == NULL) return;
    memset((void*)state,0,sizeof(jsonstate));
    state->input = strdup(input);
    state->next = state->input;
    state->text = ocbytesnew();
    state->reclaim = oclistnew();
    setwordchars(state,0); /* Assume JSON */
}

void
jsonlexcleanup(jsonstate** statep)
{
    jsonstate* state = *statep;
    if(state == NULL) return;
    if(state->input != NULL) ocfree(state->input);
    if(state->reclaim != NULL) {
	while(oclistlength(state->reclaim) > 0) {
	    char* word = (char*)oclistpop(state->reclaim);
	    if(word) free(word);
	}
	oclistfree(state->reclaim);
    }
    ocbytesfree(state->text);
    free(state);
    *statep = NULL;
}
