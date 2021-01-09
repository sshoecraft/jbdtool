
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
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <dlfcn.h>
//#include "util.h"
#include "parson.h"
#include "mybmm.h"
#include "jbd.h"

int debug = 0;

int outfmt = 0;
FILE *outfp;
char sepch;
char *sepstr;

JSON_Value *root_value;
JSON_Object *root_object;
char *serialized_string = NULL;

char *trim(char *);

extern mybmm_module_t ip_module;

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
};

#define JBD_FUNC_SWITCH 	0x01
#define JBD_FUNC_SCRL		0x02
#define JBD_FUNC_BALANCE_EN	0x04
#define JBD_FUNC_LED_EN		0x08
#define JBD_FUNC_LED_NUM	0x10
#define JBD_FUNC_CHG_BALANCE	0x20

#define JBD_NTC1	0x01
#define JBD_NTC2	0x02
#define JBD_NTC3	0x04
#define JBD_NTC4	0x08
#define JBD_NTC5	0x10
#define JBD_NTC6	0x20
#define JBD_NTC7	0x40
#define JBD_NTC8	0x80

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
	{ JBD_REG_HCOC,"HardChgOverCurrent", JBD_PARM_DT_B0 },
	{ JBD_REG_HCOC,"HardDsgOverCurrent", JBD_PARM_DT_B1 },
	{ JBD_REG_HTRT,"HardTime", JBD_PARM_DT_B0 },
	{ JBD_REG_HTRT,"SCReleaseTime",JBD_PARM_DT_B1 },
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
	{ JBD_REG_IVOLCAP95,"IVOLCAP95", JBD_PARM_DT_INT },
	{ JBD_REG_IVOLCAP75,"IVOLCAP75", JBD_PARM_DT_INT },
	{ JBD_REG_IVOLCAP50,"IVOLCAP50", JBD_PARM_DT_INT },
	{ JBD_REG_IVOLCAP30,"IVOLCAP30", JBD_PARM_DT_INT },
	{ JBD_REG_IVOLCAP10,"IVOLCAP10", JBD_PARM_DT_INT },
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

	dprintf(3,"dint: label: %s, val: %d\n", label, val);
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

void disp(char *label, int dt, ...) {
	va_list ap;

	dprintf(3,"label: %s, dt: %d\n", label, dt);

	va_start(ap,dt);
	switch(dt) {
	default:
	case JBD_PARM_DT_INT:
		_dint(label,va_arg(ap,int));
		break;
	case JBD_PARM_DT_FLOAT:
		_dfloat(label,va_arg(ap,double));
		break;
	case JBD_PARM_DT_STR:
		_dstr(label,va_arg(ap,char *));
		break;
	case JBD_PARM_DT_TEMP:
		_dint(label,va_arg(ap,int));
		break;
	case JBD_PARM_DT_DATE:
		_dint(label,va_arg(ap,int));
		break;
	case JBD_PARM_DT_FUNC:
		_dint(label,va_arg(ap,int));
#if 0
		i = va_arg(ap,int);
		str[0] = 0;
		if (i & JBD_FUNC_SWITCH) _addstr(str,"Switch");
		if (i & JBD_FUNC_SCRL)  _addstr(str,"SCRL");
		if (i & JBD_FUNC_BALANCE_EN) _addstr(str,"BALANCE_EN");
		if (i & JBD_FUNC_LED_EN) _addstr(str,"LED_EN");
		if (i & JBD_FUNC_LED_NUM) _addstr(str,"LED_NUM");
		if (i & JBD_FUNC_CHG_BALANCE) _addstr(str,"CHG_BALANCE");
		switch(outfmt) {
		case 2:
			sprintf(temp,"[ %s ]",str);
			dprintf(1,"temp: %s\n", temp);
			json_object_dotset_value(root_object, label, json_parse_string(temp));
			break;
		case 1:
			printf("%s,%s\n",label,str);
			break;
		default:
			printf("%-25s %s\n",label,str);
			break;
		}
#endif
		break;
	case JBD_PARM_DT_NTC:
		_dint(label,va_arg(ap,int));
#if 0
		i = va_arg(ap,int);
		str[0] = 0;
		if (i & JBD_NTC1) _addstr(str,"NTC1");
		if (i & JBD_NTC2) _addstr(str,"NTC2");
		if (i & JBD_NTC3) _addstr(str,"NTC3");
		if (i & JBD_NTC4) _addstr(str,"NTC4");
		if (i & JBD_NTC5) _addstr(str,"NTC5");
		if (i & JBD_NTC6) _addstr(str,"NTC6");
		if (i & JBD_NTC7) _addstr(str,"NTC7");
		if (i & JBD_NTC8) _addstr(str,"NTC8");
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
#endif
		break;
        case JBD_PARM_DT_B0:
		_dint(label,va_arg(ap,int));
		break;
        case JBD_PARM_DT_B1:
		_dint(label,va_arg(ap,int));
		break;
	}
#if 0
        JBD_PARM_DT_PCT,                /* % */
#endif
}

void pdisp(char *label, int dt, uint8_t *data, int len) {
	dprintf(3,"label: %s, dt: %d\n", label, dt);
	switch(dt) {
	case JBD_PARM_DT_INT:
	case JBD_PARM_DT_TEMP:
	case JBD_PARM_DT_DATE:
	case JBD_PARM_DT_PCT:
	case JBD_PARM_DT_FUNC:
	case JBD_PARM_DT_NTC:
		_dint(label,(int)_getshort(data));
		break;
	case JBD_PARM_DT_B0:
		_dint(label,data[0]);
		break;
	case JBD_PARM_DT_B1:
		_dint(label,data[1]);
		break;
	case JBD_PARM_DT_FLOAT:
		_dfloat(label,(float)_getshort(data));
		break;
	case JBD_PARM_DT_STR:
		data[len] = 0;
		trim((char *)data);
		_dstr(label,(char *)data);
		break;
	}
}

void display_info(jbd_info_t *info) {
	char temp[256],*p;
	int i;

	dfloat("Voltage","%.3f",info->voltage);
	dfloat("Current","%.3f",info->current);
	dfloat("DesignCapacity","%.3f",info->fullcap);
	dfloat("CycleCapacity","%.3f",info->capacity);
	_dint("CycleCount",info->cycles);
	_dint("PercentCapacity",info->pctcap);
	_dint("Probes",info->probes);
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
	_dint("Strings",info->strings);
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
	dfloat("cell_total","%.3f",info->cell_total);
	dfloat("cell_min","%.3f",info->cell_min);
	dfloat("cell_max","%.3f",info->cell_max);
	dfloat("cell_diff","%.3f",info->cell_diff);
	dfloat("cell_avg","%.3f",info->cell_avg);
	_dstr("DeviceName",info->model);
	_dstr("ManufactureDate",info->mfgdate);
	dfloat("Version","%.1f",info->version);
	temp[0] = 0;
	p = temp;
	if (info->fetstate & JBD_MOS_CHARGE) p += sprintf(p,"Charge");
	if (info->fetstate & JBD_MOS_DISCHARGE) {
		if (info->fetstate & JBD_MOS_CHARGE) p += sprintf(p,sepstr);
		p += sprintf(p,"Discharge");
	}
	dstr("FET","%s",temp);
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
        unsigned short fetstat;                 /* for the MOS tube status */
        struct {
                unsigned charging: 1;
                unsigned discharging: 1;
        } fet;
#endif
}

int init_pack(mybmm_pack_t *pp, mybmm_config_t *c, char *type, char *transport, char *target, char *params, mybmm_module_t *cp, mybmm_module_t *tp) {
	memset(pp,0,sizeof(*pp));
	strcpy(pp->type,type);
	if (transport) strcpy(pp->transport,transport);
	if (target) strcpy(pp->target,target);
	if (params) strcpy(pp->params,params);
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

void usage() {
	printf("usage: jbdtool [-acjJrwlh] [-f filename] [-b <bluetooth mac addr | -i <ip addr>] [-o output file]\n");
	printf("arguments:\n");
	printf("  -a		All parameters (only valid for read -r)\n");
#ifdef DEBUG
	printf("  -d <#>		debug output\n");
#endif
	printf("  -c		comma-delimited output\n");
	printf("  -j		JSON output\n");
	printf("  -J		JSON output pretty print\n");
	printf("  -r		read parameters\n");
	printf("  -w		write parameters\n");
	printf("  -w		write parameters\n");
	printf("  -l		list supported parameters\n");
	printf("  -h		this output\n");
	printf("  -f <filename>	input filename for read/write.\n");
	printf("  -o <filename>	output filename\n");
	printf("  -b <mac addr>	bluetooth mac addr\n");
	printf("  -i <IP addr>	ip addr\n");
}

int main(int argc, char **argv) {
	int opt,bytes,action,pretty,all,i;
	char *transport,*target,*label,*filename,*outfile,*p;
	mybmm_config_t *conf;
	mybmm_module_t *cp,*tp;
	mybmm_pack_t pack;
	jbd_info_t info;
	jbd_params_t *pp;
	uint8_t data[128];

	action = pretty = outfmt = all = 0;
	sepch = ',';
	sepstr = ",";
	transport = target = label = filename = outfile = 0;
	while ((opt=getopt(argc, argv, "+ab:cd:i:f:jJo:rwlh")) != -1) {
		switch (opt) {
		case 'a':
			all = 1;
			break;
                case 'b':
			transport="bt";
			target=optarg;
			break;
		case 'c':
			outfmt=1;
			sepch = ' ';
			sepstr = " ";
			break;
		case 'd':
			debug=atoi(optarg);
			break;
                case 'i':
			transport="ip";
			target=optarg;
			break;
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
#if 0
                case 'n':
			action=JBDTOOL_ACTION_INFO;
			break;
#endif
#if 1
                case 'r':
			action=JBDTOOL_ACTION_READ;
			break;
		case 'w':
			action=JBDTOOL_ACTION_WRITE;
			break;
#else
                case 'r':
			action=1;
			if (optarg[0] == '@') filename = optarg+1;
			else label=optarg;
			break;
                case 'w':
			action=2;
			if (optarg[0] == '@') filename = optarg+1;
			else label=optarg;
			break;
#endif
                case 'l':
			for(pp = params; pp->label; pp++) printf("%s\n", pp->label);
			return 0;
			break;
		case 'h':
		default:
//			usage();
			exit(0);
                }
        }
	dprintf(1,"transport: %p, target: %p\n", transport, target);
	if (!transport && action != JBDTOOL_ACTION_LIST) {
		usage();
		return 1;
	}

        argc -= optind;
        argv += optind;
        optind = 0;

	if ((action == JBDTOOL_ACTION_READ || action == JBDTOOL_ACTION_WRITE) && !filename && !argc && !all) {
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

	dprintf(2,"transport: %s\n", transport);

	tp = mybmm_load_module(conf,transport,MYBMM_MODTYPE_TRANSPORT);
	if (!tp) return 1;
	cp = mybmm_load_module(conf,"jbd",MYBMM_MODTYPE_CELLMON);
	if (!cp) return 1;

	/* Init the pack */
	if (init_pack(&pack,conf,"jbd",transport,target,0,cp,tp)) return 1;

	if (outfile) {
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
	switch(action) {
	case JBDTOOL_ACTION_INFO:
		if (pack.open(pack.handle)) return 1;
		if (jbd_get_info(pack.handle,&info) == 0) display_info(&info);
		pack.close(pack.handle);
		break;
	case JBDTOOL_ACTION_READ:
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
				json_value_free(root_value);
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
					if (pack.open(pack.handle)) return 1;
					if (jbd_eeprom_start(pack.handle)) return 1;
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
					default:
						printf("error: bad type in json file: %d\n", type);
						break;
					}
					if (write_parm(pack.handle,pp,p)) break;
				}
				jbd_eeprom_end(pack.handle);
				pack.close(pack.handle);
				json_value_free(root_value);
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
		fprintf(outfp,"%s",serialized_string);
		json_free_serialized_string(serialized_string);
		json_value_free(root_value);
	}
	fclose(outfp);

	return 0;
}
