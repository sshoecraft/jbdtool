
#ifndef __MYBMM_INVERTER_H
#define __MYBMM_INVERTER_H

#include "config.h"

struct mybmm_inverter {
	mybmm_config_t *conf;		/* back ptr */
	char name[MYBMM_PACK_NAME_LEN];	/* Inverter name */
	char uuid[37];			/* Inverter UUID */
	char type[32];			/* Inverter type */
	char transport[32];		/* Transport name */
	char target[32];		/* Device/Interface/Address */
	char params[64];		/* Inverter-specific params */
	float battery_voltage;		/* Really? I need to comment this? */
	float battery_amps;		/* batt power, in amps */
	float battery_power;		/* batt power, in watts */
	float battery_temp;
	float grid_power;		/* Grid/Gen watts */
	float load_power;		/* loads watts */
	float site_power;		/* pv/wind/caes/chp watts */
	int error;			/* Inverter Error code */
	char errmsg[256];		/* Inverter Error message */
	void *handle;			/* Inverter Handle */
	mybmm_module_open_t open;	/* Inverter Open */
	mybmm_module_control_t control;	/* Inverter Control */
	mybmm_module_read_t read;	/* Inverter Read */
	mybmm_module_write_t write;	/* Inverter Write */
	mybmm_module_close_t close;	/* Inverter Close */
	uint16_t state;			/* Inverter State */
	uint16_t capabilities;		/* Capability bits */
//	int failed;			/* Failed to update count */
	int have_temp;
};
typedef struct mybmm_inverter mybmm_inverter_t;

/* States */
#define MYBMM_INVERTER_STATE_UPDATED	0x01
#define MYBMM_INVERTER_STATE_RUNNING	0x02
#define MYBMM_INVERTER_STATE_GRID	0x04
#define MYBMM_INVERTER_STATE_GEN	0x08
#define MYBMM_INVERTER_STATE_CHARGING	0x10

/* Capabilities */
#define MYBMM_INVERTER_GRID_CONTROL	0x01
#define MYBMM_INVERTER_GEN_CONTROL	0x02
#define MYBMM_INVERTER_POWER_CONTROL	0x04

int inverter_add(mybmm_config_t *conf, mybmm_inverter_t *inv);
int inverter_init(mybmm_config_t *conf);
//int inverter_start_update(mybmm_config_t *conf);
int inverter_read(mybmm_inverter_t *inv);
int inverter_write(mybmm_inverter_t *inv);

#endif /* __MYBMM_INVERTER_H */
