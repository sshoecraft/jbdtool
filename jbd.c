
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "mybmm.h"
#include "jbd.h"
#ifndef __WIN32
#include <linux/can.h>
#endif

#define JBD_PKT_START		0xDD
#define JBD_PKT_END		0x77
#define JBD_CMD_READ		0xA5
#define JBD_CMD_WRITE		0x5A

#define JBD_CMD_HWINFO		0x03
#define JBD_CMD_CELLINFO	0x04
#define JBD_CMD_HWVER		0x05
#define JBD_CMD_MOS		0xE1

#define JBD_MOS_CHARGE          0x01
#define JBD_MOS_DISCHARGE       0x02

#define _getshort(p) ((short) ((*((p)) << 8) | *((p)+1) ))
#define _putshort(p,v) { float tmp; *((p)) = ((int)(tmp = v) >> 8); *((p+1)) = (int)(tmp = v); }

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

static int jbd_verify(uint8_t *buf, int len, uint8_t reg) {
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
	dprintf(5,"register: %x, wanted: %x\n", buf[i], reg);
	if (buf[i++] != reg) return 1;
	/* 2: Status */
	dprintf(5,"status: %d\n", buf[i]);
//	if (buf[i++] != 0) return 1;
	i++;
	/* 3: Length - must be size of packet minus protocol bytes */
	data_length = buf[i++];
	dprintf(5,"data_length: %d, len - 7: %d\n", data_length, len - 7);
	if (data_length != (len - 7)) return 1;
	/* Data */
	my_crc = jbd_crc(&buf[2],data_length+2);
	i += data_length;
	/* CRC */
	pkt_crc = _getshort(&buf[i]);
	dprintf(5,"my_crc: %x, pkt_crc: %x\n", my_crc, pkt_crc);
	if (my_crc != pkt_crc) {
		dprintf(1,"CRC ERROR: my_crc: %x, pkt_crc: %x\n", my_crc, pkt_crc);
		bindump("data",buf,len);
		return 1;
	}
	i += 2;
	/* Stop bit */
	dprintf(5,"stop bit: %x\n", buf[i]);
	if (buf[i++] != 0x77) return 1;

	dprintf(3,"good data!\n");
	return 0;
}

static int jbd_cmd(uint8_t *pkt, int pkt_size, int action, uint8_t reg, uint8_t *data, int data_len) {
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

void jbd_get_protect(struct jbd_protect *p, unsigned short bits) {
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
		dprintf(5,"protect: %s\n",bitstr);
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

#ifndef __WIN32
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


int jbd_can_get(jbd_session_t *s, int id, unsigned char *data, int datalen, int chk) {
        unsigned short crc, mycrc;
	int retries,bytes;

	dprintf(3,"id: %03x, data: %p, len: %d, chk: %d\n", id, data, datalen, chk);
	retries = 3;
	do {
		if (s->tp->write(s->tp_handle,id,0,0) < 0) return 1;
		memset(data,0,datalen);
		bytes = s->tp->read(s->tp_handle,id,data,datalen);
		dprintf(3,"bytes: %d\n", bytes);
		if (bytes < 0) return 1;
		if (!chk) break;
		/* Verify CRC */
		crc = _getshort(&data[6]);
		dprintf(3,"crc: %x\n", crc);
		mycrc = jbd_can_crc(data);
		dprintf(3,"mycrc: %x\n", mycrc);
		if (crc == 0 || crc == mycrc) break;
	} while(retries--);
	dprintf(3,"retries: %d\n", retries);
	if (!retries) printf("ERROR: CRC failed retries for ID %03x!\n", id);
	return (retries < 0);
}

int jbd_can_get_crc(jbd_session_t *s, int id, unsigned char *data, int len) {
	return jbd_can_get(s,id,data,len,1);
}


/* For CAN bus only */
int jbd_can_get_pack(struct jbd_session *s) {
	mybmm_pack_t *pp = s->pp;
	uint8_t data[8];
	int id,i;
	uint16_t protectbits,fetbits;
	struct jbd_protect prot;

	/* 0x102 Equalization state low byte, equalized state high byte, protection status */
	if (jbd_can_get_crc(s,0x102,data,8)) return 1;
	pp->balancebits = _getshort(&data[0]);
	pp->balancebits |= _getshort(&data[2]) << 16;
	protectbits = _getshort(&data[4]);
	/* Do we have any protection actions? */
	if (protectbits) {
		jbd_get_protect(&prot,protectbits);
	}

	/* 0x103 FET control status, production date, software version */
	if (jbd_can_get_crc(s,0x103,data,8)) return 1;
	fetbits = _getshort(&data[0]);
	dprintf(2,"fetbits: %02x\n", fetbits);
	if (fetbits & JBD_MOS_CHARGE) mybmm_set_state(pp,MYBMM_PACK_STATE_CHARGING);
	else mybmm_clear_state(pp,MYBMM_PACK_STATE_CHARGING);
	if (fetbits & JBD_MOS_DISCHARGE) mybmm_set_state(pp,MYBMM_PACK_STATE_DISCHARGING);
	else mybmm_clear_state(pp,MYBMM_PACK_STATE_DISCHARGING);
	if (s->balancing) mybmm_set_state(pp,MYBMM_PACK_STATE_BALANCING);
	else mybmm_clear_state(pp,MYBMM_PACK_STATE_BALANCING);

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
#endif

int jbd_rw(jbd_session_t *s, uint8_t action, uint8_t reg, uint8_t *data, int datasz) {
	uint8_t cmd[256],buf[256];
	int cmdlen,bytes,retries;

	dprintf(5,"action: %x, reg: %x, data: %p, datasz: %d\n", action, reg, data, datasz);
	cmdlen = jbd_cmd(cmd, sizeof(cmd), action, reg, data, datasz);
	if (debug >= 5) bindump("cmd",cmd,cmdlen);

	/* Read the data */
	retries=5;
	while(1) {
		dprintf(5,"retries: %d\n", retries);
		if (!retries--) {
			dprintf(5,"returning: -1\n");
			return -1;
		}
		dprintf(5,"writing...\n");
		bytes = s->tp->write(s->tp_handle,cmd,cmdlen);
		dprintf(5,"bytes: %d\n", bytes);
		memset(data,0,datasz);
		bytes = s->tp->read(s->tp_handle,buf,sizeof(buf));
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) return -1;
		if (!jbd_verify(buf,bytes,reg)) break;
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
	uint8_t fetbits;
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
#ifdef DEBUG
	{
		char bits[40];
		unsigned short mask = 1;
		i = 0;
		while(mask) {
			bits[i++] = ((pp->balancebits & mask) != 0 ? '1' : '0');
			mask <<= 1;
		}
		bits[i] = 0;
		dprintf(5,"balancebits: %s\n",bits);
	}
#endif

	/* Protectbits */
	jbd_get_protect(&prot,_getshort(&data[16]));

	fetbits = data[20];
	dprintf(2,"fetbits: %02x\n", fetbits);
	if (fetbits & JBD_MOS_CHARGE) mybmm_set_state(pp,MYBMM_PACK_STATE_CHARGING);
	else mybmm_clear_state(pp,MYBMM_PACK_STATE_CHARGING);
	if (fetbits & JBD_MOS_DISCHARGE) mybmm_set_state(pp,MYBMM_PACK_STATE_DISCHARGING);
	else mybmm_set_state(pp,MYBMM_PACK_STATE_DISCHARGING);
	if (s->balancing) mybmm_set_state(pp,MYBMM_PACK_STATE_BALANCING);
	else mybmm_clear_state(pp,MYBMM_PACK_STATE_BALANCING);

	pp->cells = data[21];
	dprintf(2,"cells: %d\n", pp->cells);
	pp->ntemps = data[22];
	dprintf(2,"ntemps: %d\n", pp->ntemps);

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
	dprintf(1,"pp->target: %s, pp->opts: %s\n", pp->target, pp->opts);
	s->tp_handle = tp->new(conf,pp->target,pp->opts);
	if (!s->tp_handle) {
		free(s);
		return 0;
	}
	return s;
}

static int jbd_open(void *handle) {
	jbd_session_t *s = handle;
	dprintf(1,"opening...\n");
	return s->tp->open(s->tp_handle);
}

static int jbd_read(void *handle,...) {
	jbd_session_t *s = handle;
	int r;
	va_list ap;

	va_start(ap, handle);
	va_end(ap);
	dprintf(5,"transport: %s\n", s->tp->name);
#ifndef __WIN32
	if (strncmp(s->tp->name,"can",3)==0) 
		r = jbd_can_get_pack(s);
	else
#endif
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
	case MYBMM_CHARGE_CONTROL:
		action = va_arg(ap,int);
		dprintf(1,"action: %d\n", action);
		break;
	}
	va_end(ap);
	return 0;
}

mybmm_module_t jbd_module = {
	MYBMM_MODTYPE_CELLMON,
	"jbd",
	MYBMM_CHARGE_CONTROL | MYBMM_DISCHARGE_CONTROL | MYBMM_BALANCE_CONTROL,
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
