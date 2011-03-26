/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information.
*/
/*$Id$*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rcon.h"
#include "errno.h"
#include <unistd.h>

static int printmeta;

static void usage(char* msg)
{
    if(msg == NULL) msg = "";
    fprintf(stderr,"%s\nusage: rcontest <test1> <test2>...\n",msg);
    exit(1);
}

int
main(int argc, char** argv)
{
    rconnode* node = NULL;
    rconerror err;
    FILE* f = NULL;
    int i;
    int c;

    printmeta=0;
    while((c = getopt(argc, argv, "m")) != EOF) {
        switch (c) {
        case 'm': printmeta = 1; break;
	case '?': /* fall thru */
	default: usage("unknown option");
	}
    }
    argc -= optind;
    argv += optind;

    if(argc == 0) usage("no file specified");
    for(i=0;i<argc;i++) {
	char fname[1024];
	char* p;
	strncpy(fname,argv[i],sizeof(fname));
	p = strrchr(fname,'.');
	if(p == NULL || strcmp(p,".rc") != 0) {
	    strcat(fname,".rc");
	}

	f = fopen(fname,"r");
	if(f == NULL) {
	    fprintf(stderr,"Test: %s : open failed: %s\n",fname,strerror(errno));
	    continue;
	}
        int stat = rcon(f,&node,&err);
        if(!stat) {
	     fprintf(stderr,"syntax error: line %d  char %d ",err.lineno,err.charno);
	     if(err.errmsg != NULL)
		fprintf(stderr," ; %s",err.errmsg);
	     fputs("\n",stderr);
	} else if(printmeta)
	    rcondumpmeta(node,stdout);
	else
	    rcondump(node,stdout);
	fclose(f);
    }
    return 0;
}

