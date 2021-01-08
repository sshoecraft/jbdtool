
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "mybmm.h"
#include "jbd.h"

struct jbd_session {
	mybmm_module_t *tp;		/* Our transport */
	void *tp_handle;		/* Our transport handle */
	mybmm_pack_t *pp;		/* Our pack info */
};
typedef struct jbd_session jbd_session_t;

static uint16_t jbd_crc(unsigned char *data, int len) {
	uint16_t crc = 0;
	register int i;

//	bindump("jbd_crc",data,len);
	dprintf(5,"len: %d\n", len);
	dprintf(5,"crc: %x\n", crc);
	for(i=0; i < len; i++) crc -= data[i];
	dprintf(5,"crc: %x\n", crc);
	return crc;
}

static int jbd_verify(uint8_t *buf, int len) {
	uint16_t my_crc,pkt_crc;
	int i,data_length;

	/* Anything less than 7 bytes is an error */
	dprintf(3,"len: %d\n", len);
	if (len < 7) return 1;
	if (debug >= 5) bindump("verify",buf,len);

	i=0;
	/* 0: Start bit */
	dprintf(5,"start bit: %x\n", buf[i]);
	if (buf[i++] != 0xDD) return 1;
	/* 1: Register */
	dprintf(5,"register: %x\n", buf[i]);
	i++;
	/* 2: Status */
	dprintf(5,"status: %d\n", buf[i]);
//	if (buf[i++] != 0) return 1;
	i++;
	/* 3: Length - must be size of packet minus protocol bytes */
	data_length = buf[i++];
	dprintf(5,"data_length: %d, len - 7: %d\n", data_length, len - 7);
	if (data_length && data_length != (len - 7)) return 1;
	/* Data */
	my_crc = jbd_crc(&buf[2],data_length+2);
	i += data_length;
	/* CRC */
	pkt_crc = _getshort(&buf[i]);
	dprintf(5,"my_crc: %x, pkt_crc: %x\n", my_crc, pkt_crc);
	if (my_crc != pkt_crc) return 1;
	i += 2;
	/* Stop bit */
	dprintf(5,"stop bit: %x\n", buf[i]);
	if (buf[i++] != 0x77) return 1;

	dprintf(3,"good data!\n");
	return 0;
}

static int jbd_cmd(uint8_t *pkt, int pkt_size, int action, uint16_t reg, uint8_t *data, int data_len) {
	unsigned short crc;
	int idx;

	/* Make sure no data in command */
	if (action == JBD_CMD_READ) data_len = 0;

	dprintf(5,"action: %x, reg: %x, data: %p, len: %d\n", action, reg, data, data_len);
	memset(pkt,0,pkt_size);
	idx = 0;
	pkt[idx++] = JBD_PKT_START;
	pkt[idx++] = action;
	pkt[idx++] = reg;
	pkt[idx++] = data_len;
	if (idx + data_len > pkt_size) return -1;
	memcpy(&pkt[idx],data,data_len);
	crc = jbd_crc(&pkt[2],data_len+2);
	idx += data_len;
	_putshort(&pkt[idx],crc);
	idx += 2;
	pkt[idx++] = JBD_PKT_END;
//	bindump("pkt",pkt,idx);
	dprintf(5,"returning: %d\n", idx);
	return idx;
}

static void jbd_get_protect(struct jbd_protect *p, unsigned short bits) {
#ifdef DEBUG
	{
		char bitstr[40];
		unsigned short mask = 1;
		int i;

		i = 0;
		while(mask) {
			bitstr[i++] = ((bits & mask) != 0 ? '1' : '0');
			mask <<= 1;
		}
		bitstr[i] = 0;
		dprintf(1,"protect: %s\n",bitstr);
	}
#endif
#if 0
Bit0 monomer overvoltage protection
Bit1 monomer under voltage protection
Bit2 whole group overvoltage protection
Bit3 whole group under voltage protection
Bit4 charging over temperature protection
Bit5 charging low temperature protection
Bit6 discharge over temperature protection
Bit7 discharge low temperature protection
Bit8 charging over-current protection
Bit9 discharge over current protection
Bit10 short circuit protection
Bit11 front end detection IC error
Bit12 software lock MOS
Bit13 ~ bit15 reserved
bit0	......
Single overvoltage protection
bit1	......
Single undervoltage protection
bit2	......
Whole group overvoltage protection
bit3	......
Whole group undervoltage protection
bit4	......
Charge over temperature protection
bit5	......
Charge low temperature protection
bit6	......
Discharge over temperature protection
bit7	......
Discharge low temperature protection
bit8	......
Charge overcurrent protection
bit9	......
Discharge overcurrent protection
bit10	....
Short circuit protection
bit11	....IC..
Front detection IC error
bit12	....MOS
Software lock MOS
      bit13~bit15	..
Reserved

#endif
}

#define         CRC_16_POLYNOMIALS  0xa001    
static unsigned short jbd_can_crc(unsigned char *pchMsg) {
	unsigned char i, chChar;
	unsigned short wCRC = 0xFFFF;
	int wDataLen = 6;

	while (wDataLen--) {
		chChar = *pchMsg++;
		wCRC ^= (unsigned short) chChar;
		for (i = 0; i < 8; i++) {
			if (wCRC & 0x0001)
				wCRC = (wCRC >> 1) ^ CRC_16_POLYNOMIALS;
			else
				wCRC >>= 1;
		}
	}
	return wCRC;
}


static int jbd_can_get(jbd_session_t *s, int id, unsigned char *data, int len, int chk) {
        unsigned short crc, mycrc;
	uint8_t cmd[2];
	int retries;

	dprintf(3,"id: %03x, data: %p, len: %d\n", id, data, len);
	_putshort(cmd,id);
	retries = 3;
	do {
		if (s->tp->read(s->tp_handle,cmd,2,data,len) < 0) return 1;
		if (chk) {
			/* Verify CRC */
			crc = _getshort(&data[6]);
			dprintf(3,"crc: %x\n", crc);
			mycrc = jbd_can_crc(data);
			dprintf(3,"mycrc: %x\n", mycrc);
			if (crc == 0 || crc == mycrc) return 0;
		}
	} while(retries--);
	printf("ERROR: CRC failed retries for ID %03x!\n", id);
	return 1;
}

static int jbd_can_get_crc(jbd_session_t *s, int id, unsigned char *data, int len) {
	return jbd_can_get(s,id,data,len,1);
}


/* For CAN bus only */
int jbd_can_get_pack(struct jbd_session *s) {
	mybmm_pack_t *pp = s->pp;
	uint8_t data[8];
	int id,i;
	struct jbd_protect prot;

	/* 1-16 balance */
	if (jbd_can_get_crc(s,0x102,data,8)) return 1;
	pp->balancebits = _getshort(&data[0]);

	/* 17 - 33 */
	pp->balancebits |= _getshort(&data[2]) << 16;

	/* Protectbits */
	jbd_get_protect(&prot,_getshort(&data[4]));

	if (jbd_can_get_crc(s,0x104,data,8)) return 1;
	pp->cells = data[0];
	dprintf(1,"strings: %d\n", pp->cells);
	pp->ntemps = data[1];
	dprintf(1,"probes: %d\n", pp->ntemps);

	/* Get Temps */
	i = 0;
#define CTEMP(v) ( (v - 2731) / 10 )
	for(id = 0x105; id < 0x107; id++) {
		if (jbd_can_get_crc(s,id,data,8)) return 1;
		pp->temps[i++] = CTEMP((float)_getshort(&data[0]));
		if (i >= pp->ntemps) break;
		pp->temps[i++] = CTEMP((float)_getshort(&data[2]));
		if (i >= pp->ntemps) break;
		pp->temps[i++] = CTEMP((float)_getshort(&data[4]));
		if (i >= pp->ntemps) break;
	}

	/* Cell volts */
	i = 0;
	for(id = 0x107; id < 0x111; id++) {
		if (jbd_can_get_crc(s,id,data,8)) return 1;
		pp->cellvolt[i++] = (float)_getshort(&data[0]) / 1000;
		if (i >= pp->cells) break;
		pp->cellvolt[i++] = (float)_getshort(&data[2]) / 1000;
		if (i >= pp->cells) break;
		pp->cellvolt[i++] = (float)_getshort(&data[4]) / 1000;
		if (i >= pp->cells) break;
	}

	return 0;
}

int jbd_rw(jbd_session_t *s, uint8_t action, uint8_t reg, uint8_t *data, int datasz) {
	uint8_t cmd[256],buf[256];
	int cmdlen,bytes,retries;

	dprintf(5,"action: %x, reg: %x, data: %p, datasz: %d\n", action, reg, data, datasz);
	cmdlen = jbd_cmd(cmd, sizeof(cmd), action, reg, data, datasz);
	if (debug >= 5) bindump("cmd",cmd,cmdlen);

	/* Read the data */
	retries=3;
	while(1) {
		dprintf(5,"retries: %d\n", retries);
		if (!retries--) {
			dprintf(5,"returning: -1\n");
			return -1;
		}
		dprintf(1,"writing...\n");
		bytes = s->tp->write(s->tp_handle,cmd,cmdlen);
		dprintf(5,"bytes: %d\n", bytes);
		bytes = s->tp->read(s->tp_handle,buf,sizeof(buf));
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) return -1;
		if (!jbd_verify(buf,bytes)) break;
		sleep(1);
	}
	memcpy(data,&buf[4],buf[3]);
	dprintf(5,"returning: %d\n",buf[3]);
	return buf[3];
}

static int jbd_get_pack(jbd_session_t *s) {
	mybmm_pack_t *pp = s->pp;
	uint8_t data[128];
	int i,bytes;;
	uint8_t fetstate;
	struct jbd_protect prot;

	dprintf(3,"getting HWINFO...\n");
	if ((bytes = jbd_rw(s, JBD_CMD_READ, JBD_CMD_HWINFO, data, sizeof(data))) < 0) {
		dprintf(1,"returning 1!\n");
		return 1;
	}

	pp->voltage = (float)_getshort(&data[0]) / 100.0;
	pp->current = (float)_getshort(&data[2]) / 100.0;
	pp->capacity = (float)_getshort(&data[6]) / 100.0;
        dprintf(2,"voltage: %.2f\n", pp->voltage);
        dprintf(2,"current: %.2f\n", pp->current);
        dprintf(2,"capacity: %.2f\n", pp->capacity);

	/* Balance */
	pp->balancebits = _getshort(&data[12]);
	pp->balancebits |= _getshort(&data[14]) << 16;

	/* Protectbits */
	jbd_get_protect(&prot,_getshort(&data[16]));

	fetstate = data[20];
	dprintf(2,"fetstate: %02x\n", fetstate);

	pp->cells = data[21];
	dprintf(2,"cells: %d\n", pp->cells);
	pp->ntemps = data[22];

	/* Temps */
#define CTEMP(v) ( (v - 2731) / 10 )
	for(i=0; i < pp->ntemps; i++) {
		pp->temps[i] = CTEMP((float)_getshort(&data[23+(i*2)]));
	}

	/* Cell volts */
	if ((bytes = jbd_rw(s, JBD_CMD_READ, JBD_CMD_CELLINFO, data, sizeof(data))) < 0) return 1;

	for(i=0; i < pp->cells; i++) {
		pp->cellvolt[i] = (float)_getshort(&data[i*2]) / 1000;
	}

	return 0;
}

static int jbd_init(mybmm_config_t *conf) {
#if 0
        struct cfg_proctab jbdconf[] = {
		CFG_PROCTAB_END
	};

	cfg_get_tab(conf->cfg,jbdconf);
	if (debug >= 3) cfg_disp_tab(jbdconf,"",1);
#endif
	return 0;
}

static void *jbd_new(mybmm_config_t *conf, ...) {
	jbd_session_t *s;
	va_list ap;
	mybmm_pack_t *pp;
	mybmm_module_t *tp;

	va_start(ap,conf);
	pp = va_arg(ap,mybmm_pack_t *);
	tp = va_arg(ap,mybmm_module_t *);
	va_end(ap);

	dprintf(3,"transport: %s\n", pp->transport);
	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("jbd_new: malloc");
		return 0;
	}
	/* Save a copy of the pack */
	s->pp = pp;
	s->tp = tp;
	s->tp_handle = tp->new(conf,pp->target,pp->params);
	if (!s->tp_handle) {
		free(s);
		return 0;
	}
	return s;
}

static int jbd_open(void *handle) {
	jbd_session_t *s = handle;
	return s->tp->open(s->tp_handle);
}

static int jbd_read(void *handle,...) {
	jbd_session_t *s = handle;
	int r;
	va_list ap;

	va_start(ap, handle);
	va_end(ap);
	dprintf(5,"transport: %s\n", s->tp->name);
	if (strcmp(s->tp->name,"can")==0) 
		r = jbd_can_get_pack(s);
	else
		r = jbd_get_pack(s);
	return r;
}

static int jbd_close(void *handle) {
	jbd_session_t *s = handle;
	return s->tp->close(s->tp_handle);
}

static int jbd_control(void *handle,...) {
//	jbd_session_t *s = handle;
	va_list ap;
	int op,action;

	va_start(ap, handle);
	op = va_arg(ap,int);
	dprintf(1,"op: %d\n", op);
	switch(op) {
	case MYBMM_BMS_CHARGE_CONTROL:
		action = va_arg(ap,int);
		dprintf(1,"action: %d\n", action);
		break;
	}
	va_end(ap);
	return 0;
}

EXPORT_API mybmm_module_t jbd_module = {
	MYBMM_MODTYPE_CELLMON,
	"jbd",
	MYBMM_BMS_CHARGE_CONTROL | MYBMM_BMS_DISCHARGE_CONTROL,
	jbd_init,			/* Init */
	jbd_new,			/* New */
	jbd_open,			/* Open */
	jbd_read,			/* Read */
	0,				/* Write */
	jbd_close,			/* Close */
	0,				/* Free */
	0,				/* Shutdown */
	jbd_control,			/* Control */
	0,			/* Config */
};

/* For jbdtool use */
#ifdef JBDTOOL
int jbd_eeprom_start(jbd_session_t *s) {
	uint8_t payload[2] = { 0x56, 0x78 };

	return jbd_rw(s, JBD_CMD_WRITE, JBD_REG_EEPROM, payload, sizeof(payload) );
}

int jbd_eeprom_end(jbd_session_t *s) {
	uint8_t payload[2] = { 0x00, 0x00 };

	return jbd_rw(s, JBD_CMD_WRITE, JBD_REG_CONFIG, payload, sizeof(payload) );
}

/* For CAN bus only */
int jbd_can_get_info(jbd_session_t *s, jbd_info_t *info) {
	unsigned char data[8];
	unsigned short mfgdate;
	int day,mon,year,id,i;

	if (jbd_can_get_crc(s,0x100,data,sizeof(data))) return 1;
	info->voltage = (float)_getshort(&data[0]) / 100.0;
	info->current = (float)_getshort(&data[2]) / 100.0;
	info->capacity = (float)_getshort(&data[4]) / 100.0;
	dprintf(1,"voltage: %.2f\n", info->voltage);
	dprintf(1,"current: %.2f\n", info->current);
	dprintf(1,"capacity: %.2f\n", info->capacity);

	if (jbd_can_get_crc(s,0x101,data,8)) return 1;
	info->fullcap = (float)_getshort(&data[0]) / 100.0;
	info->cycles = _getshort(&data[2]);
	info->pctcap = _getshort(&data[4]);
	dprintf(1,"fullcap: %.2f\n", info->fullcap);
	dprintf(1,"cycles: %d\n", info->cycles);
	dprintf(1,"pctcap: %d\n", info->pctcap);

	/* 1-16 */
	if (jbd_can_get_crc(s,0x102,data,8)) return 1;
	info->balancebits = _getshort(&data[0]);
	/* 17 - 33 */
	info->balancebits |= _getshort(&data[2]) << 16;
#ifdef DEBUG
	{
		char bits[40];
		unsigned short mask = 1;
		i = 0;
		while(mask) {
			bits[i++] = ((info->balancebits & mask) != 0 ? '1' : '0');
			mask <<= 1;
		}
		bits[i] = 0;
		dprintf(1,"balance1: %s\n",bits);
	}
#endif

	/* protection */
	info->protectbits = _getshort(&data[4]);
#ifdef DEBUG
	{
		char bits[40];
		unsigned short mask = 1;
		i = 0;
		while(mask) {
			bits[i++] = ((info->protectbits & mask) != 0 ? '1' : '0');
			mask <<= 1;
		}
		bits[i] = 0;
		dprintf(1,"protect: %s\n",bits);
	}
#endif

	if (jbd_can_get_crc(s,0x103,data,8)) return 1;
	info->fetstate = _getshort(&data[0]);
	mfgdate = _getshort(&data[2]);
	day = mfgdate & 0x1f;
	mon = (mfgdate >> 5) & 0x0f;
	year = 2000 + (mfgdate >> 9);
	dprintf(3,"year: %d, mon: %d, day: %d\n", year, mon, day);
	sprintf(info->mfgdate,"%04d%02d%02d",year,mon,day);
	info->version = (float)_getshort(&data[4]) / 10.0;
	dprintf(1,"fetstate: %d\n", info->fetstate);
	dprintf(1,"mfgdate: %s\n", info->mfgdate);
	dprintf(1,"version: %.1f\n", info->version);

	if (jbd_can_get_crc(s,0x104,data,8)) return 1;
	info->strings = data[0];
	info->probes = data[1];
	dprintf(1,"strings: %d\n", info->strings);
	dprintf(1,"probes: %d\n", info->probes);

	/* Temps */
	i = 0;
#define CTEMP(v) ( (v - 2731) / 10 )
	for(id = 0x105; id < 0x107; id++) {
		if (jbd_can_get_crc(s,id,data,8)) return 1;
		info->temps[i++] = CTEMP((float)_getshort(&data[0]));
		if (i >= info->probes) break;
		info->temps[i++] = CTEMP((float)_getshort(&data[2]));
		if (i >= info->probes) break;
		info->temps[i++] = CTEMP((float)_getshort(&data[4]));
		if (i >= info->probes) break;
	}

#define FTEMP(v) ( ( ( (float)(v) * 9.0 ) / 5.0) + 32.0)
	for(i = 0; i < info->probes; i++) {
		dprintf(1,"temp %d: %.3f\n", i, FTEMP(info->temps[i]));
	}

	/* Cell volts */
	i = 0;
	for(id = 0x107; id < 0x111; id++) {
		if (jbd_can_get_crc(s,id,data,8)) return 1;
		info->cellvolt[i++] = (float)_getshort(&data[0]) / 1000;
		if (i >= info->strings) break;
		info->cellvolt[i++] = (float)_getshort(&data[2]) / 1000;
		if (i >= info->strings) break;
		info->cellvolt[i++] = (float)_getshort(&data[4]) / 1000;
		if (i >= info->strings) break;
	}

	if (jbd_can_get_crc(s,0x111,data,8)) return 1;
	memcpy(&info->model[0],data,8);
	if (jbd_can_get_crc(s,0x112,data,8)) return 1;
	memcpy(&info->model[8],data,8);
	if (jbd_can_get_crc(s,0x113,data,8)) return 1;
	memcpy(&info->model[16],data,8);
	if (jbd_can_get_crc(s,0x114,data,8)) return 1;
	memcpy(&info->model[24],data,8);
	dprintf(1,"model: %s\n", info->model);
	return 0;
}

static int jbd_std_get_info(jbd_session_t *s, jbd_info_t *info) {
	unsigned char data[256];
	unsigned short mfgdate;
	int day,mon,year,i;

	if (jbd_rw(s, JBD_CMD_READ, JBD_CMD_HWINFO, data, sizeof(data)) < 0) return 1;

        info->voltage = (float)_getshort(&data[0]) / 100.0;
        info->current = (float)_getshort(&data[2]) / 100.0;
        info->capacity = (float)_getshort(&data[4]) / 100.0;
        dprintf(1,"voltage: %.2f\n", info->voltage);
        dprintf(1,"current: %.2f\n", info->current);
        dprintf(1,"capacity: %.2f\n", info->capacity);

	info->fullcap = (float)_getshort(&data[6]) / 100.0;
	info->cycles = _getshort(&data[8]);
	info->pctcap = data[19];
	dprintf(1,"fullcap: %.2f\n", info->fullcap);
	dprintf(1,"cycles: %d\n", info->cycles);
	dprintf(1,"pctcap: %x\n", info->pctcap);

	/* 1-16 */
	info->balancebits = _getshort(&data[12]);
#ifdef DEBUG
	{
		char bits[40];
		unsigned short mask = 1;
		i = 0;
		while(mask) {
			bits[i++] = ((info->balancebits & mask) != 0 ? '1' : '0');
			mask <<= 1;
		}
		bits[i] = 0;
		dprintf(1,"balance: %s\n",bits);
	}
#endif

	/* protection */
	info->protectbits = _getshort(&data[16]);
#ifdef DEBUG
	{
		char bits[40];
		unsigned short mask = 1;
		i = 0;
		while(mask) {
			bits[i++] = ((info->protectbits & mask) != 0 ? '1' : '0');
			mask <<= 1;
		}
		bits[i] = 0;
		dprintf(1,"protect: %s\n",bits);
	}
#endif

	info->fetstate = data[20];
	mfgdate = _getshort(&data[10]);
	day = mfgdate & 0x1f;
	mon = (mfgdate >> 5) & 0x0f;
	year = 2000 + (mfgdate >> 9);
	dprintf(3,"year: %d, mon: %d, day: %d\n", year, mon, day);
	sprintf(info->mfgdate,"%04d%02d%02d",year,mon,day);
	info->version = data[18] / 10.0;
	dprintf(1,"fetstate: %x\n", info->fetstate);
	dprintf(1,"mfgdate: %s\n", info->mfgdate);
	dprintf(1,"version: %.1f\n", info->version);

	info->strings = data[21];
	info->probes = data[22];
	dprintf(1,"strings: %d\n", info->strings);
	dprintf(1,"probes: %d\n", info->probes);

	/* Temps */
#define CTEMP(v) ( (v - 2731) / 10 )
	for(i=0; i < info->probes; i++) {
		info->temps[i] = CTEMP((float)_getshort(&data[23+(i*2)]));
	}

#define FTEMP(v) ( ( ( (float)(v) * 9.0 ) / 5.0) + 32.0)
	for(i = 0; i < info->probes; i++) {
		dprintf(1,"temp %d: %.3f\n", i, FTEMP((float)info->temps[i]/100.0));
	}

	/* Cell volts */
	if (jbd_rw(s, JBD_CMD_READ, JBD_CMD_CELLINFO, data, sizeof(data)) < 0) return 1;

	for(i=0; i < info->strings; i++) {
		info->cellvolt[i] = (float)_getshort(&data[i*2]) / 1000;
	}

	/* HWVER */
	i = jbd_rw(s, JBD_CMD_READ, JBD_CMD_HWVER, (uint8_t *)info->model, sizeof(info->model));
	if (i < 0) return 1;
	info->model[i] = 0;

	return 0;
}

int jbd_get_info(jbd_session_t *s, jbd_info_t *info) {
	int r,i;

	dprintf(1,"transport: %s\n", s->tp->name);
	if (strcmp(s->tp->name,"can")==0) 
		r = jbd_can_get_info(s,info);
	else
		r = jbd_std_get_info(s,info);
	if (r != 0) return r;
	dprintf(1,"r: %d\n", r);

	/* Fill in the protect struct */
	jbd_get_protect(&info->protect,info->protectbits);

	info->cell_min = 4.2;
	info->cell_max = 0.0;
	info->cell_total = 0;
	for(i = 0; i < info->strings; i++) {
		dprintf(1,"cell %d: %.3f\n", i, info->cellvolt[i]);
		if (info->cellvolt[i] < info->cell_min) info->cell_min = info->cellvolt[i];
		if (info->cellvolt[i] > info->cell_max) info->cell_max = info->cellvolt[i];
		info->cell_total += info->cellvolt[i];
	}
	info->cell_diff = info->cell_max - info->cell_min;
	info->cell_avg = info->cell_total / info->strings;
	dprintf(1,"cells: total: %.3f, min: %.3f, max: %.3f, diff: %.3f, avg: %.3f\n",
		info->cell_total, info->cell_min, info->cell_max, info->cell_diff, info->cell_avg);

	return 0;
}

#if 0
#define TEMP_BASE 22.22

static void get_pack_temp(mybmm_pack_t *pack, float *temps, int probes) {
	float min_temp,max_temp,min_diff,max_diff;
	int i;

#define FTEMP(v) ( ( ( (float)(v) * 9.0 ) / 5.0) + 32.0)
	min_temp = 9999.0;
	max_temp = 0.0;
	for(i = 0; i < probes; i++) {
		dprintf(5,"temp %d: %.3f\n", i, temps[i]);
		if (temps[i] < min_temp) min_temp = temps[i];
		if (temps[i] > max_temp) max_temp = temps[i];
	}
	dprintf(5,"temps: min: %.3f, max: %.3f\n", min_temp, max_temp);
	min_diff = (min_temp < TEMP_BASE ? TEMP_BASE - min_temp : min_temp - TEMP_BASE);
	max_diff = (max_temp < TEMP_BASE ? TEMP_BASE - max_temp : max_temp - TEMP_BASE);
	dprintf(5,"min_temp: %f, max_temp: %f, min_diff: %f, max_diff: %f\n",
		min_temp, max_temp, min_diff, max_diff);
	if (min_diff > max_diff) pack->temp = min_temp;
	else pack->temp = max_temp;
	dprintf(2,"temp: %f\n", pack->temp);
}
#endif
#endif /* JBDTOOL */
