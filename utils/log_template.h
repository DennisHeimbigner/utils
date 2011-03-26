/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

#ifndef XXLOG_H
#define XXLOG_H

#define XXNOTE 0
#define XXWARN 1
#define XXERR 2
#define XXDBG 3

extern void xxloginit();
extern void xxsetlogging(int tf);
extern void xxlogopen(const char* file);
extern void xxlogclose(void);

extern void xxlog(int tag, const char* fmt, ...);
extern void xxlogtext(int tag, const char* text);

#endif /*XXLOG_H*/
