
#ifndef __MYBMM_CONFIG_H
#define __MYBMM_CONFIG_H

#include <pthread.h>

struct mybmm_inverter;
typedef struct mybmm_inverter mybmm_inverter_t;

struct mybmm_pack;
typedef struct mybmm_pack mybmm_pack_t;

enum BATTERY_CHEMS {
	BATTERY_CHEM_UNKNOWN,
	BATTERY_CHEM_LITHIUM,
	BATTERY_CHEM_LIFEPO4,
	BATTERY_CHEM_TITANATE,
};

struct mybmm_config {
	char *filename;			/* Config filename */
	char db_name[32];		/* DB Name */
	void *dlsym_handle;		/* Image handle */
	list modules;			/* Modules */
	mybmm_inverter_t *inverter;	/* Inverter */
	pthread_t inverter_tid;		/* Inverter Thread ID */
	list packs;			/* Packs */
	pthread_t pack_tid;		/* Pack thread ID */
	int interval;			/* Check interval */
	int system_voltage;		/* System Voltage (defaults to 48) */
	int battery_chem;		/* Battery type (0=Li-ion, 1=LifePO4, 2=Titanate) */
	int user_capacity;		/* User-specified capacity */
	float battery_voltage;		/* Battery Voltage  */
	float battery_current;		/* Total amount of power into/out of battery */
	int cells;			/* Number of cells per battery pack */
	float cell_low;			/* Cell discharge low cutoff */
	float cell_crit_low;		/* Cell critical low */
	float cell_high;		/* Cell charge high cutoff */
	float cell_crit_high;		/* Cell critical high */
	float capacity;			/* Total capacity, in AH (all packs) */
	float c_rate;			/* Discharge current rate */
	float kwh;			/* Calculated kWh */
	float soc;			/* State of Charge */
	float soh;			/* State of Health */
	float input_current;		/* Power from sources */
	float output_current;		/* Power to loads/batt */
	float capacity_remain;		/* Remaining capacity */
	float max_charge_amps;		/* From inverter */
	float max_discharge_amps;	/* From inverter */
	float discharge_voltage;	/* Calculated: cell_low * cells */
	float discharge_amps;		/* Calculated */
	float charge_voltage;		/* Calculated: cell_high * cells */
	float charge_amps;		/* Calculated */
	float user_charge_voltage;	/* User-specified charge voltage */
	float user_discharge_voltage;	/* User-specified discharge voltage */
	float user_soc;			/* Forced State of Charge */
	void *cfg;			/* Config file handle */
	uint32_t state;			/* States */
};
typedef struct mybmm_config mybmm_config_t;

#define MYBMM_STATE_CONFIG_DIRTY	0x01		/* Config has been updated & needs written */
#define MYBMM_STATE_CHARGE_CONTROL	0x02		/* All packs have ability to start/stop charging */
#define MYBMM_STATE_DISCHARGE_CONTROL	0x04		/* All packs have ability to start/stop discharging */

mybmm_config_t *get_config(char *);
int reconfig(mybmm_config_t *conf);

#endif /* __MYBMM_CONFIG_H */
