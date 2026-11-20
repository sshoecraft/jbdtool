---
name: jbd-protocol-manual-and-eeprom-commit
description: JBD protocol manual V12 URL; 0x2828 zeroes 0xAA (hw-verified); on the user's fw 2.5 packs the 0x0000 close already persists writes
metadata:
  type: reference
---

JBD manufacturer manual "Software board universal protocol V12":
https://cdn-files.myshopline.com/file/store/1720492677212/JBD-Smart-BMS-RS485-RS232-UART-Protocl-New-pdf-WiFi.pdf
(Read tool renders it with `pages`; WebFetch can't extract its text.) Not in /data/docs; that folder has no JBD/BMS docs.

Facts from it that bear on jbdtool:
- Write procedure (sec 5): enter factory mode = write 0x5678 to reg 0x00; write params; exit = write 0x2828 to reg 0x01. That exit is the only one documented, and there is no separate save command.
- 0x0000 to reg 0x01 (jbdtool's `jbd_eeprom_end`) is NOT in JBD's manual; only the community bms-tools register map lists it ("exit Factory Mode").
- 0xAA layout (sec 3.4): 12 big-endian words when length is 24: short circuit, chg OC, dsg OC, cell OV, cell UV, chg OT, chg UT, dsg OT, dsg UT, pack OV, pack UV, system restarts.
- Control cmd 0x0A takes 0xAABB codes: 0x0100 reset capacity, 0x0200 clear history, 0x0300 reset MCU, 0x0500 sleep, 0x0600 ultra-low-power (needs a CHARGE to wake — never send by mistake).

Verified on hardware 2026-11-19 (pack_01, SP15S008C-P14S-100A, fw 2.5, `tests/eeprom_commit_test.sh`):
- Closing a write session with 0x2828 zeroes ALL of 0xAA, restart count included (bms-tools map is right; JBD manual doesn't mention it).
- The 0x0000 close DOES persist on this firmware: the user's DesignCapacity=7500 set with old jbdtool `-w` survived the pack's 137 recorded restarts. PR #21's "writes are ephemeral" claim does not hold here; it may hold on other boards/firmware.
- `jbdtool -X` does not reset this firmware: reg 0xE3=0x4321 and reg 0x0A=0x1881 both get status 0x80 and the restart counter doesn't move. Any test that relies on -X to reload EEPROM is invalid.
- `-R <reg>` prints nothing unless `-d` is given.
- The packs power the house in parallel; never reset or reboot one without asking.

Related: [[check-docs-before-hardware-tests]]
