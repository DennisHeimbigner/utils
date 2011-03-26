/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information.
*/
/*$Id$*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "json.h"
#include "errno.h"
#include <unistd.h>

static int printmeta;

static void usage(char* msg)
{
    if(msg == NULL) msg = "";
    fprintf(stderr,"%s\nusage: jsontest <test1> <test2>...\n",msg);
    exit(1);
}

int
main(int argc, char** argv)
{
    jsonnode* node = NULL;
    jsonerror err;
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
	if(p == NULL || strcmp(p,".json") != 0) {
	    strcat(fname,".json");
	}

	f = fopen(fname,"r");
	if(f == NULL) {
	    fprintf(stderr,"Test: %s : open failed: %s\n",fname,strerror(errno));
	    continue;
	}
        int stat = json(f,&node,&err);
        if(!stat) {
	     fprintf(stderr,"syntax error: line %d  char %d ",err.lineno,err.charno);
	     if(err.errmsg != NULL)
		fprintf(stderr," ; %s",err.errmsg);
	     fputs("\n",stderr);
	} else if(printmeta)
	    jsondumpmeta(node,stdout);
	else
	    jsondump(node,stdout);
	fclose(f);
    }
    return 0;
}

