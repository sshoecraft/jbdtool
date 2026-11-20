#!/bin/sh
#
# Hardware test: do -w (exit 0x0000) and -W (exit 0x2828) writes survive a
# power cycle, and which of them clears the protection event counters
# (register 0xAA)?
#
# usage: eeprom_commit_test.sh <jbdtool> <target>
#   jbdtool - build with -W
#   target  - transport:target, eg. ip:pack_01
#
# WARNING: any -W write ZEROES 0xAA on the pack.  Phase 2 does a -W write;
# the counters are printed first so they are not lost unrecorded.
#
# The pack has to be power cycled by hand twice (the -X reset is rejected
# by fw 2.5 boards, so it cannot be used to reload EEPROM).  If the pack is
# carrying a load, make sure something else can carry it first.
#
# SerialNumber is used as the scratch parameter and is restored at the end
# with -w.

BIN=$1
TARGET=$2

if [ -z "$BIN" -o -z "$TARGET" ]; then
	echo "usage: $0 <jbdtool> <target>"
	exit 1
fi

serial() {
	$BIN -t $TARGET -r SerialNumber | awk '{ print $2 }'
}

# raw 24-byte 0xAA payload (12 big-endian words)
counters() {
	$BIN -d 5 -t $TARGET -R 0xAA 2>&1 | grep -A2 'verify(31)' | tail -2 | \
		cut -c7-53 | tr -s ' \n' ' ' | cut -d' ' -f5-28
}

show() {
	echo "  SerialNumber: $(serial)"
	echo "  0xAA:         $(counters)"
}

power_cycle() {
	echo
	echo ">>> power cycle the BMS on $TARGET now, then press enter once it answers again"
	read junk
}

SN0=$(serial)
if [ -z "$SN0" ]; then
	echo "error: cannot read SerialNumber from $TARGET"
	exit 1
fi
echo "baseline"
show

echo
echo "1. -w SerialNumber=$((SN0+1)) (exit 0x0000)"
$BIN -t $TARGET -w SerialNumber $((SN0+1)) || exit 1
show
power_cycle
echo "   after power cycle - $((SN0+1)) means -w persists, $SN0 means it does not"
show

echo
echo "2. -W SerialNumber=$((SN0+2)) (exit 0x2828) - zeroes 0xAA"
$BIN -t $TARGET -W SerialNumber $((SN0+2)) || exit 1
show
power_cycle
echo "   after power cycle - $((SN0+2)) means -W persists"
show

echo
echo "3. restore SerialNumber=$SN0 with -w"
$BIN -t $TARGET -w SerialNumber $SN0
show
