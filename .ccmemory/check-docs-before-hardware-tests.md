---
name: check-docs-before-hardware-tests
description: For BMS protocol questions, find the documented answer before proposing writes to a live pack; pasted pack output is not a go-ahead to test
metadata:
  type: feedback
---

When a protocol behavior was in question (does 0x2828 clear the 0xAA counters?), I treated the user's pasted `jbdtool -t ip:pack_01` output as permission to test and proposed writing to a live pack. The user interrupted: "Wait, why are we doing this again? What is the documented way…"

**Why:** the packs are live production batteries; writes can destroy data (error counters) or change protection settings, and the documentation answered the question anyway.

**How to apply:** for protocol questions, look up the manufacturer manual / register map first (see [[jbd-protocol-manual-and-eeprom-commit]]). Propose hardware writes only when the docs can't settle it, and say what data would be lost. A paste of read-only output is information, not a request to run a test.
