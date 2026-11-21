#!/usr/bin/env python3
"""Write raw 16-bit values to JBD BMS registers over an ip (esplink) target.

Recovery tool: jbdtool only writes registers through its parameter table,
so if a parameter write puts bad bits in a register, this puts the raw
value back without going through the code that broke it.

usage: jbd_raw_write.py [--save] <host[:port]> <reg>=<value> [<reg>=<value> ...]
  reg and value are hex, eg. 38=0d5c 39=f03c
  --save  exit factory mode with 0x2828 (save) instead of 0x0000.
          0x2828 zeroes the protection event counters (register 0xAA).

Sends a 0x03 read first (a sleeping board ignores the first frame), then
enter factory mode (0x5678 to reg 0x00), the writes, and the exit, reading
each register back afterwards.  Exits non-zero if any reply is an error or
a read back doesn't match.
"""

import socket
import sys

TIMEOUT = 3.0


def frame(action, reg, data=b""):
    body = bytes([reg, len(data)]) + data
    crc = (0x10000 - sum(body)) & 0xFFFF
    return bytes([0xDD, action]) + body + bytes([crc >> 8, crc & 0xFF, 0x77])


def reply(sock):
    buf = b""
    while True:
        chunk = sock.recv(256)
        if not chunk:
            raise IOError("connection closed")
        buf += chunk
        start = buf.find(b"\xdd")
        if start < 0 or len(buf) - start < 4:
            continue
        need = start + 4 + buf[start + 3] + 3
        if len(buf) >= need:
            pkt = buf[start:need]
            if pkt[-1] != 0x77:
                raise IOError("bad frame: " + pkt.hex(" "))
            return pkt


def xfer(sock, action, reg, data=b""):
    sock.sendall(frame(action, reg, data))
    pkt = reply(sock)
    # DD <reg> <status> <len> <data...> <crc> <crc> 77
    if pkt[1] != reg:
        raise IOError("reply for reg %02x, expected %02x: %s" % (pkt[1], reg, pkt.hex(" ")))
    return pkt[2], pkt[4:4 + pkt[3]]


def main():
    args = sys.argv[1:]
    save = False
    if args and args[0] == "--save":
        save = True
        args = args[1:]
    if len(args) < 2:
        print(__doc__.strip())
        return 1
    host, _, port = args[0].partition(":")
    port = int(port) if port else 23
    writes = []
    for a in args[1:]:
        reg, _, val = a.partition("=")
        writes.append((int(reg, 16), int(val, 16)))

    sock = socket.create_connection((host, port), timeout=TIMEOUT)
    sock.settimeout(TIMEOUT)
    ok = True
    try:
        try:
            xfer(sock, 0xA5, 0x03)
        except (IOError, socket.timeout):
            pass
        status, _ = xfer(sock, 0x5A, 0x00, b"\x56\x78")
        print("enter factory mode: status %02x" % status)
        if status != 0:
            return 1
        for reg, val in writes:
            status, _ = xfer(sock, 0x5A, reg, bytes([val >> 8, val & 0xFF]))
            print("write %02x = %04x: status %02x" % (reg, val, status))
            ok = ok and status == 0
        for reg, val in writes:
            status, data = xfer(sock, 0xA5, reg)
            back = (data[0] << 8 | data[1]) if len(data) >= 2 else -1
            print("read  %02x = %04x: status %02x %s" % (reg, back, status,
                  "ok" if back == val else "MISMATCH"))
            ok = ok and status == 0 and back == val
        exitval = b"\x28\x28" if save else b"\x00\x00"
        status, _ = xfer(sock, 0x5A, 0x01, exitval)
        print("exit factory mode (%s): status %02x" % (exitval.hex(), status))
        ok = ok and status == 0
    finally:
        sock.close()
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
