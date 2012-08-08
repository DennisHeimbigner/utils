/*********************************************************************
 *   Copyright 2010, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/

#ifndef XXLOG_H
#define XXLOG_H

#define XXENVFLAG "XXLOGFILE"

/* Suggested tag values */
#define XXLOGNOTE 0
#define XXLOGWARN 1
#define XXLOGERR 2
#define XXLOGDBG 3

extern void xxloginit(void);
extern int xxsetlogging(int tf);
extern int xxlogopen(const char* file);
extern void xxlogclose(void);

/* The tag value is an arbitrary integer */
extern void xxlog(int tag, const char* fmt, ...);
extern void xxlogtext(int tag, const char* text);
extern void xxlogtextn(int tag, const char* text, size_t count);

/* Provide printable names for tags */
extern void xxlogsettags(char** tagset, char* dfalt);

#endif /*XXLOG_H*/
