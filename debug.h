
#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>
extern int debug;

#ifdef DEBUG
#include "utils.h"

//#define dprintf(level, format, args...) { if (debug >= level) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args); }
#define dprintf(level, format, args...) { if (debug >= level) log_write(LOG_DEBUG, "%s(%d): " format, __FUNCTION__, __LINE__, ## args); }
#define DPRINTF(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#define DLOG(opts, format, args...) log_write(opts, "%s(%d): " format, __FUNCTION__, __LINE__, ## args)
#define DDLOG(format, args...) log_write(LOG_DEBUG, "%s(%d): " format, __FUNCTION__, __LINE__, ## args)
#else
#define dprintf(level,format,args...) /* noop */
#define DPRINTF(format, args...) /* noop */
#define DLOG(level, format, args...) /* noop */
#define DDLOG(format, args...) /* noop */
#endif

#endif /* __DEBUG_H */
