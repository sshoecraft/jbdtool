/* Unit test for writing the indexed hardware protection fields in registers
   0x38 (HCOC) and 0x39 (HTRT): DoubleOCSC, SCValue, SCDelay, DSGOC2,
   DSGOC2Delay, HCOVPDelay, HCUVPDelay (and SCRelease, which shares 0x39).

   write_parm() used to have no case for these, so it sent 2 bytes of
   uninitialized stack to the register - and on boards that persist writes
   (or with -W) the garbage stayed in EEPROM.

   Includes main.c directly (renaming its main) and replaces jbd_rw() with a
   fake register file, so the real write_parm()/pdisp() run with no BMS.

   build & run:
	cd tests && make test
*/

#define main jbdtool_main
#define jbd_rw fake_jbd_rw
#include "../main.c"
#undef jbd_rw
#undef main

static uint8_t regs[256][2];
static int writes;
static int failed = 0;

int fake_jbd_rw(jbd_session_t *s, uint8_t action, uint8_t reg, uint8_t *data, int datasz) {
	if (action == JBD_CMD_READ) {
		data[0] = regs[reg][0];
		data[1] = regs[reg][1];
		return 2;
	}
	regs[reg][0] = data[0];
	regs[reg][1] = data[1];
	writes++;
	return 0;
}

static unsigned short reg16(int reg) {
	return (regs[reg][0] << 8) | regs[reg][1];
}

static void setreg(int reg, unsigned short val) {
	regs[reg][0] = val >> 8;
	regs[reg][1] = val & 0xFF;
}

/* write label=value, expect rc (0 ok, -1 rejected) and the resulting register */
static void chkw(char *label, char *value, int expect_rc, int reg, unsigned short expect) {
	jbd_params_t *pp = _getp(label);
	int rc,before;

	if (!pp) {
		printf("FAIL: no param %s\n", label);
		failed++;
		return;
	}
	before = writes;
	rc = write_parm(0,pp,value);
	if (rc != expect_rc) {
		printf("FAIL: %s=%s: rc %d, expected %d\n", label, value, rc, expect_rc);
		failed++;
		return;
	}
	if (rc && writes != before) {
		printf("FAIL: %s=%s: rejected but still wrote the register\n", label, value);
		failed++;
		return;
	}
	if (reg16(reg) != expect) {
		printf("FAIL: %s=%s: reg %02x = %04x, expected %04x\n", label, value, reg, reg16(reg), expect);
		failed++;
		return;
	}
	printf("ok: %s=%s -> rc %d, reg %02x = %04x\n", label, value, rc, reg, reg16(reg));
}

/* what pdisp() shows for label from the current register contents */
static double shown(char *label) {
	jbd_params_t *pp = _getp(label);
	double val;
	FILE *fp;

	fp = outfp;
	outfp = fopen("/dev/null","w");
	outfmt = 2;
	root_value = json_value_init_object();
	root_object = json_value_get_object(root_value);
	pdisp(pp->label,pp->dt,regs[pp->reg],2);
	if (pp->dt == JBD_PARM_DT_DOUBLE)
		val = json_object_get_boolean(root_object,pp->label);
	else
		val = json_object_get_number(root_object,pp->label);
	json_value_free(root_value);
	root_value = 0;
	fclose(outfp);
	outfp = fp;
	outfmt = 0;
	return val;
}

/* every table value written must read back as itself, and leave the
   other bits of the register alone */
static void roundtrip(char *label, int background) {
	jbd_params_t *pp = _getp(label);
	struct jbd_field *fp = jbd_field_get(pp->dt);
	int count = (fp->mask >> fp->shift) + 1;
	unsigned short owned = (fp->byte == 0 ? fp->mask << 8 : fp->mask);
	char value[16];
	int i;

	for(i=0; i < count; i++) {
		setreg(pp->reg,background);
		sprintf(value,"%d",fp->vals[i]);
		if (write_parm(0,pp,value) != 0) {
			printf("FAIL: %s=%s rejected\n", label, value);
			failed++;
			continue;
		}
		if ((reg16(pp->reg) & ~owned) != (background & ~owned)) {
			printf("FAIL: %s=%s changed other bits: %04x -> %04x\n", label, value, background, reg16(pp->reg));
			failed++;
			continue;
		}
		if (shown(label) != fp->vals[i]) {
			printf("FAIL: %s=%s reads back as %g\n", label, value, shown(label));
			failed++;
			continue;
		}
	}
	printf("ok: %s: all %d values round trip over %04x\n", label, count, background);
}

int main(void) {
	char *labels[] = { "SCValue","SCDelay","DSGOC2","DSGOC2Delay","HCOVPDelay","HCUVPDelay",0 };
	int i;

	setbuf(stdout,0);
	sepch = ',';
	sepstr = ",";
	outfp = fdopen(1,"w");

	/* the factory values from issue #22, in parameter table order, from a
	   cleared register: must give the factory 0x9950 / 0xB005 */
	setreg(JBD_REG_HCOC,0);
	setreg(JBD_REG_HTRT,0);
	chkw("DoubleOCSC","true",0,JBD_REG_HCOC,0x8000);
	chkw("SCValue","33",0,JBD_REG_HCOC,0x8100);
	chkw("SCDelay","400",0,JBD_REG_HCOC,0x9900);
	chkw("DSGOC2","8",0,JBD_REG_HCOC,0x9900);
	chkw("DSGOC2Delay","320",0,JBD_REG_HCOC,0x9950);
	chkw("HCOVPDelay","8",0,JBD_REG_HTRT,0x3000);
	chkw("HCUVPDelay","8",0,JBD_REG_HTRT,0xB000);
	chkw("SCRelease","5",0,JBD_REG_HTRT,0xB005);

	/* DoubleOCSC: true/false/1/0 (json booleans arrive as 1/0) */
	chkw("DoubleOCSC","false",0,JBD_REG_HCOC,0x1950);
	chkw("DoubleOCSC","1",0,JBD_REG_HCOC,0x9950);
	chkw("DoubleOCSC","0",0,JBD_REG_HCOC,0x1950);
	chkw("DoubleOCSC","TRUE",0,JBD_REG_HCOC,0x9950);

	/* rejected, and the register is not touched */
	setreg(JBD_REG_HCOC,0x9950);
	chkw("DoubleOCSC","yes",-1,JBD_REG_HCOC,0x9950);
	chkw("SCValue","34",-1,JBD_REG_HCOC,0x9950);
	chkw("SCValue","",-1,JBD_REG_HCOC,0x9950);
	chkw("SCValue","33x",-1,JBD_REG_HCOC,0x9950);
	chkw("SCDelay","3",-1,JBD_REG_HCOC,0x9950);
	chkw("DSGOC2Delay","-1",-1,JBD_REG_HCOC,0x9950);

	/* -n: the value is the index, the same thing -r -n prints.  HCOVPDelay
	   index 2 is 4 seconds - without -n "2" is the value 2 seconds (index 1) */
	setreg(JBD_REG_HTRT,0xB005);
	dont_interpret = 1;
	chkw("HCOVPDelay","2",0,JBD_REG_HTRT,0xA005);
	chkw("HCOVPDelay","3",0,JBD_REG_HTRT,0xB005);
	chkw("HCUVPDelay","3",0,JBD_REG_HTRT,0xF005);
	chkw("HCOVPDelay","4",-1,JBD_REG_HTRT,0xF005);
	chkw("HCOVPDelay","-1",-1,JBD_REG_HTRT,0xF005);
	chkw("HCOVPDelay","8",-1,JBD_REG_HTRT,0xF005);
	setreg(JBD_REG_HCOC,0x9950);
	chkw("SCValue","5",0,JBD_REG_HCOC,0x9D50);
	chkw("DSGOC2","15",0,JBD_REG_HCOC,0x9D5F);
	chkw("DSGOC2","16",-1,JBD_REG_HCOC,0x9D5F);
	dont_interpret = 0;
	setreg(JBD_REG_HTRT,0xB005);
	chkw("HCOVPDelay","2",0,JBD_REG_HTRT,0x9005);

	/* -n round trip: every index written reads back (as -r -n shows it)
	   as itself */
	dont_interpret = 1;
	for(i=0; labels[i]; i++) {
		jbd_params_t *pp = _getp(labels[i]);
		struct jbd_field *fp = jbd_field_get(pp->dt);
		int j,count = (fp->mask >> fp->shift) + 1;
		char value[16];

		for(j=0; j < count; j++) {
			setreg(pp->reg,0x5AA5);
			sprintf(value,"%d",j);
			if (write_parm(0,pp,value) != 0 || shown(labels[i]) != j) {
				printf("FAIL: -n %s=%d reads back as %g\n", labels[i], j, shown(labels[i]));
				failed++;
			}
		}
		printf("ok: -n %s: all %d indexes round trip\n", labels[i], count);
	}
	dont_interpret = 0;

	/* every value of every field, over all-zeros and all-ones */
	for(i=0; labels[i]; i++) {
		roundtrip(labels[i],0x0000);
		roundtrip(labels[i],0xFFFF);
	}

	/* a type write_parm doesnt know must be refused, not sent unset */
	{
		jbd_params_t bogus = { 0x99, "Bogus", JBD_PARM_DT_DUMP };
		int before = writes;

		if (write_parm(0,&bogus,"1") != -1 || writes != before) {
			printf("FAIL: unsupported type was written\n");
			failed++;
		} else {
			printf("ok: unsupported type refused\n");
		}
	}

	printf("%s\n", failed ? "FAILED" : "all passed");
	return failed ? 1 : 0;
}
