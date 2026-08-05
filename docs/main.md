# main.c - CLI, parameter table, display & write

## What it does

`main.c` is the command line front end.  It loads a transport module
(serial/bt/ip/can/can_ip) and the `jbd` cellmon module, locks the target, then
performs one of four actions: INFO (default), READ (`-r`), WRITE (`-w`),
LIST (`-l`).

Parameters are described by one table:

```c
struct jbd_params {
	uint8_t reg;	/* eeprom register */
	char *label;	/* name used on the command line and in output */
	int dt;		/* JBD_PARM_DT_* - how to interpret the register */
} params[];
```

Several labels can share one register (`JBD_REG_HCOC` is DoubleOCSC / SCValue /
SCDelay / DSGOC2 / DSGOC2Delay) - the `dt` selects which part of the register a
label refers to.  Adding a parameter means adding a row here; display
(`pdisp()`) and write (`write_parm()`) both switch on `dt`.

## Data types (JBD_PARM_DT_*)

Most types are a plain 16 bit register value with a scale/format applied on
display.  The ones with structure:

| dt | meaning |
|----|---------|
| `B0` / `B1` | one byte of a two byte register - write does read-modify-write |
| `FUNC` | BatteryConfig bit field (`func_bits[]`) |
| `NTC` | NtcConfig bit field (`ntc_bits[]`) |
| `SCVAL`, `SCDELAY`, `DSGOC2`, `DSGOC2DELAY` | packed subfields of `JBD_REG_HCOC` |
| `HCOVPDELAY`, `HCUVPDELAY` | packed subfields of `JBD_REG_HTRT` |
| `DUMP` | raw: signed, unsigned and hex |

`-n` (`dont_interpret`) makes the interpreted types print as plain numbers.

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

### History

Before 1.9.1 `write_parm()` handled `DT_FUNC` and `DT_NTC` in the same case as
the plain integer types - `_putshort(data,atoi(value))`.  `atoi("SCRL,...")` is
0, so writing the same symbolic form that `-r` printed silently wrote 0 and
cleared the field.  Reported from the field: a pack whose BatteryConfig was
`SCRL,BALANCE_EN,CHG_BALANCE` (0x0E) was set to `SCRL,BALANCE_EN`, and read back
blank (0x00) - balancing disabled entirely, with no error and no other symptom.
Recovery is a numeric write (`-w BatteryConfig 14`), which always worked.

## JSON round trip

`-j`/`-J` writes bit fields as arrays of names (`json_object_dotset_value` with
a `[ ... ]` string).  `-w -f file.json` reads `JSONArray` back and joins the
elements with commas before handing them to `write_parm()`, so a settings file
dumped with `-j` can be written back as-is.

An unhandled JSON value type now `continue`s.  It used to `break` out of the
type switch and fall into `write_parm()` with `p` still pointing at the
parameter *name*, writing a garbage value.

## Output formats

`outfmt`: 0 = aligned text, 1 = comma delimited (`-c`), 2 = JSON (`-j`/`-J`).
`_addstr()` appends to a list using `sepstr`, and quotes each element when
`outfmt == 2` so the result parses as a JSON array.  `-F` (`flat`) flattens
JSON arrays of cell values into `cell_1`, `cell_2`, ... keys.

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

`tests/test_parse_bits.c` includes `main.c` with `main` renamed and exercises
`parse_bits()` against the real tables, plus a `pdisp()` -> `parse_bits()` round
trip over all 256 masks (that round trip is what found the `str[64]` overflow).
`cd tests && make test`.

## Gotchas

- EEPROM access must be bracketed by `jbd_eeprom_start()` / `jbd_eeprom_end()`.
  Every read/write path in `main()` does this; a new path must too.
- CAN cannot read or write parameters - both actions bail out early.
- `-r -a` walks the whole params table, so a register that returns a short read
  stops the loop (`if (bytes < 0) break;`).
