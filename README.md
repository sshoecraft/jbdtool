
JBD BMS linux-based utility

MQTT support requires the phao.mqtt.c library https://github.com/eclipse/paho.mqtt.c

build & install the library then edit the Makefile and set MQTT=yes

To build MQTT - paho.mqtt.c (https://github.com/eclipse/paho.mqtt.c.git)

        mkdir -p build && cd build
        cmake -DPAHO_BUILD_SHARED=TRUE -DPAHO_BUILD_STATIC=TRUE -DPAHO_ENABLE_TESTING=FALSE -DPAHO_BUILD_SAMPLES=FALSE ..
        make && make install


BLUETOOTH support requires glib-2.0 gio-2.0.  Install the development packages and set BLUETOOTH=yes


Transports are specified as:

jbdtool -t transport:target,opt1[,optN]

For CAN:

jbdtool -t can:device[,speed]

example:

	jbdtool -t can:can0,500000

For Serial:

jbdtool -t serial:device[,speed]

example:

	jbdtool -t serial:/dev/ttyS0,9600

For Bluetooth:

jbdtool -t bt:mac,descriptor

exmples:

	jbdtool -t bt:01:02:03:04:05,06

	jbdtool -t bt:01:02:03:04:05:06,ff01

For IP/esplink:

jbdtool -t ip:address[,port]

example:

	jbdtool -t ip:10.0.0.1,23

for CANServer/Can-over-ip

jbdtool -t can_ip:address,[port],interface on server,[speed]

example:

	jbdtool -t can_ip:10.0.0.1,3930,can0,500000


>>> CAN bus cannot read/write parameters


to read all parameters using bluetooth:

jbdtool -t bt:01:02:03:04:06 -r -a

to list the params the program supports, use -l

to specify single params, specify them after -r

jbdtool -t bt:01:02:03:04:06 -r BalanceStartVoltage BalanceWindow

to read a list of parameters using a file use -f:

jbdtool -t serial:/dev/ttyS0,9600 -r -f jbd_settings.fig

use -j and -J (pretty) to specify filename is a json file


to write parameters, specify a key value pair after -w

jbdtool -t ip:10.0.0.1 -w BalanceStartVoltage 4050 BalanceWindow 20

Values are read and written in the BMS's own register units - what -r prints
is what -w takes:

	pack voltages		10 mV	(16.8V = 1680)
	cell voltages		mV	(4.2V = 4200)
	currents		10 mA	(30A = 3000)
	capacities		10 mAh	(75Ah = 7500)
	temperatures		0.1 K	(25C = 2981, ie. 2731 + C * 10)

A value in the wrong units is written as-is: a settings file with pack
voltages in mV instead of 10 mV left one reported pack with a 116V pack under
voltage limit, stuck in under voltage protection.

If an error stops a write partway (unknown parameter, bad value, failed
write) jbdtool exits non-zero.  Parameters written before the error may
already be applied.


Saving writes: -w and -W

-w exits factory mode with 0x0000.  On older firmware that saves the writes:
a 2.5 board (SP15S008C-P14S-100A) keeps them through power cycles.  Newer
firmware does not - a 3.5 board (SP04S010A-P4S-30A) was reported going back
to its old settings when the battery connector was pulled.

-W exits with 0x2828 instead, "exit and save", the only exit JBD's protocol
manual documents.  It saves on both.  It also ZEROES the protection event
counters (register 0xAA, see Protection counts below) - save the info output
first if you want to keep them.  The XiaoXiang app was reported to save with
0x2828 on every settings screen, so it clears them too.

	jbdtool -t ip:10.0.0.1 -W BalanceStartVoltage 4050


Hardware protection settings

DoubleOCSC, SCValue, SCDelay, DSGOC2 and DSGOC2Delay share register 0x38;
HCOVPDelay, HCUVPDelay and SCRelease share 0x39.  Writing one changes only
its own bits.  Except for SCRelease, the register holds an index into a
fixed list, so the value must be one from the list - anything else is an
error and nothing is written:

	DoubleOCSC	true, false (or 1, 0)
	SCValue		22 33 44 56 67 78 89 100
	SCDelay		70 100 200 400
	DSGOC2		8 11 14 17 19 22 25 28 31 33 36 39 42 44 47 50
	DSGOC2Delay	8 20 40 80 160 320 640 1280
	HCOVPDelay	1 2 4 8
	HCUVPDelay	1 4 8 16

With -n these are the raw index instead, both ways: -r -n prints the index
and -w -n takes it (0 to list length - 1), so -n output writes back
unchanged.  Mixing them is not: HCOVPDelay index 2 is 4 seconds, but
"-w HCOVPDelay 2" without -n sets 2 seconds.
Before 1.9.5 writing any of these sent random bytes to the register; if a
pack was written with an older jbdtool, check them.


Protection counts

The info output includes ProtectionCounts, the protection event counters
the BMS keeps (register 0xAA), in this order:

	ShortCircuitCount	short circuit
	ChargeOCCount		charge over current
	DischargeOCCount	discharge over current
	CellOVCount		cell over voltage
	CellUVCount		cell under voltage
	ChargeOTCount		charge over temperature
	ChargeUTCount		charge under temperature
	DischargeOTCount	discharge over temperature
	DischargeUTCount	discharge under temperature
	PackOVCount		pack over voltage
	PackUVCount		pack under voltage
	RestartCount		system restarts (not on older firmware)

It is an array like Cells and Temps; -F (flatten) outputs each one as its own
field with the names above, which is easier to chart from MQTT/JSON.


BatteryConfig and NtcConfig are bit fields.  They read back as a list of names
and can be written the same way, or as a number (decimal, or 0x for hex):

	jbdtool -t serial:/dev/ttyS0,9600 -w BatteryConfig SCRL,BALANCE_EN
	jbdtool -t serial:/dev/ttyS0,9600 -w BatteryConfig 6
	jbdtool -t serial:/dev/ttyS0,9600 -w NtcConfig NTC1,NTC2

The value replaces the whole field - names left out are cleared.  Writing an
unrecognized name is an error and nothing is written.

BatteryConfig bits:

	Switch		0x01	key/switch control
	SCRL		0x02	sleep/current recalibration
	BALANCE_EN	0x04	balancing enabled
	CHG_BALANCE	0x08	balance only while charging
	LED_EN		0x10	led enabled
	LED_NUM		0x20	led shows number of cells
	RTC		0x40	real time clock
	EDV		0x80	end of discharge voltage

NtcConfig bits: NTC1 (0x01) through NTC8 (0x80)


to send all output to a file, use -o.   If the filename ends with .json, file will be written in JSON format:

jbdtool -t can:can0,500000 -j -o pack_1.json


Raw registers

-R reads one register and shows it raw: a single word as signed, unsigned
and hex, a longer reply as every word in hex.

	jbdtool -t ip:10.0.0.1 -R 0x38

-X (reset BMS) is rejected by 2.5 firmware (the BMS answers 0x80) and does
nothing there.

scripts/jbd_raw_write.py writes raw register values over an ip target
without going through jbdtool's parameter code - for putting a register back
by hand.  It exits factory mode with 0x0000 unless given --save (0x2828,
which zeroes the protection counts):

	scripts/jbd_raw_write.py 10.0.0.1 38=9950 39=b005


Tests

	cd tests && make test

runs the offline tests (no BMS needed).  The hardware tests write to a real
BMS - read the header of each before running it:

	tests/hw_fields_hwtest.sh	writes the 0x38/0x39 settings back with their
				current values, then changes one field in each
				and restores it
	tests/eeprom_commit_test.sh	compares -w and -W across power cycles; it
				uses -W, which zeroes the protection counts
