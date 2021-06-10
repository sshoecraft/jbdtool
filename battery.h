
#ifndef __BATTERY_H
#define __BATTERY_H

#include "config.h"

enum BATTERY_CHEMS {
	BATTERY_CHEM_UNKNOWN,
	BATTERY_CHEM_LITHIUM,
	BATTERY_CHEM_LIFEPO4,
	BATTERY_CHEM_TITANATE,
};

int battery_init(mybmm_config_t *conf);
void charge_check(mybmm_config_t *conf);
void charge_stop(mybmm_config_t *conf,int);
void charge_start(mybmm_config_t *conf,int);

#endif
