
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

struct mybmm_config;
typedef struct mybmm_config mybmm_config_t;

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#ifndef __WIN32
#include <dlfcn.h>
#endif
//#include "util.h"
#include "parson.h"
#include "mybmm.h"
#include "jbd.h"		/* For session info */
#include "jbd_info.h"		/* For info struct */
#ifdef MQTT
#include "mqtt.h"
#endif

#define VERSION "1.8"
#include "build.h"

int debug = 0;

int outfmt = 0;
FILE *outfp;
char sepch;
char *sepstr;

JSON_Value *root_value;
JSON_Object *root_object;
char *serialized_string = NULL;

char *trim(char *);

int dont_interpret = 0;
int flat = 0;

enum JBD_PARM_DT {
	JBD_PARM_DT_UNK,
	JBD_PARM_DT_INT,		/* Std int/number */
	JBD_PARM_DT_FLOAT,		/* floating pt */
	JBD_PARM_DT_STR,		/* string */
	JBD_PARM_DT_TEMP,		/* temp */
	JBD_PARM_DT_DATE,		/* date */
	JBD_PARM_DT_PCT,		/* % */
	JBD_PARM_DT_FUNC,		/* function bits */
	JBD_PARM_DT_NTC,		/* ntc bits */
	JBD_PARM_DT_B0,			/* byte 0 */
	JBD_PARM_DT_B1,			/* byte 1 */
	JBD_PARM_DT_DOUBLE,
	JBD_PARM_DT_SCVAL,
	JBD_PARM_DT_SCDELAY,
	JBD_PARM_DT_DSGOC2,
	JBD_PARM_DT_DSGOC2DELAY,
	JBD_PARM_DT_HCOVPDELAY,
	JBD_PARM_DT_HCUVPDELAY,
	JBD_PARM_DT_DUMP,		/* short, ushort, hex */
};

#define JBD_FUNC_SWITCH 	0x01
#define JBD_FUNC_SCRL		0x02
#define JBD_FUNC_BALANCE_EN	0x04
#define JBD_FUNC_CHG_BALANCE	0x08
#define JBD_FUNC_LED_EN		0x10
#define JBD_FUNC_LED_NUM	0x20
#define JBD_FUNC_RTC		0x40
#define JBD_FUNC_EDV		0x80

#define JBD_NTC1		0x01
#define JBD_NTC2		0x02
#define JBD_NTC3		0x04
#define JBD_NTC4		0x08
#define JBD_NTC5		0x10
#define JBD_NTC6		0x20
#define JBD_NTC7		0x40
#define JBD_NTC8		0x80

struct jbd_params {
	uint8_t reg;
	char *label;
	int dt;
} params[] = {
//	{ JBD_FC,"FileCode",0 },
	{ JBD_REG_DCAP,"DesignCapacity", JBD_PARM_DT_INT },
	{ JBD_REG_CCAP,"CycleCapacity", JBD_PARM_DT_INT },
	{ JBD_REG_FULL,"FullChargeVol", JBD_PARM_DT_INT },
	{ JBD_REG_EMPTY,"ChargeEndVol", JBD_PARM_DT_INT },
	{ JBD_REG_RATE,"DischargingRate", JBD_PARM_DT_PCT },
	{ JBD_REG_MFGDATE,"ManufactureDate", JBD_PARM_DT_DATE },
	{ JBD_REG_SERIAL,"SerialNumber", JBD_PARM_DT_INT },
	{ JBD_REG_CYCLE,"CycleCount", JBD_PARM_DT_INT },
	{ JBD_REG_CHGOT,"ChgOverTemp", JBD_PARM_DT_TEMP },
	{ JBD_REG_RCHGOT,"ChgOTRelease", JBD_PARM_DT_TEMP },
	{ JBD_REG_CHGUT,"ChgLowTemp", JBD_PARM_DT_TEMP },
	{ JBD_REG_RCHGUT,"ChgUTRelease", JBD_PARM_DT_TEMP },
	{ JBD_REG_DSGOT,"DisOverTemp", JBD_PARM_DT_TEMP },
	{ JBD_REG_RDSGOT,"DsgOTRelease", JBD_PARM_DT_TEMP },
	{ JBD_REG_DSGUT,"DisLowTemp", JBD_PARM_DT_TEMP },
	{ JBD_REG_RDSGUT,"DsgUTRelease", JBD_PARM_DT_TEMP },
	{ JBD_REG_POVP,"PackOverVoltage", JBD_PARM_DT_INT },
	{ JBD_REG_RPOVP,"PackOVRelease", JBD_PARM_DT_INT },
	{ JBD_REG_PUVP,"PackUnderVoltage", JBD_PARM_DT_INT },
	{ JBD_REG_RPUVP,"PackUVRelease", JBD_PARM_DT_INT },
	{ JBD_REG_COVP,"CellOverVoltage", JBD_PARM_DT_INT },
	{ JBD_REG_RCOVP,"CellOVRelease", JBD_PARM_DT_INT },
	{ JBD_REG_CUVP,"CellUnderVoltage", JBD_PARM_DT_INT },
	{ JBD_REG_RCUVP,"CellUVRelease", JBD_PARM_DT_INT },
	{ JBD_REG_CHGOC,"OverChargeCurrent", JBD_PARM_DT_INT },
	{ JBD_REG_DSGOC,"OverDisCurrent", JBD_PARM_DT_INT },
	{ JBD_REG_BALVOL,"BalanceStartVoltage", JBD_PARM_DT_INT },
	{ JBD_REG_BALPREC,"BalanceWindow", JBD_PARM_DT_INT },
	{ JBD_REG_CURRES,"SenseResistor", JBD_PARM_DT_INT },
	{ JBD_REG_FUNCMASK,"BatteryConfig", JBD_PARM_DT_FUNC },
	{ JBD_REG_NTCMASK,"NtcConfig", JBD_PARM_DT_NTC },
	{ JBD_REG_STRINGS,"PackNum", JBD_PARM_DT_INT },
	{ JBD_REG_FETTIME,"fet_ctrl_time_set", JBD_PARM_DT_INT },
	{ JBD_REG_LEDTIME,"led_disp_time_set", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP80,"VoltageCap80", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP60,"VoltageCap60", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP40,"VoltageCap40", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP20,"VoltageCap20", JBD_PARM_DT_INT },
	{ JBD_REG_HCOVP,"HardCellOverVoltage", JBD_PARM_DT_INT },
	{ JBD_REG_HCUVP,"HardCellUnderVoltage", JBD_PARM_DT_INT },
	{ JBD_REG_HCOC,"DoubleOCSC", JBD_PARM_DT_DOUBLE },
	{ JBD_REG_HCOC,"SCValue", JBD_PARM_DT_SCVAL },
	{ JBD_REG_HCOC,"SCDelay", JBD_PARM_DT_SCDELAY },
	{ JBD_REG_HCOC,"DSGOC2", JBD_PARM_DT_DSGOC2 },
	{ JBD_REG_HCOC,"DSGOC2Delay", JBD_PARM_DT_DSGOC2DELAY },
	{ JBD_REG_HTRT,"HCOVPDelay", JBD_PARM_DT_HCOVPDELAY },
	{ JBD_REG_HTRT,"HCUVPDelay",JBD_PARM_DT_HCUVPDELAY },
	{ JBD_REG_HTRT,"SCRelease",JBD_PARM_DT_B1 },
	{ JBD_REG_CHGDELAY,"ChgUTDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_CHGDELAY,"ChgOTDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_DSGDELAY,"DsgUTDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_DSGDELAY,"DsgOTDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_PVDELAY,"PackUVDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_PVDELAY,"PackOVDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_CVDELAY,"CellUVDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_CVDELAY,"CellOVDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_CHGOCDELAY,"ChgOCDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_CHGOCDELAY,"ChgOCRDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_DSGOCDELAY,"DsgOCDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_DSGOCDELAY,"DsgOCRDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_MFGNAME,"ManufacturerName", JBD_PARM_DT_STR },
	{ JBD_REG_MODEL,"DeviceName", JBD_PARM_DT_STR },
	{ JBD_REG_BARCODE,"BarCode", JBD_PARM_DT_STR },
	{ JBD_REG_GPSOFF,"GPS_VOL", JBD_PARM_DT_INT },
	{ JBD_REG_GPSOFFTIME,"GPS_TIME", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP90,"VoltageCap90", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP70,"VoltageCap70", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP50,"VoltageCap50", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP30,"VoltageCap30", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP10,"VoltageCap10", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP100,"VoltageCap100", JBD_PARM_DT_INT },
	{ JBD_REG_MOSFET,"Mosfet", JBD_PARM_DT_INT },
	{ 0,0,0 }
};
typedef struct jbd_params jbd_params_t;

struct jbd_params *_getp(char *label) {
	register struct jbd_params *pp;

	dprintf(3,"label: %s\n", label);
	for(pp = params; pp->label; pp++) {
		dprintf(3,"pp->label: %s\n", pp->label);
		if (strcmp(pp->label,label)==0) {
			return pp;
		}
	}
	return 0;
}

void dint(char *label, char *format, int val) {
	char temp[128];

	dprintf(3,"label: %s, val: %d\n", label, val);
	switch(outfmt) {
	case 2:
		json_object_set_number(root_object, label, val);
		break;
	case 1:
		sprintf(temp,"%%s,%s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	default:
		sprintf(temp,"%%-25s %s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	}
}
#define _dint(l,v) dint(l,"%d",v)

void dbool(char *label, int val) {
	dprintf(3,"label: %s, val: %d\n", label, val);
	switch(outfmt) {
	case 2:
		json_object_set_boolean(root_object, label, val);
		break;
	case 1:
		if (dont_interpret)
			fprintf(outfp,"%s,%d\n",label,val);
		else
			fprintf(outfp,"%s,%s\n",label,val ? "true" : "false");
		break;
	default:
		if (dont_interpret)
			fprintf(outfp,"%-25s %d\n",label,val);
		else
			fprintf(outfp,"%-25s %s\n",label,val ? "true" : "false");
		break;
	}
}

void dfloat(char *label, char *format, float val) {
	char temp[128];

	dprintf(3,"dint: label: %s, val: %f\n", label, val);
	switch(outfmt) {
	case 2:
		json_object_set_number(root_object, label, val);
		break;
	case 1:
		sprintf(temp,"%%s,%s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	default:
		sprintf(temp,"%%-25s %s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	}
}
#define _dfloat(l,v) dfloat(l,"%f",v)

void dstr(char *label, char *format, char *val) {
	char temp[128];

	dprintf(3,"dint: label: %s, val: %s\n", label, val);
	switch(outfmt) {
	case 2:
		json_object_set_string(root_object, label, val);
		break;
	case 1:
		sprintf(temp,"%%s,%s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	default:
		sprintf(temp,"%%-25s %s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	}
}
#define _dstr(l,v) dstr(l,"%s",v)

static inline void _addstr(char *str,char *newstr) {
	dprintf(4,"str: %s, newstr: %s\n", str, newstr);
	if (strlen(str)) strcat(str,sepstr);
	dprintf(4,"str: %s\n", str);
	if (outfmt == 2) strcat(str,"\"");
	strcat(str,newstr);
	if (outfmt == 2) strcat(str,"\"");
	dprintf(4,"str: %s\n", str);
}

void _dump(char *label, short val) {
	char str[64],temp[72];

	dprintf(3,"label: %s, val: %d\n", label, val);
	str[0] = 0;
	sprintf(temp,"%d",val);
	_addstr(str,temp);
	sprintf(temp,"%d",(unsigned short)val);
	_addstr(str,temp);
	sprintf(temp,"%04x",(unsigned short)val);
	_addstr(str,temp);
	dprintf(3,"str: %s\n",str);
	switch(outfmt) {
	case 2:
		sprintf(temp,"[ %s ]",str);
		dprintf(2,"temp: %s\n", temp);
		json_object_dotset_value(root_object, label, json_parse_string(temp));
		break;
	case 1:
		printf("%s,%s\n",label,str);
		break;
	default:
		printf("%-25s %s\n",label,str);
		break;
	}
}

void pdisp(char *label, int dt, uint8_t *data, int len) {
	char str[64],temp[72];
	uint16_t val;

	dprintf(3,"label: %s, dt: %d\n", label, dt);
	switch(dt) {
	case JBD_PARM_DT_INT:
	case JBD_PARM_DT_TEMP:
	case JBD_PARM_DT_DATE:
	case JBD_PARM_DT_PCT:
		_dint(label,(int)_getshort(data));
		break;
	case JBD_PARM_DT_B0:
		_dint(label,data[0]);
		break;
	case JBD_PARM_DT_B1:
		_dint(label,data[1]);
		break;
	case JBD_PARM_DT_FUNC:
		if (dont_interpret) {
			_dint(label,(int)_getshort(data));
		} else {
			val = _getshort(data);
			str[0] = 0;
			if (val & JBD_FUNC_SWITCH) _addstr(str,"Switch");
			if (val & JBD_FUNC_SCRL)  _addstr(str,"SCRL");
			if (val & JBD_FUNC_BALANCE_EN) _addstr(str,"BALANCE_EN");
			if (val & JBD_FUNC_CHG_BALANCE) _addstr(str,"CHG_BALANCE");
			if (val & JBD_FUNC_LED_EN) _addstr(str,"LED_EN");
			if (val & JBD_FUNC_LED_NUM) _addstr(str,"LED_NUM");
			if (val & JBD_FUNC_RTC) _addstr(str,"RTC");
			if (val & JBD_FUNC_EDV) _addstr(str,"EDV");
			switch(outfmt) {
			case 2:
				sprintf(temp,"[ %s ]",str);
				dprintf(1,"temp: %s\n", temp);
				json_object_dotset_value(root_object, label, json_parse_string(temp));
				break;
			case 1:
				dstr(label,"%s",str);
				break;
			default:
				dstr(label,"%s",str);
				break;
			}
		}	
		break;
	case JBD_PARM_DT_NTC:
		if (dont_interpret) {
			_dint(label,(int)_getshort(data));
		} else {
			val = _getshort(data);
			str[0] = 0;
			if (val & JBD_NTC1) _addstr(str,"NTC1");
			if (val & JBD_NTC2) _addstr(str,"NTC2");
			if (val & JBD_NTC3) _addstr(str,"NTC3");
			if (val & JBD_NTC4) _addstr(str,"NTC4");
			if (val & JBD_NTC5) _addstr(str,"NTC5");
			if (val & JBD_NTC6) _addstr(str,"NTC6");
			if (val & JBD_NTC7) _addstr(str,"NTC7");
			if (val & JBD_NTC8) _addstr(str,"NTC8");
			switch(outfmt) {
			case 2:
				sprintf(temp,"[ %s ]",str);
				dprintf(2,"temp: %s\n", temp);
				json_object_dotset_value(root_object, label, json_parse_string(temp));
				break;
			case 1:
				dstr(label,"%s",str);
				break;
			default:
				dstr(label,"%s",str);
				break;
			}
		}
		break;
	case JBD_PARM_DT_FLOAT:
		_dfloat(label,(float)_getshort(data));
		break;
	case JBD_PARM_DT_STR:
		data[len] = 0;
		trim((char *)data);
		_dstr(label,(char *)data);
		break;
	case JBD_PARM_DT_DUMP:
		_dump(label,_getshort(data));
		break;
	case JBD_PARM_DT_DOUBLE:
		dbool(label,((data[0] & 0x80) != 0));
		break;
	case JBD_PARM_DT_DSGOC2:
		if (dont_interpret) {
			_dint(label,data[1] & 0x0f);
		} else {
			int vals[] = { 8,11,14,17,19,22,25,28,31,33,36,39,42,44,47,50 };
			int i = data[1] & 0x0f;
			dprintf(1,"data[1]: %02x\n", data[1]);

			dprintf(1,"i: %d\n", i);
			_dint(label,vals[i]);
		}
		break;
	case JBD_PARM_DT_DSGOC2DELAY:
		if (dont_interpret) {
			_dint(label,(data[1] >> 4) & 0x07);
		} else {
			int vals[] = { 8,20,40,80,160,320,640,1280 };
			int i = (data[1] >> 4) & 0x07;
			dprintf(1,"data[1]: %02x\n", data[1]);

			dprintf(1,"i: %d\n", i);
			_dint(label,vals[i]);
		}
		break;
	case JBD_PARM_DT_SCVAL:
		if (dont_interpret) {
			_dint(label,data[0] & 0x07);
		} else {
			int vals[] = { 22,33,44,56,67,78,89,100 };
			int i = data[0] & 0x07;
			dprintf(1,"data[0]: %02x\n", data[0]);
			dprintf(1,"i: %d\n", i);
			_dint(label,vals[i]);
		}
		break;
	case JBD_PARM_DT_SCDELAY:
		if (dont_interpret) {
			_dint(label,(data[0] >> 3) & 0x03);
		} else {
			int vals[] = {  70,100,200,400 };
			int i = (data[0] >> 3) & 0x03;
			dprintf(1,"data[0]: %02x\n", data[0]);

			dprintf(1,"i: %d\n", i);
			_dint(label,vals[i]);
		}
		break;
	case JBD_PARM_DT_HCOVPDELAY:
		if (dont_interpret) {
			_dint(label,(data[0] >> 4) & 0x03);
		} else {
			int vals[] = {  1,2,4,8 };
			int i = (data[0] >> 4) & 0x03;
			dprintf(1,"data[0]: %02x\n", data[0]);

			dprintf(1,"i: %d\n", i);
			_dint(label,vals[i]);
		}
        

		break;
	case JBD_PARM_DT_HCUVPDELAY:
		if (dont_interpret) {
			_dint(label,(data[0] >> 6) & 0x03);
		} else {
			int vals[] = {  1,4,8,16 };
			int i = (data[0] >> 6) & 0x03;
			dprintf(1,"data[0]: %02x\n", data[0]);

			dprintf(1,"i: %d\n", i);
			_dint(label,vals[i]);
		}
		break;
	}
}

void display_info(jbd_info_t *info) {
	char label[16], temp[256],*p;
	int i;

	if (strlen(info->name)) dstr("Name","%s",info->name);
	dfloat("Voltage","%.3f",info->voltage);
	dfloat("Current","%.3f",info->current);
	dfloat("DesignCapacity","%.3f",info->fullcap);
	dfloat("RemainingCapacity","%.3f",info->capacity);
	_dint("PercentCapacity",info->pctcap);
	_dint("CycleCount",info->cycles);
	_dint("Probes",info->probes);
	_dint("Strings",info->strings);
	if (flat) {
		for(i=0; i < info->probes; i++) {
			sprintf(label,"temp_%02d",i);
			dfloat(label,"%.1f",info->temps[i]);
		}
		for(i=0; i < info->strings; i++) {
			sprintf(label,"cell_%02d",i);
			dfloat(label,"%.3f",info->cellvolt[i]);
		}
	} else {
		switch(outfmt) {
		case 2:
			p = temp;
			p += sprintf(p,"[ ");
			for(i=0; i < info->probes; i++) {
				if (i) p += sprintf(p,",");
				p += sprintf(p, "%.1f",info->temps[i]);
			}
			strcat(temp," ]");
			dprintf(1,"temp: %s\n", temp);
			json_object_dotset_value(root_object, "Temps", json_parse_string(temp));
			break;
		default:
			p = temp;
			for(i=0; i < info->probes; i++) {
				if (i) p += sprintf(p,"%c",sepch);
				p += sprintf(p, "%.1f",info->temps[i]);
			}
			dstr("Temps","%s",temp);
			break;
		}
	switch(outfmt) {
	case 2:
		p = temp;
		p += sprintf(p,"[ ");
		for(i=0; i < info->strings; i++) {
			if (i) p += sprintf(p,",");
			p += sprintf(p, "%.3f",info->cellvolt[i]);
		}
		strcat(temp," ]");
                dprintf(1,"temp: %s\n", temp);
                json_object_dotset_value(root_object, "Cells", json_parse_string(temp));
                break;
	default:
		p = temp;
		for(i=0; i < info->strings; i++) {
			if (i) p += sprintf(p,"%c",sepch);
			p += sprintf(p, "%.3f",info->cellvolt[i]);
		}
		dstr("Cells","%s",temp);
                break;
	}
	}
	{
		char bits[40];
		unsigned long mask = 1;
		i = 0;
		while(mask) {
			bits[i++] = ((info->balancebits & mask) != 0 ? '1' : '0');
			if (i >= info->strings) break;
			mask <<= 1;
		}
		bits[i] = 0;
		dstr("Balance","%s",bits);
	}
	dfloat("CellTotal","%.3f",info->cell_total);
	dfloat("CellMin","%.3f",info->cell_min);
	dfloat("CellMax","%.3f",info->cell_max);
	dfloat("CellDiff","%.3f",info->cell_diff);
	dfloat("CellAvg","%.3f",info->cell_avg);
	_dstr("DeviceName",info->model);
	_dstr("ManufactureDate",info->mfgdate);
	dfloat("Version","%.1f",info->version);
	if (dont_interpret) {
		_dint("FET",info->fetstate);
	} else {
		temp[0] = 0;
		p = temp;
		if (info->fetstate & JBD_MOS_CHARGE) p += sprintf(p,"Charge");
		if (info->fetstate & JBD_MOS_DISCHARGE) {
			if (info->fetstate & JBD_MOS_CHARGE) p += sprintf(p,sepstr);
			p += sprintf(p,"Discharge");
		}
		dstr("FET","%s",temp);
	}
#if 0
        unsigned long balancebits;
        /* the protection sign */
        unsigned short protectbits;
        struct {
                unsigned sover: 1;              /* Single overvoltage protection */
                unsigned sunder: 1;             /* Single undervoltage protection */
                unsigned gover: 1;              /* Whole group overvoltage protection */
                unsigned gunder: 1;             /* Whole group undervoltage protection */
                unsigned chitemp: 1;            /* Charge over temperature protection */
                unsigned clowtemp: 1;           /* Charge low temperature protection */
                unsigned dhitemp: 1;            /* Discharge over temperature protection */
                unsigned dlowtemp: 1;           /* Discharge low temperature protection */
                unsigned cover: 1;              /* Charge overcurrent protection */
                unsigned cunder: 1;             /* Discharge overcurrent protection */
                unsigned shorted: 1;            /* Short circuit protection */
                unsigned ic: 1;                 /* Front detection IC error */
                unsigned mos: 1;                /* Software lock MOS */
        } protect;
#endif
}

int init_pack(mybmm_pack_t *pp, mybmm_config_t *c, char *type, char *transport, char *target, char *opts, mybmm_module_t *cp, mybmm_module_t *tp) {
	memset(pp,0,sizeof(*pp));
	strcpy(pp->type,type);
	if (transport) strcpy(pp->transport,transport);
	if (target) strcpy(pp->target,target);
	if (opts) strcpy(pp->opts,opts);
        pp->open = cp->open;
        pp->read = cp->read;
        pp->close = cp->close;
        pp->handle = cp->new(c,pp,tp);
        return 0;
}

enum JBDTOOL_ACTION {
	JBDTOOL_ACTION_INFO=0,
	JBDTOOL_ACTION_READ,
	JBDTOOL_ACTION_WRITE,
	JBDTOOL_ACTION_LIST
};

int write_parm(void *h, struct jbd_params *pp, char *value) {
	uint8_t data[128];
	int len;

	dprintf(3,"h: %p, pp->label: %s, value: %s\n",h,pp->label,value);
	len = 2;
	dprintf(3,"dt: %d\n", pp->dt);
	switch(pp->dt) {
	case JBD_PARM_DT_INT:
	case JBD_PARM_DT_TEMP:
	case JBD_PARM_DT_DATE:
	case JBD_PARM_DT_PCT:
	case JBD_PARM_DT_FUNC:
	case JBD_PARM_DT_NTC:
		_putshort(data,atoi(value));
		break;
	case JBD_PARM_DT_B0:
		if (jbd_rw(h, JBD_CMD_READ, pp->reg, data, sizeof(data)) < 1) return -1;
		data[0] = atoi(value);
		break;
	case JBD_PARM_DT_B1:
		if (jbd_rw(h, JBD_CMD_READ, pp->reg, data, sizeof(data)) < 1) return -1;
		data[1] = atoi(value);
		break;
	case JBD_PARM_DT_FLOAT:
		_putshort(data,atof(value));
		break;
	case JBD_PARM_DT_STR:
		len = strlen(value);
		memcpy(data,value,len);
		break;
	}
//	bindump("write data",data,len);
	return jbd_rw(h, JBD_CMD_WRITE, pp->reg, data, len);
}

#ifdef MQTT
#define MQTT_OPTS " [-m <mqtt_info>] [-i <mqtt_update_interval>]"
#define MQTT_GETOPT "m:i:"
#else
#define MQTT_OPTS ""
#define MQTT_GETOPT ""
#endif

void usage() {
	printf("jbdtool version %s build %lld\n",VERSION,BUILD);
	printf("usage: jbdtool [-abcjJFrwlhXN] [-f filename] [-t <module:target>] [-o output file]" MQTT_OPTS "\n");
	printf("arguments:\n");
#ifdef DEBUG
	printf("  -d <#>		debug output\n");
#endif
	printf("  -c		comma-delimited output\n");
	printf("  -g <on|off>	charging on/off\n");
	printf("  -G <on|off>	discharging on/off\n");
	printf("  -j		JSON output\n");
	printf("  -J		JSON output pretty print\n");
	printf("  -F 		Flatten JSON arrays\n");
	printf("  -r		read parameters\n");
	printf("  -a		read all parameters\n");
	printf("  -w		write parameters\n");
	printf("  -l		list supported parameters\n");
	printf("  -h		this output\n");
	printf("  -f <filename>	input filename for read/write.\n");
	printf("  -o <filename>	output filename\n");
	printf("  -t <transport:target> transport & target\n");
	printf("  -e <opts>	transport-specific opts\n");
	printf("  -n 		numbers only; dont interpret\n");
#ifdef MQTT
	printf("  -m <host:clientid:topic[:user[:pass]]> Send results to MQTT broker\n");
	printf("  -i 		Update interval\n");
#endif
	printf("  -X 		reset BMS\n");
	printf("  -N 		dont wait if locked\n");
}

int main(int argc, char **argv) {
	int opt,bytes,action,pretty,all,i,reg,dump;
	int charge,discharge,reset;
	char *transport,*target,*label,*filename,*outfile,*p,*opts;
	mybmm_config_t *conf;
	mybmm_module_t *cp,*tp;
	mybmm_pack_t pack;
	jbd_info_t info;
	jbd_params_t *pp;
	uint8_t data[128];
#ifdef MQTT
	int interval;
	char *mqtt;
#endif
	char lockfile[256];
	int lockfd,dont_wait;

	charge = discharge = -1;
	action = pretty = outfmt = all = reg = dump = flat = reset = dont_wait = 0;
	sepch = ',';
	sepstr = ",";
	transport = target = label = filename = outfile = opts = 0;
#ifdef MQTT
	interval = 0;
	mqtt = 0;
	char mqtt_topic[128];
#endif
	while ((opt=getopt(argc, argv, "+acDg:G:d:nNt:e:f:R:jJo:rwlhFX" MQTT_GETOPT)) != -1) {
		switch (opt) {
		case 'D':
			dump = 1;
			break;
		case 'a':
			all = 1;
			break;
		case 'c':
			outfmt=1;
			sepch = ' ';
			sepstr = " ";
			break;
		case 'd':
			debug=atoi(optarg);
			break;
		case 'g':
			if (strcasecmp(optarg,"on")==0)
				charge = 1;
			else if (strcasecmp(optarg,"off")==0)
				charge = 0;
			else {
				printf("error: invalid charge state: %s\n", optarg);
				usage();
				return 1;
			}
			break;
		case 'G':
			if (strcasecmp(optarg,"on")==0)
				discharge = 1;
			else if (strcasecmp(optarg,"off")==0)
				discharge = 0;
			else {
				printf("error: invalid discharge state: %s\n", optarg);
				usage();
				return 1;
			}
			break;
#ifdef MQTT
		case 'm':
			mqtt = optarg;
			break;
                case 'i':
			interval=atoi(optarg);
			break;
#endif
                case 'f':
			filename = optarg;
			break;
		case 'j':
			outfmt=2;
			pretty = 0;
			break;
		case 'J':
			outfmt=2;
			pretty = 1;
			break;
                case 'o':
			outfile = optarg;
			break;
                case 'n':
			dont_interpret = 1;
			break;
                case 'N':
			dont_wait = 1;
			break;
                case 't':
			transport = optarg;
			target = strchr(transport,':');
			if (!target) {
				printf("error: format is transport:target\n");
				usage();
				return 1;
			}
			*target = 0;
			target++;
			break;
		case 'e':
			opts = optarg;
			break;
                case 'R':
			action=JBDTOOL_ACTION_READ;
			if (strstr(optarg,"0x") || strncmp(optarg,"x",1)==0)
				reg = strtol(optarg,0,16);
			else
				reg = strtol(optarg,0,10);
			break;
                case 'r':
			action=JBDTOOL_ACTION_READ;
			break;
		case 'w':
			action=JBDTOOL_ACTION_WRITE;
			break;
                case 'l':
			for(pp = params; pp->label; pp++) printf("%s\n", pp->label);
			return 0;
			break;
		case 'F':
			flat=1;
			break;
		case 'X':
			reset=1;
			break;
		case 'h':
		default:
			usage();
			exit(0);
                }
        }
	dprintf(1,"transport: %p, target: %p\n", transport, target);
	if (!transport && action != JBDTOOL_ACTION_LIST) {
		usage();
		return 1;
	}

        log_open("mybmm",0,LOG_DEBUG|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR);

        argc -= optind;
        argv += optind;
        optind = 0;

	if ((action == JBDTOOL_ACTION_READ || action == JBDTOOL_ACTION_WRITE) && !filename && !argc && !all && !reg && !dump) {
		printf("error: a filename or parameter name or all (a) must be specified.\n");
		usage();
		return 1;
	}

	conf = calloc(sizeof(*conf),1);
	if (!conf) {
		perror("calloc conf");
		return 1;
	}
	conf->modules = list_create();
	memset(&info,0,sizeof(info));

	dprintf(2,"transport: %s\n", transport);

	tp = mybmm_load_module(conf,transport,MYBMM_MODTYPE_TRANSPORT);
	dprintf(1,"tp: %p\n", tp);
	if (!tp) return 1;
	cp = mybmm_load_module(conf,"jbd",MYBMM_MODTYPE_CELLMON);
	dprintf(1,"cp: %p\n", cp);
	if (!cp) return 1;

	/* Init the pack */
	p = strchr(target,',');
	if (p) {
		*p++ = 0;
		if (!opts) opts = p;
	}
	if (init_pack(&pack,conf,"jbd",transport,target,opts,cp,tp)) return 1;

#ifndef WINDOWS
	/* Lock the target */
	for(p = target + strlen(target); p >= target; p--) {
		if (*p != 0 && !isalnum(*p) && *p != '_' && *p != '-') {
			break;
		}
	}
	dprintf(2,"p: %p, target: %p\n", p, target);
	sprintf(lockfile,"/tmp/%s.lock", p+1);
	dprintf(2,"lockfile: %s\n", lockfile);
	lockfd = lock_file(lockfile,(dont_wait ? 0 : 1));
	dprintf(2,"lockfd: %d\n", lockfd);
	if (lockfd < 0) {
		log_error("unable to lock target");
		return 1;
	}
#endif

	/* If setting charge or discharge do that here */
	dprintf(2,"charge: %d, discharge: %d\n", charge, discharge);
	if (charge >= 0 || discharge >= 0) {
		opt = 0;
		if (pack.open(pack.handle)) return 1;
		if (jbd_get_info(pack.handle,&info) == 0) {
			dprintf(2,"fetstate: %x\n", info.fetstate);
			if (charge == 0 || (info.fetstate & JBD_MOS_CHARGE) == 0) opt |= JBD_MOS_CHARGE;
			if (charge == 1) opt = (opt & ~JBD_MOS_CHARGE);
			dprintf(2,"opt: %x\n", opt);
			if (discharge == 0 || (info.fetstate & JBD_MOS_DISCHARGE) == 0) opt |= JBD_MOS_DISCHARGE;
			if (discharge == 1) opt = (opt & ~JBD_MOS_DISCHARGE);
			dprintf(2,"opt: %x\n", opt);
			i = jbd_set_mosfet(pack.handle,opt);
			if (!i) {
				if (charge >= 0) printf("charging %s\n", charge ? "enabled" : "disabled");
				if (discharge >= 0) printf("discharging %s\n", discharge ? "enabled" : "disabled");
			}
		}
		if (pack.close(pack.handle)) return 1;
		return i;
	}

	/* Reset? */
	dprintf(2,"reset: %d\n", reset);
	if (reset) {
		printf("*** RESETTING ***\n");
		if (pack.open(pack.handle)) return 1;
		if (jbd_eeprom_start(pack.handle)) {
			pack.close(pack.handle);
			return 1;
		}
		jbd_reset(pack.handle);
		jbd_eeprom_end(pack.handle);
		pack.close(pack.handle);
		return 1;
	}
	
#ifdef MQTT
	/* If MQTT, output is compact JSON */
	dprintf(1,"mqtt: %p\n", mqtt);
	if (mqtt) {
		struct mqtt_config mc;

		memset(&mc,0,sizeof(mc));

		action = JBDTOOL_ACTION_INFO;
		outfmt = 2;
		strcpy(mc.host,strele(0,":",mqtt));
		strcpy(mc.clientid,strele(1,":",mqtt));
		strcpy(mqtt_topic,strele(2,":",mqtt));
		strcpy(mc.user,strele(3,":",mqtt));
		strcpy(mc.pass,strele(4,":",mqtt));
		dprintf(1,"host: %s, clientid: %s, topic: %s, user: %s, pass: %s\n", mc.host, mc.clientid, mqtt_topic, mc.user, mc.pass);
		if (strlen(mc.host) ==0 || strlen(mc.clientid) == 0 || strlen(mqtt_topic)==0) {
			printf("error: mqtt format is: host:clientid:topic[:user:pass]\n");
			return 1;
		}

		conf->mqtt = mqtt_new(&mc,0,0);
		if (!conf->mqtt) return 1;

		/* Test the connection */
		if (mqtt_connect(conf->mqtt,20)) return 1;
		mqtt_disconnect(conf->mqtt,10);

		strncat(info.name,mc.clientid,sizeof(info.name)-1);
		dprintf(1,"info.name: %s\n", info.name);
		pretty = 0;
	}
#endif

	if (outfile) {
		p = strrchr(outfile,'.');
		if (p) {
			dprintf(1,"p: %s\n", p);
			if (strcmp(p,".json")==0 || strcmp(p,".JSON")==0) {
				outfmt = 2;
				pretty = 1;
			}
		}
		dprintf(1,"outfile: %s\n", outfile);
		outfp = fopen(outfile,"w+");
		if (!outfp) {
			perror("fopen outfile");
			return 1;
		}
	} else {
		outfp = fdopen(1,"w");
	}
	dprintf(1,"outfp: %p\n", outfp);

	if (outfmt == 2) {
		root_value = json_value_init_object();
		root_object = json_value_get_object(root_value);
	}
	if (dump) {
//		char temp[16];

		if (pack.open(pack.handle)) return 1;
		if (jbd_eeprom_start(pack.handle)) return 1;
		for(i=0x10; i < 0xFF; i++) {
			bytes = jbd_rw(pack.handle, JBD_CMD_READ, i, data, sizeof(data));
			dprintf(3,"bytes: %d\n", bytes);
			if (bytes > 0) {
//				sprintf(temp,"Register %02x\n", reg);
//				pdisp(temp,JBD_PARM_DT_INT,data,bytes);
				printf("%02x: %d\n", i, _getshort(data));
			}
		}
		jbd_eeprom_end(pack.handle);
		pack.close(pack.handle);
		return 0;
	}
#ifdef MQTT
    do {
#endif
	switch(action) {
	case JBDTOOL_ACTION_INFO:
		if (pack.open(pack.handle)) return 1;
		if (jbd_get_info(pack.handle,&info) == 0) {
			display_info(&info);
		}
		pack.close(pack.handle);
		break;
	case JBDTOOL_ACTION_READ:
		if (strcmp(transport,"can")==0) {
			printf("error: reading parameters not possible using CAN bus\n");
			return 1;
		}
		if (reg) {
			char temp[16];

			if (pack.open(pack.handle)) return 1;
			if (jbd_eeprom_start(pack.handle)) return 1;
			bytes = jbd_rw(pack.handle, JBD_CMD_READ, reg, data, sizeof(data));
			dprintf(3,"bytes: %d\n", bytes);
			if (bytes > 0) {
				sprintf(temp,"Register %02x", reg);
				dprintf(2,"temp: %s\n", temp);
				pdisp(temp,JBD_PARM_DT_DUMP,data,bytes);
			}
			jbd_eeprom_end(pack.handle);
			pack.close(pack.handle);
		}
		if (filename) {
			char line[128];
			FILE *fp;

			dprintf(2,"filename: %s\n", filename);

			/* Get param names from .json file? */
			p = strrchr(filename,'.');
			if (p && strcmp(p,".json")==0) {
				JSON_Object *object;
				int count;

				root_value = json_parse_file(filename);
				if (json_value_get_type(root_value) != JSONObject) {
					printf("error: not a valid json file\n");
					return 1;
				}
				if (pack.open(pack.handle)) return 1;
				if (jbd_eeprom_start(pack.handle)) return 1;
				object = json_value_get_object(root_value);
				count  = json_object_get_count(object);
				for (i = 0; i < count; i++) {
					p = (char *)json_object_get_name(object, i);
					if (!p) {
						printf("error reading json file\n");
						return 1;
					}
					dprintf(3,"p: %s\n", p);
					pp = _getp(p);
					if (!pp) {
						printf("error: parm in json file not found: %s\n", p);
						return 1;
					}
					memset(data,0,sizeof(data));
					bytes = jbd_rw(pack.handle, JBD_CMD_READ, pp->reg, data, sizeof(data));
					if (bytes < 0) continue;
					dprintf(3,"bytes: %d\n", bytes);
					pdisp(pp->label,pp->dt,data,bytes);
				}
				jbd_eeprom_end(pack.handle);
				pack.close(pack.handle);
			} else {
				fp = fopen(filename,"r");
				if (!fp) {
					printf("fopen(r) %s: %s\n", filename, strerror(errno));
					return 1;
				}
				if (pack.open(pack.handle)) return 1;
				if (jbd_eeprom_start(pack.handle)) return 1;
				while(fgets(line,sizeof(line),fp)) {
					p = line;
					while(*p && isspace(*p)) p++;
					label = p;
					while(*p && !isspace(*p)) p++;
					*p = 0;
					pp = _getp(label);
					dprintf(3,"pp: %p\n", pp);
					if (!pp) continue;
					memset(data,0,sizeof(data));
					bytes = jbd_rw(pack.handle, JBD_CMD_READ, pp->reg, data, sizeof(data));
					if (bytes < 0) continue;
					dprintf(3,"bytes: %d\n", bytes);
					pdisp(pp->label,pp->dt,data,bytes);
				}
				jbd_eeprom_end(pack.handle);
				pack.close(pack.handle);
				fclose(fp);
			}
		} else {
			dprintf(1,"all: %d\n", all);
			if (pack.open(pack.handle)) return 1;
			if (jbd_eeprom_start(pack.handle)) return 1;
			if (all) {
				for(pp = params; pp->label; pp++) {
					dprintf(3,"pp->label: %s\n", pp->label);
					memset(data,0,sizeof(data));
					bytes = jbd_rw(pack.handle, JBD_CMD_READ, pp->reg, data, sizeof(data));
					if (bytes < 0) break;
					if (bytes) pdisp(pp->label,pp->dt,data,bytes);
				}
			} else {
				/* Every arg is a parm name */
				for(i=0; i < argc; i++) {
					pp = _getp(argv[i]);
					dprintf(2,"pp: %p\n", pp);
					if (!pp) {
						printf("error: parameter %s not found.\n",argv[i]);
						return 1;
					}
					memset(data,0,sizeof(data));
					bytes = jbd_rw(pack.handle, JBD_CMD_READ, pp->reg, data, sizeof(data));
					if (bytes > 0) pdisp(pp->label,pp->dt,data,bytes);
				}
			}
			jbd_eeprom_end(pack.handle);
			pack.close(pack.handle);
		}
		break;
	case JBDTOOL_ACTION_WRITE:
		if (strcmp(transport,"can")==0) {
			printf("error: writing parameters not possible using CAN bus\n");
			return 1;
		}
		if (filename) {
			char line[128],*valp;
			FILE *fp;

			dprintf(3,"filename: %s\n", filename);

			p = strrchr(filename,'.');
			if (p && strcmp(p,".json")==0) {
				char temp[128];
				JSON_Object *object;
				JSON_Value *value;
				int count,type,num;

				root_value = json_parse_file(filename);
				if (json_value_get_type(root_value) != JSONObject) {
					printf("error: not a valid json file\n");
					return 1;
				}
				if (pack.open(pack.handle)) return 1;
				if (jbd_eeprom_start(pack.handle)) return 1;
				object = json_value_get_object(root_value);
				count  = json_object_get_count(object);
				for (i = 0; i < count; i++) {
					p = (char *)json_object_get_name(object, i);
					if (!p) {
						printf("error reading json file\n");
						return 1;
					}
					dprintf(3,"p: %s\n", p);
					pp = _getp(p);
					if (!pp) {
						printf("error: parm in json file not found: %s\n", p);
						return 1;
					}
					value = json_object_get_value(object, pp->label);
					type = json_value_get_type(value);
					switch(type) {
					case JSONString:
						p = (char *)json_value_get_string(value);
						break;
					case JSONNumber:
						num = (int)json_value_get_number(value);
						dprintf(3,"value: %d\n", num);
						sprintf(temp,"%d",num);
						p = temp;
						break;
					case JSONBoolean:
						num = (int)json_value_get_boolean(value);
						dprintf(3,"value: %d\n", num);
						sprintf(temp,"%d",num);
						p = temp;
						break;
					default:
						printf("error: bad type in json file: %d\n", type);
						break;
					}
					if (write_parm(pack.handle,pp,p)) break;
				}
				jbd_eeprom_end(pack.handle);
				pack.close(pack.handle);
			} else {
				fp = fopen(filename,"r");
				if (!fp) {
					printf("fopen(r) %s: %s\n", filename, strerror(errno));
					return 1;
				}
				if (pack.open(pack.handle)) return 1;
				if (jbd_eeprom_start(pack.handle)) return 1;
				while(fgets(line,sizeof(line),fp)) {
					/* get parm */
					p = line;
					while(*p && isspace(*p)) p++;
					label = p;
					while(*p && !isspace(*p)) p++;
					*p = 0;
					dprintf(3,"label: %s\n", label);
					pp = _getp(label);
					dprintf(4,"pp: %p\n", pp);
					if (!pp) continue;
					/* get value */
					p++;
					while(*p && isspace(*p)) p++;
					valp = p;
					while(*p && !isspace(*p)) p++;
					*p = 0;
					dprintf(3,"valp: %s\n", valp);
					if (write_parm(pack.handle,pp,valp)) break;
				}
			}
			jbd_eeprom_end(pack.handle);
			pack.close(pack.handle);
		} else {
			/* Every arg par is a parm name & value */
			if (pack.open(pack.handle)) return 1;
			if (jbd_eeprom_start(pack.handle)) return 1;
			for(i=0; i < argc; i++) {
				/* Ge the parm */
				pp = _getp(argv[i]);
				dprintf(3,"pp: %p\n", pp);
				if (!pp) {
					printf("error: parameter %s not found.\n",argv[i]);
					break;
				}
				/* Get the value */
				if (i+1 == argc) {
					printf("error: no value for parameter %s\n",argv[i]);
					break;
				}
				i++;
				if (write_parm(pack.handle,pp,argv[i])) break;
			}
			jbd_eeprom_end(pack.handle);
			pack.close(pack.handle);
		}
		break;
	}
	if (outfmt == 2) {
		if (pretty)
	    		serialized_string = json_serialize_to_string_pretty(root_value);
		else
    			serialized_string = json_serialize_to_string(root_value);
#ifdef MQTT
		if (conf->mqtt) {
			if (mqtt_connect(conf->mqtt,10)) return 1;
			mqtt_pub(conf->mqtt,mqtt_topic,serialized_string, 0);
			mqtt_disconnect(conf->mqtt,5);
		}
		else
#endif
		fprintf(outfp,"%s",serialized_string);
		json_free_serialized_string(serialized_string);
	}
#ifdef MQTT
	sleep(interval);
    } while(interval);
#endif
#ifndef WINDOWS
	dprintf(2,"unlocking target\n");
	unlock_file(lockfd);
#endif
	json_value_free(root_value);
	fclose(outfp);

	return 0;
}
