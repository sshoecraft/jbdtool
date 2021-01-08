
#ifndef __MYBMM_H
#define __MYBMM_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

typedef struct mybmm_module mybmm_module_t;
typedef int (*mybmm_module_open_t)(void *);
typedef int (*mybmm_module_read_t)(void *,...);
typedef int (*mybmm_module_write_t)(void *,...);
typedef int (*mybmm_module_close_t)(void *);
typedef int (*mybmm_module_control_t)(void *,...);

#include "list.h"

#define MYBMM_TARGET_LEN 32

#include "config.h"
#include "module.h"
#define MYBMM_PACK_NAME_LEN 32
#define MYBMM_MAX_CELLS 32
#include "pack.h"

extern int debug;
#ifdef dprintf
#undef dprintf
#endif
#define dprintf(level, format, args...) { if (debug >= level) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args); }

void bindump(char *,void *,int);
char *strele(int num,char *delimiter,char *string);
char *trim(char *string);

#endif
