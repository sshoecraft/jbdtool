
#ifndef __JBD_INFO_H__
#define __JBD_INFO_H__

#include <stdint.h>
#include "jbd.h"

/* This is for jbdtool */
struct jbd_info {
	char name[32];
	float voltage;			/* the total voltage */
	float current;			/* the current, charging is positive, the discharge is negative */
	float capacity;			/* the remaining capacity */
	float fullcap;			/* full capacity */
	unsigned short cycles;		/* the number of discharge cycle , unsigned , the unit is 1 */
	unsigned short pctcap;		/* the remaining capacity (RSOC), unsigned, in units of % */
	unsigned long balancebits;
	/* the protection sign */
	unsigned short protectbits;
	struct jbd_protect protect;
	unsigned short fetstate;		/* for the MOS tube status */
	char mfgdate[9];			/* the production date, YYYYMMDD, zero terminated */
	float version;				/* the software version */
	unsigned char strings;			/* the number of battery strings */
	float cellvolt[32];			/* Cell voltages */
	float cell_total;			/* sum of all cells */
	float cell_min;				/* lowest cell value */
	float cell_max;				/* highest cell value */
	float cell_diff;			/* diff from lowest to highest */
	float cell_avg;				/* avergae cell value */
	unsigned char probes;			/* the number of NTC (temp) probes */
	float temps[6];				/* Temps */
	char model[32];				/* Model name */
};
typedef struct jbd_info jbd_info_t;

#define JBD_PKT_START		0xDD
#define JBD_PKT_END		0x77
#define JBD_CMD_READ		0xA5
#define JBD_CMD_WRITE		0x5A

#define JBD_CMD_HWINFO		0x03
#define JBD_CMD_CELLINFO	0x04
#define JBD_CMD_HWVER		0x05
#define JBD_CMD_MOS		0xE1

#define JBD_REG_EEPROM  	0x00
#define JBD_REG_CONFIG  	0x01

#define JBD_FC			3838
#define JBD_REG_FRESET		0x0A
#define JBD_REG_DCAP		0x10
#define JBD_REG_CCAP		0x11
#define JBD_REG_FULL		0x12
#define JBD_REG_EMPTY		0x13
#define JBD_REG_RATE		0x14
#define JBD_REG_MFGDATE		0x15
#define JBD_REG_SERIAL		0x16
#define JBD_REG_CYCLE		0x17
#define JBD_REG_CHGOT		0x18
#define JBD_REG_RCHGOT		0x19
#define JBD_REG_CHGUT		0x1A
#define JBD_REG_RCHGUT		0x1B
#define JBD_REG_DSGOT		0x1C
#define JBD_REG_RDSGOT		0x1D
#define JBD_REG_DSGUT		0x1E
#define JBD_REG_RDSGUT		0x1F
#define JBD_REG_POVP		0x20
#define JBD_REG_RPOVP		0x21
#define JBD_REG_PUVP		0x22
#define JBD_REG_RPUVP		0x23
#define JBD_REG_COVP		0x24
#define JBD_REG_RCOVP		0x25
#define JBD_REG_CUVP		0x26
#define JBD_REG_RCUVP		0x27
#define JBD_REG_CHGOC		0x28
#define JBD_REG_DSGOC		0x29
#define JBD_REG_BALVOL		0x2A
#define JBD_REG_BALPREC		0x2B
#define JBD_REG_CURRES		0x2C
#define JBD_REG_FUNCMASK	0x2D
#define JBD_REG_NTCMASK		0x2E
#define JBD_REG_STRINGS		0x2F
#define JBD_REG_FETTIME		0x30
#define JBD_REG_LEDTIME		0x31
#define JBD_REG_VOLCAP80	0x32
#define JBD_REG_VOLCAP60	0x33
#define JBD_REG_VOLCAP40	0x34
#define JBD_REG_VOLCAP20	0x35
#define JBD_REG_HCOVP		0x36		/* HardCellOverVoltage */
#define JBD_REG_HCUVP		0x37		/* HardCellUnderVoltage */
#define JBD_REG_HCOC		0x38		/* HardChg/DsgOverCurrent */
#define JBD_REG_HTRT		0x39		/* HardTime/SCReleaseTime */
#define JBD_REG_CHGDELAY	0x3A		/* low: O, high: U */
#define JBD_REG_DSGDELAY	0x3B		/* low: O, high: U */
#define JBD_REG_PVDELAY		0x3C		/* low: O, high: U */
#define JBD_REG_CVDELAY		0x3D		/* low: O, high: U */
#define JBD_REG_CHGOCDELAY	0x3E		/* low: release, high: delay */
#define JBD_REG_DSGOCDELAY	0x3F		/* low: release, high: delay */
#define JBD_REG_GPSOFF		0x40
#define JBD_REG_GPSOFFTIME	0x41
#define JBD_REG_VOLCAP90	0x42
#define JBD_REG_VOLCAP70	0x43
#define JBD_REG_VOLCAP50	0x44
#define JBD_REG_VOLCAP30	0x45
#define JBD_REG_VOLCAP10	0x46
#define JBD_REG_VOLCAP10	0x46
#define JBD_REG_VOLCAP100	0x47

#define JBD_REG_MFGNAME 	0xA0
#define JBD_REG_MODEL   	0xA1
#define JBD_REG_BARCODE   	0xA2
#define JBD_REG_ERROR   	0xAA

/* Cell Voltage Calibration */
#define JBD_REG_VCAL01		0xB0
#define JBD_REG_VCAL02		0xB1
#define JBD_REG_VCAL03		0xB2
#define JBD_REG_VCAL04		0xB3
#define JBD_REG_VCAL05		0xB4
#define JBD_REG_VCAL06		0xB5
#define JBD_REG_VCAL07		0xB6
#define JBD_REG_VCAL08		0xB7
#define JBD_REG_VCAL09		0xB8
#define JBD_REG_VCAL10		0xB9
#define JBD_REG_VCAL11		0xBA
#define JBD_REG_VCAL12		0xBB
#define JBD_REG_VCAL13		0xBC
#define JBD_REG_VCAL14		0xBD
#define JBD_REG_VCAL15		0xBE
#define JBD_REG_VCAL16		0xBF
#define JBD_REG_VCAL17		0xC0
#define JBD_REG_VCAL18		0xC1
#define JBD_REG_VCAL19		0xC2
#define JBD_REG_VCAL20		0xC3
#define JBD_REG_VCAL21		0xC4
#define JBD_REG_VCAL22		0xC5
#define JBD_REG_VCAL23		0xC6
#define JBD_REG_VCAL24		0xC7
#define JBD_REG_VCAL25		0xC8
#define JBD_REG_VCAL26		0xC9
#define JBD_REG_VCAL27		0xCA
#define JBD_REG_VCAL28		0xCB
#define JBD_REG_VCAL29		0xCC
#define JBD_REG_VCAL30		0xCD
#define JBD_REG_VCAL31		0xCE
#define JBD_REG_VCAL32		0xCF
#define JBD_REG_VCAL33		0xCF
    
/* NTC Calibration */
#define JBD_REG_TCAL00		0xD0
#define JBD_REG_TCAL01		0xD1
#define JBD_REG_TCAL02		0xD2
#define JBD_REG_TCAL03		0xD3
#define JBD_REG_TCAL04		0xD4
#define JBD_REG_TCAL05		0xD5
#define JBD_REG_TCAL06		0xD6
#define JBD_REG_TCAL07		0xD7

#define JBD_REG_CAPACITY	0xE0
#define JBD_REG_MOSFET		0xE1
#define JBD_REG_BALANCE		0xE2
#define JBD_REG_RESET           0xE3
#define JBD_REG_ICURCAL		0xAD
#define JBD_REG_CCURCAL		0xAE
#define JBD_REG_DCURCAL		0xAF
     
#define JBD_MOS_CHARGE		0x01
#define JBD_MOS_DISCHARGE	0x02

int jbd_get_info(jbd_session_t *, jbd_info_t *);
int jbd_eeprom_start(jbd_session_t *);
int jbd_eeprom_end(jbd_session_t *);
int jbd_set_mosfet(jbd_session_t *,int);

#define _getshort(p) ((short) ((*((p)) << 8) | *((p)+1) ))
#define _putshort(p,v) { float tmp; *((p)) = ((int)(tmp = v) >> 8); *((p+1)) = (int)(tmp = v); }

#endif
