
version		date		what
1.8		2022-04-11	lock target
1.9.1		2026-08-05	fix: writing BatteryConfig/NtcConfig by bit name
				(eg. SCRL,BALANCE_EN) silently wrote 0 - the value
				went through atoi().  Bit names are now parsed, and
				an unrecognized name is an error instead of a write.
				JSON files written by -j can now be fed back to -w
				(bit fields are arrays); a bad json value type no
				longer writes the parameter name as the value.
				fix: pdisp() str[] was too small to hold all 8
				BatteryConfig names quoted for json - reading a pack
				with most function bits set smashed the stack.
				_addstr() now takes the buffer size.
				serial: guard baud rates missing on macOS.
				tests/: added test_parse_bits.
1.9.2		2026-11-19	EEPROM commit on write (PR #21)
				-W: write parameters and exit factory mode with
				0x2828 (save) instead of 0x0000, for boards that
				lose -w writes at power off.  0x2828 also zeroes
				the protection event counters (0xAA) - verified on
				a fw 2.5 pack - which is why -w keeps 0x0000: on
				fw 2.5 it already persists writes.
				An error partway through a write (bad parm name,
				missing value, failed write) now returns an error
				and never uses the -W save.  A failed session
				close is reported instead of exiting 0.
				JSON-file writes no longer close the session twice
				(the save/close sat outside the else).
				tests/eeprom_commit_test.sh: hardware test for the
				two exits.
1.9.3		2026-11-19	-R raw register output
				-R printed nothing when stdout was not a tty (pipe,
				script, cron) and ignored -o: the raw display used
				printf while everything else uses outfp, and outfp
				(fd 1) is fclose()d before stdout is flushed.
				-R now shows every word of a multi-word reply in
				hex (0xAA showed only the first word).
1.9.4		2026-11-19	protection counts in info output (issue #6)
				Info output (and so -j/-J and MQTT) now includes
				ProtectionCounts: the 12 protection event counters
				from register 0xAA, as an array like Cells/Temps.
				-F flattens them to named fields (ShortCircuitCount,
				CellUVCount, RestartCount, ...).  0xAA is read
				without entering factory mode - one more request
				per poll.  Older firmware sends 11 (no restart
				count); a board that doesnt answer just omits them.
1.9.5		2026-11-21	hardware protection param writes (issue #22)
				fix: write_parm() had no case for DoubleOCSC,
				SCValue, SCDelay, DSGOC2, DSGOC2Delay, HCOVPDelay or
				HCUVPDelay and sent 2 bytes of uninitialized stack
				to register 0x38/0x39 - the hardware short circuit
				and overcurrent settings.  Boards that persist -w
				writes (fw 2.5) or a -W write kept the garbage; a
				-j dump fed back to -w hit it.
				They are now read-modify-write of only their own
				bits, using the same index tables as the display.
				A value not in the table is an error and nothing is
				written; DoubleOCSC takes true/false/1/0.  A param
				type write_parm() doesnt handle is refused instead
				of written.
				tests/test_hw_fields: offline test with a fake
				register file.
1.9.6		2026-11-21	-n writes for the indexed protection fields
				With -n, -w takes the raw index for SCValue, SCDelay,
				DSGOC2, DSGOC2Delay, HCOVPDelay and HCUVPDelay - the
				same thing -r -n prints - so -n output writes back
				unchanged.  Before, -w always took the real value:
				-n output was rejected, or for HCOVPDelay/HCUVPDelay
				some indexes are also valid values and were written
				as the wrong setting (index 2 = 4s, "2" set 2s).
