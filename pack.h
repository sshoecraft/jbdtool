
#ifndef __PACK_H
#define __PACK_H

#define MYBMM_MAX_TEMPS 8
#define MYBMM_MAX_CELLS 32

struct battery_cell {
	float voltage;
	unsigned char status;
	char *label;
};

#define CELL_STATUS_OK 		0x00
#define CELL_STATUS_WARNING 	0x01
#define CELL_STATUS_ERROR 	0x02
#define CELL_STATUS_UNDERVOLT	0x40
#define CELL_STATUS_OVERVOLT	0x80

struct mybmm_pack {
	char name[MYBMM_PACK_NAME_LEN];	/* Pack name */
	char uuid[37];			/* UUID */
	char type[32];			/* BMS name */
	char transport[32];		/* Transport name */
	char target[32];		/* Transport target */
	char opts[64];			/* Pack-specific options */
	unsigned char state;		/* Pack state */
	int failed;			/* Update fail count */
	char *errmsg;			/* Error message, updated by BMS */
	float capacity;			/* Battery pack capacity, in AH */
	float voltage;			/* Pack voltage */
	float current;			/* Pack current */
	int status;			/* Pack status, updated by BMS */
	int ntemps;			/* Number of temps */
	float temps[MYBMM_MAX_TEMPS];	/* Temp values */
	float temp;			/* Temp, in C */
	int cells;			/* Number of cells, updated by BMS */
//	battery_cell_t *cells;		/* Cell info */
	float cellvolt[MYBMM_MAX_CELLS]; /* Per-cell voltages, updated by BMS */
	uint32_t balancebits;		/* Balance bitmask */
	void *handle;			/* BMS Handle */
	mybmm_module_open_t open;	/* BMS Open */
	mybmm_module_read_t read;	/* BMS Read */
	mybmm_module_close_t close;	/* BMS Close */
	mybmm_module_control_t control;	/* BMS Control */
//	mybmm_config_t *conf;		/* Back pointer */
};
typedef struct mybmm_pack mybmm_pack_t;

/* Pack states */
#define MYBMM_PACK_STATE_UPDATED	0x01

#define MYBMM_BMS_CHARGE_CONTROL	0x01
#define MYBMM_BMS_DISCHARGE_CONTROL	0x02

int pack_init(mybmm_config_t *conf);

#endif
