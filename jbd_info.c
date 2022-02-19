
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <string.h>
#include "jbd_info.h"
#include "debug.h"

#define _getshort(p) ((short) ((*((p)) << 8) | *((p)+1) ))
#define _putshort(p,v) { float tmp; *((p)) = ((int)(tmp = v) >> 8); *((p+1)) = (int)(tmp = v); }

int jbd_eeprom_start(jbd_session_t *s) {
	uint8_t payload[2] = { 0x56, 0x78 };

	dprintf(2,"opening eemprom...\n");
	return jbd_rw(s, JBD_CMD_WRITE, JBD_REG_EEPROM, payload, sizeof(payload) );
}

int jbd_eeprom_end(jbd_session_t *s) {
	uint8_t payload[2] = { 0x00, 0x00 };

	dprintf(2,"closing eemprom...\n");
	return jbd_rw(s, JBD_CMD_WRITE, JBD_REG_CONFIG, payload, sizeof(payload) );
}

int jbd_set_mosfet(jbd_session_t *s, int val) {
	uint8_t payload[2];
	int r;

	dprintf(2,"val: %x\n", val);
	_putshort(payload,val);
	if (jbd_eeprom_start(s) < 0) return 1;
	r = jbd_rw(s, JBD_CMD_WRITE, JBD_REG_MOSFET, payload, sizeof(payload));
	if (jbd_eeprom_end(s) < 0) return 1;
	return (r < 0 ? 1 : 0);
}

int jbd_reset(jbd_session_t *s) {
	uint8_t payload[2];
	int r;

#if 0
2021-06-05 16:33:25,846 INFO client 192.168.1.7:45868 -> server pack_01:23 (9 bytes)
-> 0000   DD 5A E3 02 43 21 FE B7 77                         .Z..C!..w
2021-06-05 16:33:26,032 INFO client 192.168.1.7:45868 <= server pack_01:23 (7 bytes)
<= 0000   DD E3 80 00 FF 80 77                               ......w
2021-06-05 19:34:58,910 INFO client 192.168.1.7:39154 -> server pack_01:23 (9 bytes)
-> 0000   DD 5A 0A 02 18 81 FF 5B 77                         .Z.....[w
2021-06-05 19:34:59,150 INFO client 192.168.1.7:39154 <= server pack_01:23 (7 bytes)
<= 0000   DD 0A 80 00 FF 80 77                               ......w
#endif

	_putshort(payload,0x4321);
	r = jbd_rw(s, JBD_CMD_WRITE, JBD_REG_RESET, payload, sizeof(payload));
	if (!r) {
		_putshort(payload,0x1881);
		r = jbd_rw(s, JBD_CMD_WRITE, JBD_REG_FRESET, payload, sizeof(payload));
	}
	return (r < 0 ? 1 : 0);
}

#ifndef __WIN32
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

	if (jbd_can_get(s,0x111,data,8,0)) return 1;
	memcpy(&info->model[0],data,8);
	if (jbd_can_get(s,0x112,data,8,0)) return 1;
	memcpy(&info->model[8],data,8);
	if (jbd_can_get(s,0x113,data,8,0)) return 1;
	memcpy(&info->model[16],data,8);
	if (jbd_can_get(s,0x114,data,8,0)) return 1;
	memcpy(&info->model[24],data,8);
	dprintf(1,"model: %s\n", info->model);
	return 0;
}
#endif

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
	info->balancebits |= _getshort(&data[14]) << 16;
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
		dprintf(1,"temp[%d]: %d\n",i,_getshort(&data[23+(i*2)]));
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
#ifndef __WIN32
	if (strncmp(s->tp->name,"can",3)==0) 
		r = jbd_can_get_info(s,info);
	else
#endif
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
