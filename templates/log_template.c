/*********************************************************************
 *   Copyright 2010, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

#include "xxlog.h"

#define PREFIXLEN 8
#define MAXTAGS 256
#define XXTAGDFALT "Log";

static int xxlogginginitialized = 0;
static int xxlogging = 0;
static int xxsystemfile = 0; /* 1 => we are logging to file we did not open */
static char* xxlogfile = NULL;
static FILE* xxlogstream = NULL;

static int xxtagsize = 0;
static char** xxtagset = NULL;
static char* xxtagdfalt = NULL;
static char* xxtagsetdfalt[] = {"Warning","Error","Note","Debug"};
static char* xxtagname(int tag);

/*!\defgroup XXlog XXlog Management
@{*/

/*!\internal
*/

void
xxloginit(void)
{
{
    const char* file;
    if(xxlogginginitialized)
	return;
    xxlogginginitialized = 1;
    xxsetlogging(0);
    file = getenv(XXENVFLAG);
    xxlogfile = NULL;
    xxlogstream = NULL;
    /* Use environment variables to preset xxlogging state*/
    /* I hope this is portable*/
    if(file != NULL && strlen(file) > 0) {
        if(xxlogopen(file)) {
	    xxsetlogging(1);
	}
    }
    xxtagdfalt = XXTAGDFALT;
    xxtagset = xxtagsetdfalt;
}

/*!
Enable/Disable logging.

\param[in] tf If 1, then turn on logging, if 0, then turn off logging.

\return The previous value of the logging flag.
*/

int
xxsetlogging(int tf)
{
    int was;
    if(!xxlogginginitialized) xxloginit();
    was = xxlogging;
    xxlogging = tf;
    return was;
}

/*!
Specify a file into which to place logging output.

\param[in] file The name of the file into which to place logging output.
If the file has the value NULL, then send logging output to
stderr.

\return zero if the open failed, one otherwise.
*/

int
xxlogopen(const char* file)
{
    if(!xxlogginginitialized) xxloginit();
    xxlogclose();
    if(file == NULL || strlen(file) == 0) {
	/* use stderr*/
	xxlogstream = stderr;
	xxlogfile = NULL;
	xxsystemfile = 1;
    } else if(strcmp(file,"stdout") == 0) {
	/* use stdout*/
	xxlogstream = stdout;
	xxlogfile = NULL;
	xxsystemfile = 1;
    } else if(strcmp(file,"stderr") == 0) {
	/* use stderr*/
	xxlogstream = stderr;
	xxlogfile = NULL;
	xxsystemfile = 1;
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
	    xxlogstream = NULL;
	    xxsetlogging(0);
	    return 0;
	}
	xxsystemfile = 0;
    }
    return 1;
}

void
xxlogclose(void)
{
    if(!xxlogginginitialized) xxloginit();
    if(xxlogstream != NULL && !xxsystemfile) {
	fclose(xxlogstream);
    }
    if(xxlogfile != NULL) free(xxlogfile);
    xxlogstream = NULL;
    xxlogfile = NULL;
    xxsystemfile = 0;
}

/*!
Send logging messages. This uses a variable
number of arguments and operates like the stdio
printf function.

\param[in] tag Indicate the kind of this log message.
\param[in] format Format specification as with printf.
*/

void
xxlog(int tag, const char* fmt, ...)
{
    va_list args;
    char* prefix;

    if(!xxlogginginitialized) xxloginit();

    if(!xxlogging || xxlogstream == NULL) return;

    prefix = xxtagname(tag);
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
    xxlogtextn(tag,text,strlen(text));
}

/*!
Send arbitrarily long text as a logging message.
Each line will be sent using xxlog with the specified tag.
\param[in] tag Indicate the kind of this log message.
\param[in] text Arbitrary text to send as a logging message.
*/

void
xxlogtextn(int tag, const char* text, size_t count)
{
    if(!xxlogging || xxlogstream == NULL) return;
    fwrite(text,1,count,xxlogstream);
    fflush(xxlogstream);
}

/* The tagset is null terminated */
void
xxlogsettags(char** tagset, char* dfalt)
{
    xxtagdfalt = dfalt;
    if(tagset == NULL) {
	xxtagsize = 0;
    } else {
        int i;
	/* Find end of the tagset */
	for(i=0;i<MAXTAGS;i++) {if(tagset[i]==NULL) break;}
	xxtagsize = i;
    }
    xxtagset = tagset;
}

static char*
xxtagname(int tag)
{
    if(tag < 0 || tag >= xxtagsize) {
	return xxtagdfalt;
    } else {
	return xxtagset[tag];
    }
}

/**@}*/
