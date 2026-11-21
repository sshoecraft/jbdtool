#!/bin/sh
#
# Hardware test for writing the hardware protection fields in registers
# 0x38/0x39 (DoubleOCSC, SCValue, SCDelay, DSGOC2, DSGOC2Delay, HCOVPDelay,
# HCUVPDelay, SCRelease).
#
# usage: hw_fields_hwtest.sh <jbdtool> <host>
#   jbdtool - build under test
#   host    - ip target (esplink), eg. pack_01
#
# Every field is written back with the value it already has, one at a time,
# with -w (exit 0x0000 - does not touch the 0xAA counters).  After each write
# both raw registers must be unchanged.  Then HCOVPDelay (0x39) and
# DSGOC2Delay (0x38) are each changed to another value and put back.  Then
# each indexed field is read with -r -n and written back with -w -n.  Then a
# value that is not in its table must be rejected without changing anything.
# On the first mismatch the original raw values are put back with
# scripts/jbd_raw_write.py (which does not go through the code under test)
# and the test stops.
#
# These are the pack's hardware short circuit / over current settings.  If
# the pack is carrying a load, make sure something else can carry it.

BIN=$1
HOST=$2
DIR=$(dirname $0)
RAW="python3 $DIR/../scripts/jbd_raw_write.py"
FIELDS="DoubleOCSC SCValue SCDelay DSGOC2 DSGOC2Delay HCOVPDelay HCUVPDelay SCRelease"

if [ -z "$BIN" -o -z "$HOST" ]; then
	echo "usage: $0 <jbdtool> <host>"
	exit 1
fi

rawreg() {
	$BIN -t ip:$HOST -R $1 | awk -F, '{ print $NF }'
}

R38=$(rawreg 0x38)
R39=$(rawreg 0x39)
if [ -z "$R38" -o -z "$R39" ]; then
	echo "error: cannot read 0x38/0x39 from $HOST"
	exit 1
fi
echo "baseline: 0x38=$R38 0x39=$R39"

restore() {
	echo "*** restoring 0x38=$R38 0x39=$R39"
	$RAW $HOST 38=$R38 39=$R39
	exit 1
}

check() {
	A38=$(rawreg 0x38)
	A39=$(rawreg 0x39)
	if [ "$A38" != "$R38" -o "$A39" != "$R39" ]; then
		echo "FAIL after $1: 0x38=$A38 0x39=$A39"
		restore
	fi
	echo "ok: $1 -> 0x38=$A38 0x39=$A39"
}

for f in $FIELDS; do
	v=$($BIN -t ip:$HOST -r $f | awk '{ print $2 }')
	if [ -z "$v" ]; then
		echo "error: cannot read $f"
		exit 1
	fi
	$BIN -t ip:$HOST -w $f $v || { echo "FAIL: -w $f $v returned an error"; restore; }
	check "$f=$v"
done

# all of them in one session
ARGS=""
for f in $FIELDS; do
	ARGS="$ARGS $f $($BIN -t ip:$HOST -r $f | awk '{ print $2 }')"
done
$BIN -t ip:$HOST -w $ARGS || { echo "FAIL: -w$ARGS returned an error"; restore; }
check "all fields in one session"

# A write of the same value can't tell a correct write from no write at all,
# so change one field in each register to another value from its list and
# put it back.  The field must read back as the new value, every other field
# must be unchanged, and after the restore both registers must match the
# baseline.
change() {
	f=$1
	alt=$2
	orig=$($BIN -t ip:$HOST -r $f | awk '{ print $2 }')
	[ "$orig" = "$alt" ] && alt=$3
	before=$($BIN -t ip:$HOST -r $FIELDS | grep -v "^$f ")
	$BIN -t ip:$HOST -w $f $alt || { echo "FAIL: -w $f $alt returned an error"; restore; }
	now=$($BIN -t ip:$HOST -r $f | awk '{ print $2 }')
	after=$($BIN -t ip:$HOST -r $FIELDS | grep -v "^$f ")
	if [ "$now" != "$alt" -o "$before" != "$after" ]; then
		echo "FAIL: $f=$alt read back as $now, or another field changed"
		restore
	fi
	echo "ok: $f $orig -> $alt: 0x38=$(rawreg 0x38) 0x39=$(rawreg 0x39)"
	$BIN -t ip:$HOST -w $f $orig || { echo "FAIL: -w $f $orig returned an error"; restore; }
	check "$f back to $orig"
}
# low risk: a hardware cell over voltage delay and a second level discharge
# over current delay, each changed for a few seconds
change HCOVPDelay 4 2
change DSGOC2Delay 160 80

# -n: -r -n prints the index, -w -n must take it back unchanged
for f in SCValue SCDelay DSGOC2 DSGOC2Delay HCOVPDelay HCUVPDelay; do
	v=$($BIN -n -t ip:$HOST -r $f | awk '{ print $2 }')
	$BIN -n -t ip:$HOST -w $f $v || { echo "FAIL: -n -w $f $v returned an error"; restore; }
	check "-n $f=$v"
done

# not in the table: must be refused, nothing written
if $BIN -t ip:$HOST -w SCValue 79; then
	echo "FAIL: SCValue 79 was accepted"
	restore
fi
check "SCValue=79 refused"
if $BIN -t ip:$HOST -w DoubleOCSC maybe; then
	echo "FAIL: DoubleOCSC maybe was accepted"
	restore
fi
check "DoubleOCSC=maybe refused"

echo "all passed"
