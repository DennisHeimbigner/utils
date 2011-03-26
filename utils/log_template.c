/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

#include <stdio.h>
#include <fcntl.h>

#define PREFIXLEN 8

#define ENVFLAG "XXLOGFILE"

static int xxloginit = 0;
static int xxlogging = 0;
static char* xxlogfile = NULL;
static FILE* xxlogstream = NULL;

void
xxloginit(void)
{
    xxloginit = 1;
    xxsetlogging(0);
    xxlogfile = NULL;
    xxlogstream = NULL;
    /* Use environment variables to preset xxlogging state*/
    /* I hope this is portable*/
    if(getenv(ENVFLAG) != NULL) {
	const char* file = getenv(ENVFLAG);
	xxsetlogging(1);
	xxlogopen(file);
    }
}

void
xxsetlogging(int tf)
{
    if(!xxloginit) xxloginit();
    xxlogging = tf;
}

void
xxlogopen(const char* file)
{
    if(!xxloginit) xxloginit();
    if(xxlogfile != NULL) {
	fclose(xxlogstream);
	free(xxlogfile);
	xxlogfile = NULL;
    }
    if(file == NULL || strlen(file) == 0) {
	/* use stderr*/
	xxlogstream = stderr;
	xxlogfile = NULL;
    } else if(strcmp(file,"stdout") == 0) {
	/* use stdout*/
	xxlogstream = stdout;
	xxlogfile = NULL;
    } else if(strcmp(file,"stderr") == 0) {
	/* use stderr*/
	xxlogstream = stderr;
	xxlogfile = NULL;
    } else {
	int fd;
	xxlogfile = strdup(file);
	xxlogstream = NULL;
	/* We need to deal with this file carefully
	   to avoid unauthorized access*/
	fd = open(xxlogfile,O_WRONLY|O_APPEND|O_CREAT,0600);
	if(fd >= 0) {
	    xxlogstream = fdopen(fd,"a");
	} else {
	    free(xxlogfile);
	    xxlogfile = NULL;
	    xxsetlogging(0);
	}
    }
}

void
xxlogclose(void)
{
    if(xxlogfile != NULL && xxlogstream != NULL) {
	fclose(xxlogstream);
	xxlogstream = NULL;
	if(xxlogfile != NULL) free(xxlogfile);
	xxlogfile = NULL;
    }
}

void
xxlog(int tag, const char* fmt, ...)
{
    va_list args;
    char* prefix;
    if(!xxlogging || xxlogstream == NULL) return;

    switch (tag) {
    case LOGWARN: prefix = "Warning:"; break;
    case LOGERR:  prefix = "Error:  "; break;
    case LOGNOTE: prefix = "Note:   "; break;
    case LOGDBG:  prefix = "Debug:  "; break;
    default:
        fprintf(xxlogstream,"Error:  Bad log prefix: %d\n",tag);
	prefix = "Error:  ";
	break;
    }
    fprintf(xxlogstream,"%s:",prefix);

    if(fmt != NULL) {
      va_start(args, fmt);
      vfprintf(xxlogstream, fmt, args);
      va_end( args );
    }
    fprintf(xxlogstream, "\n" );
    fflush(xxlogstream);
}

void
xxlogtext(int tag, const char* text)
{
    char line[1024];
    size_t delta = 0;
    const char* eol = text;

    if(!xxlogging || xxlogstream == NULL) return;

    while(*text) {
	eol = strchr(text,'\n');
	if(eol == NULL)
	    delta = strlen(text);
	else
	    delta = (eol - text);
	if(delta > 0) memcpy(line,text,delta);
	line[delta] = '\0';
	fprintf(xxlogstream,"        %s\n",line);
	text = eol+1;
    }
}
