
#ifndef __JBD_H__
#define __JBD_H__

#include "module.h"

struct jbd_session {
	mybmm_module_t *tp;		/* Our transport */
	void *tp_handle;		/* Our transport handle */
	mybmm_pack_t *pp;		/* Our pack info */
	int balancing;			/* Balance_en flag set in eeprom */
};
typedef struct jbd_session jbd_session_t;

/* Used by both jbdtool and mybmm */
struct jbd_protect {
	unsigned sover: 1;		/* Single overvoltage protection */
	unsigned sunder: 1;		/* Single undervoltage protection */
	unsigned gover: 1;		/* Whole group overvoltage protection */
	unsigned gunder: 1;		/* Whole group undervoltage protection */
	unsigned chitemp: 1;		/* Charge over temperature protection */
	unsigned clowtemp: 1;		/* Charge low temperature protection */
	unsigned dhitemp: 1;		/* Discharge over temperature protection */
	unsigned dlowtemp: 1;		/* Discharge low temperature protection */
	unsigned cover: 1;		/* Charge overcurrent protection */
	unsigned cunder: 1;		/* Discharge overcurrent protection */
	unsigned shorted: 1;		/* Short circuit protection */
	unsigned ic: 1;			/* Front detection IC error */
	unsigned mos: 1;		/* Software lock MOS */
};

int jbd_rw(jbd_session_t *, uint8_t action, uint8_t reg, uint8_t *data, int datasz);
void jbd_get_protect(struct jbd_protect *p, unsigned short bits);
int jbd_can_get_crc(jbd_session_t *s, int id, unsigned char *data, int len);
int jbd_can_get(jbd_session_t *s, int id, unsigned char *data, int datalen, int chk);
int jbd_reset(jbd_session_t *s);

#endif
