# main.c - CLI, parameter table, display & write

## What it does

`main.c` is the command line front end.  It loads a transport module
(serial/bt/ip/can/can_ip) and the `jbd` cellmon module, locks the target, then
performs one of four actions: INFO (default), READ (`-r`, `-R <reg>`), WRITE
(`-w`, `-W`), LIST (`-l`).

Parameters are described by one table:

```c
struct jbd_params {
	uint8_t reg;	/* eeprom register */
	char *label;	/* name used on the command line and in output */
	int dt;		/* JBD_PARM_DT_* - how to interpret the register */
} params[];
```

Several labels can share one register (`JBD_REG_HCOC` 0x38 is DoubleOCSC /
SCValue / SCDelay / DSGOC2 / DSGOC2Delay; `JBD_REG_HTRT` 0x39 is HCOVPDelay /
HCUVPDelay / SCRelease) - the `dt` selects which part of the register a label
refers to.  Adding a parameter means adding a row here; display (`pdisp()`)
and write (`write_parm()`) both switch on `dt`, and `write_parm()` has a
`default:` that refuses a `dt` it has no case for - a new type must get a
write case or it can't be written.

Values are raw register units in both directions (10 mV pack voltages, 10 mA
currents, 0.1 K temperatures, ...).  There is no unit conversion and no range
check, so `-r` output can be fed back to `-w` but a hand-written value in the
wrong units goes to the BMS as-is.

## Data types (JBD_PARM_DT_*)

Most types are a plain 16 bit register value with a scale/format applied on
display.  The ones with structure:

| dt | meaning |
|----|---------|
| `B0` / `B1` | one byte of a two byte register - write does read-modify-write |
| `FUNC` | BatteryConfig bit field (`func_bits[]`) |
| `NTC` | NtcConfig bit field (`ntc_bits[]`) |
| `DOUBLE` | bit 7 of `JBD_REG_HCOC` - write takes true/false/1/0, read-modify-write |
| `SCVAL`, `SCDELAY`, `DSGOC2`, `DSGOC2DELAY` | indexed subfields of `JBD_REG_HCOC` (`jbd_fields[]`) |
| `HCOVPDELAY`, `HCUVPDELAY` | indexed subfields of `JBD_REG_HTRT` (`jbd_fields[]`) |
| `DUMP` | raw (`-R`): one word as signed, unsigned and hex; a longer reply as every word in hex |

`-n` (`dont_interpret`) makes the interpreted types print as plain numbers.

## Indexed fields (0x38 / 0x39)

The hardware protection subfields hold an index into a fixed list of values
(SCValue index 1 = 33 mV).  `jbd_fields[]` is the single description of each:
which byte, mask, shift, and the value list.  `pdisp()` decodes with it and
`write_parm()` encodes with it, so the two can't drift apart.

A write reads the register, replaces only that field's bits and writes it
back - five labels share 0x38, so writing anything other than the whole
register has to preserve the rest.  The value must be an exact entry in the
list; anything else returns -1 and nothing is sent, because a near miss would
mean picking an index the user didn't ask for.

With `-n` (`dont_interpret`) both directions use the index: `pdisp()` prints
it and `write_parm()` takes it, range checked against the list length.  It
has to be symmetric - for HCOVPDelay/HCUVPDelay some indexes are also valid
values (HCOVPDelay index 2 = 4 s, value 2 = index 1), so a write that always
took values would turn `-r -n` output into the wrong setting.

## Bit fields

`BatteryConfig` (`JBD_REG_FUNCMASK`) and `NtcConfig` (`JBD_REG_NTCMASK`) are bit
masks.  Both the names and the masks live in one table each - `func_bits[]` and
`ntc_bits[]` - which `pdisp()` uses to build the output string and
`parse_bits()` uses to parse input.  Keep them as the single source; the old
code had the names spelled out twice as `if` chains and only on the read side,
which is how the write side ended up not understanding them at all.

`parse_bits()` accepts either a number (`strtol` base 0, so `0x0E` works) or a
list of names seperated by `,` `|` `+` or whitespace, case insensitive.  An
unrecognized name returns -1 and `write_parm()` returns without writing - the
BMS never sees a partial or zero mask because of a typo.

A write replaces the entire field.  There is no read-modify-write and no
per-bit set/clear syntax: `-w BatteryConfig SCRL,BALANCE_EN` clears every bit
not named.

## JSON round trip

`-j`/`-J` writes bit fields as arrays of names (`json_object_dotset_value` with
a `[ ... ]` string).  `-w -f file.json` reads `JSONArray` back and joins the
elements with commas before handing them to `write_parm()`, so a settings file
dumped with `-j` can be written back as-is.

An unhandled JSON value type now `continue`s.  It used to `break` out of the
type switch and fall into `write_parm()` with `p` still pointing at the
parameter *name*, writing a garbage value.

## Write sessions

Writing needs factory mode: `jbd_eeprom_start()` writes 0x5678 to register
0x00.  Every WRITE path (argument pairs, text file, JSON file) leaves the
session open, and one exit at the end of the WRITE case closes it:

- `write_error` set (unknown label, bad value, failed write) - plain exit
  (`jbd_eeprom_end()`, 0x0000), message, return 1.  Writes already sent are
  not undone; on boards that keep 0x0000 writes they stay.
- `-w` - `jbd_eeprom_end()`, 0x0000.
- `-W` - `jbd_eeprom_save()`, 0x2828.
- a failed exit is reported and returns 1.

Why two exits: 0x2828 ("exit and save") is the only exit JBD's protocol
manual documents, and it is what makes writes survive a power cycle on newer
firmware (reported on 3.5).  It also zeroes the protection event counters in
0xAA - verified on hardware.  On 2.5 firmware 0x0000 already persists writes,
so making 0x2828 the default would wipe every user's counters on every write
for no gain there.  `-W` is opt-in and its help text says it zeroes them.

## Info and protection counts

`jbd_get_info()` reads 0x03 (status), 0x04 (cells), 0x05 (model) and 0xAA
(protection event counts).  0xAA does not need factory mode, so INFO and
MQTT polling never enter it.  A board that doesn't answer 0xAA leaves
`nprotcounts` at 0 and the counts are simply not displayed; older firmware
answers with 11 words (no restart count).  `jbd_protcount_names[]` gives
the word order and the `-F` field names.

## Output formats

`outfmt`: 0 = aligned text, 1 = comma delimited (`-c`), 2 = JSON (`-j`/`-J`).
All output goes through `outfp` (stdout, or the `-o` file).  `main()`
`fclose()`s `outfp` - fd 1 - before returning, so anything written with
plain `printf()` sits in stdout's buffer and is lost when stdout is not a
tty.  Error messages use `printf()`; data must not.
`_addstr()` appends to a list using `sepstr`, and quotes each element when
`outfmt == 2` so the result parses as a JSON array.  `-F` (`flat`) flattens
the info arrays into one key per element: `temp_00`, `cell_00`, ... and the
protection counts by name (`ShortCircuitCount`, ...).

`_addstr()` takes the destination size and drops an element rather than
overflowing.  It used to be an unbounded `strcat()` into `pdisp()`'s `str[64]`:
all 8 BatteryConfig names, quoted and comma seperated for JSON, are 73 bytes -
so `-j` on a pack with most function bits set smashed the stack.  `str`/`temp`
in `pdisp()` are now sized for the whole table.

## Locking

Non-Windows builds lock `/tmp/<target>.lock` for the duration of the run so
concurrent invocations (a cron reader and a manual write) cannot interleave on
the same bus.  `-N` fails instead of waiting.

## Tests

Offline (`cd tests && make test`, no BMS):

- `test_parse_bits.c` includes `main.c` with `main` renamed and exercises
  `parse_bits()` against the real tables, plus a `pdisp()` -> `parse_bits()`
  round trip over all 256 masks (that round trip is what found the `str[64]`
  overflow).
- `test_hw_fields.c` does the same with `jbd_rw` renamed to a fake register
  file, and checks every indexed field: factory values encode to the factory
  0x9950/0xB005, every list value reads back as itself without touching other
  bits, every `-n` index reads back as itself, bad values and unknown types
  are refused without a write.

Hardware (write to a real BMS - see each script's header):

- `hw_fields_hwtest.sh` - writes the 0x38/0x39 fields back with their current
  values, changes one field in each register and restores it, writes `-r -n`
  output back with `-w -n`, checks rejection.  Restores the raw registers with `scripts/jbd_raw_write.py` on
  any mismatch.
- `eeprom_commit_test.sh` - `-w` vs `-W` across manual power cycles.  Uses
  `-W`, so it zeroes 0xAA.

`scripts/jbd_raw_write.py` writes raw register values over an ip target
without going through the parameter code - the recovery path when a
parameter write puts bad bits in a register.

## Gotchas

- EEPROM access must be bracketed by `jbd_eeprom_start()` and an exit
  (`jbd_eeprom_end()`, or `jbd_eeprom_save()` for `-W`).  Every read/write
  path in `main()` does this; a new path must too.  Reads never use the save
  exit - it zeroes 0xAA.
- `-X` sends 0x4321 to 0xE3 and 0x1881 to 0x0A; 2.5 firmware answers both
  with 0x80 and does not reset.  0x1881 isn't in JBD's manual (its reset is
  command 0x0A with 0x0300).  Nothing can rely on `-X` to reload EEPROM.
- A sleeping board was reported ignoring the first frame (UART), so the
  factory mode enter got no reply.  jbdtool doesn't send a wake frame first.
- CAN cannot read or write parameters - both actions bail out early.
- `-r -a` walks the whole params table, so a register that returns a short read
  stops the loop (`if (bytes < 0) break;`).
