
#ifndef __MYBMM_MODULE_H
#define __MYBMM_MODULE_H

#include "config.h"

#if 0
/* Forward dec */
struct mybmm_module;
#endif

typedef int (*mybmm_module_open_t)(void *);
typedef int (*mybmm_module_read_t)(void *,...);
typedef int (*mybmm_module_write_t)(void *,...);
typedef int (*mybmm_module_close_t)(void *);
typedef int (*mybmm_module_control_t)(void *,...);


struct mybmm_module {
	int type;
	char *name;
	unsigned short capabilities;
	int (*init)(mybmm_config_t *);
//	void *(*new)(mybmm_config_t *,mybmm_pack_t *,mybmm_module_t *);
	void *(*new)(mybmm_config_t *,...);
	mybmm_module_open_t open;
	mybmm_module_read_t read;
	mybmm_module_write_t write;
	mybmm_module_close_t close;
	int (*free)(void *);
	int (*shutdown)(void *);
	mybmm_module_control_t control;
	mybmm_module_control_t config;
};
typedef struct mybmm_module mybmm_module_t;

#define MYBMM_MODULE_CAPABILITY_CONTROL 0x01
#define MYBMM_MODULE_CAPABILITY_CONFIG	0x02

//mybmm_module_t *mybmm_get_module(mybmm_config_t *conf, char *name, int type);
//mybmm_module_t *mybmm_load_module(mybmm_config_t *conf, char *name, int type);
mybmm_module_t *mybmm_load_module(mybmm_config_t *conf, char *name, int type);

enum MYBMM_MODTYPE {
        MYBMM_MODTYPE_INVERTER,
        MYBMM_MODTYPE_CELLMON,
        MYBMM_MODTYPE_TRANSPORT,
        MYBMM_MODTYPE_DB,
};

#ifndef EXPORT_API
#define EXPORT_API __attribute__ ((visibility("default")))
#endif 

#endif /* __MYBMM_MODULE_H */
