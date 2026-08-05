/* Unit test for the BatteryConfig/NtcConfig bit parser.

   Includes main.c directly (renaming its main) so the test runs against the
   real tables and real parse_bits() - no duplicated logic.

   build & run:
	cd tests && make test
*/

#define main jbdtool_main
#include "../main.c"
#undef main

static int failed = 0;

static void chk(char *value, struct jbd_bit *bits, int expect_rc, uint16_t expect_mask) {
	uint16_t mask;
	int rc;

	mask = 0xDEAD;
	rc = parse_bits(value,bits,&mask);
	if (rc != expect_rc) {
		printf("FAIL: \"%s\": rc %d, expected %d\n", value, rc, expect_rc);
		failed++;
		return;
	}
	if (rc == 0 && mask != expect_mask) {
		printf("FAIL: \"%s\": mask %04x, expected %04x\n", value, mask, expect_mask);
		failed++;
		return;
	}
	printf("ok: \"%s\" -> rc %d, mask %04x\n", value, rc, (rc ? 0 : mask));
}

/* round trip: mask -> pdisp() output -> parse_bits() -> same mask */
static void chkdisp(uint16_t mask, int dt, struct jbd_bit *bits) {
	uint8_t data[2];
	uint16_t back;
	char out[256],*p;
	FILE *fp;

	/* pdisp writes text formats to stdout/outfp, so capture via the json path */
	fp = outfp;
	outfp = fopen("/dev/null","w");
	outfmt = 2;
	root_value = json_value_init_object();
	root_object = json_value_get_object(root_value);

	data[0] = mask >> 8;
	data[1] = mask & 0xFF;
	pdisp("test",dt,data,2);

	out[0] = 0;
	{
		JSON_Array *array = json_object_get_array(root_object,"test");
		int i,count = (array ? json_array_get_count(array) : 0);

		for(i=0; i < count; i++) {
			if (strlen(out)) strcat(out,",");
			strcat(out,json_array_get_string(array,i));
		}
	}
	json_value_free(root_value);
	root_value = 0;
	fclose(outfp);
	outfp = fp;
	outfmt = 0;

	back = 0xDEAD;
	p = (strlen(out) ? out : "0");
	if (parse_bits(p,bits,&back) || back != mask) {
		printf("FAIL: %04x displayed as \"%s\", parsed back as %04x\n", mask, out, back);
		failed++;
		return;
	}
	printf("ok: %04x <-> \"%s\"\n", mask, out);
}

int main(void) {
	int i;

	setbuf(stdout,0);
	sepch = ',';
	sepstr = ",";
	outfp = fdopen(1,"w");

	/* the value that got wiped: SCRL,BALANCE_EN,CHG_BALANCE */
	chk("SCRL,BALANCE_EN,CHG_BALANCE",func_bits,0,0x0E);
	/* the write that caused the report - used to atoi() to 0 */
	chk("SCRL,BALANCE_EN",func_bits,0,0x06);
	/* seperators and case */
	chk("scrl|balance_en",func_bits,0,0x06);
	chk("SCRL + BALANCE_EN",func_bits,0,0x06);
	chk("SCRL BALANCE_EN",func_bits,0,0x06);
	chk(" SCRL , BALANCE_EN ",func_bits,0,0x06);
	/* every bit */
	chk("Switch,SCRL,BALANCE_EN,CHG_BALANCE,LED_EN,LED_NUM,RTC,EDV",func_bits,0,0x00FF);
	/* numbers still work */
	chk("6",func_bits,0,0x0006);
	chk("0",func_bits,0,0x0000);
	chk("0x0E",func_bits,0,0x000E);
	chk(" 14 ",func_bits,0,0x000E);
	/* ntc table */
	chk("NTC1,NTC2",ntc_bits,0,0x0003);
	chk("NTC8",ntc_bits,0,0x0080);
	/* rejected: dont write a bogus mask to the BMS */
	chk("BALANCE",func_bits,-1,0);
	chk("SCRL,BOGUS",func_bits,-1,0);
	chk("NTC1",func_bits,-1,0);
	chk("",func_bits,-1,0);
	chk("   ",func_bits,-1,0);
	chk("65536",func_bits,-1,0);
	chk("-1",func_bits,-1,0);
	chk("6x",func_bits,-1,0);

	/* every mask must survive display -> parse unchanged.  this is what
	   caught str[] being too small to hold all 8 quoted names in json */
	for(i=0; i <= 0xFF; i++) {
		chkdisp(i,JBD_PARM_DT_FUNC,func_bits);
		chkdisp(i,JBD_PARM_DT_NTC,ntc_bits);
	}

	printf("%s\n", failed ? "FAILED" : "all passed");
	return failed ? 1 : 0;
}
