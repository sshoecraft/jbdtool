---
name: src-nfs-mount-corrupted-writes
description: /src is NFS from 192.168.1.4; jumbo frames silently corrupted writes there until en0 MTU was set to 1500 on 2026-08-05
metadata:
  type: project
tags: [nfs, build, infrastructure, toolchain]
---

`/System/Volumes/Data/src` (symlinked as `/src`) is NFSv3 over TCP from
192.168.1.4, mounted `soft` with rsize/wsize=1048576.

On 2026-08-05 en0 was set to MTU 9000 but jumbo frames did not work end to end
(`ping -D -s 8972 192.168.1.4` = 100% loss, `-s 1472` = 0% loss). Writes to the
mount were silently corrupted: three identical compiles of the same .c produced
three different object files, and an 8MB file copied in came back with a
different md5 every time. Re-reads of a corrupted file were stable, so the bad
data was persisted on write, not a read-cache flake.

The symptoms did not look like storage corruption. They looked like toolchain or
macOS problems: `ld` reporting "unknown file type" and
"ARM64_RELOC_BRANCH26 relocation on non-b/bl instruction", freshly built
binaries getting SIGKILLed, and one binary loading with its entire `__text`
section zeroed. All of it was the mount.

**Why:** anything built or edited under /src can be silently damaged when the
network path to 192.168.1.4 is misconfigured, and it surfaces as bogus
compiler/linker/loader errors that send you chasing the wrong bug.

**How to apply:** if a build under /src fails in a way that makes no sense,
verify the mount before debugging the code - copy a large file in and compare
md5, or compile the same source twice and compare hashes. Setting en0 to MTU
1500 fixed it; a 200MB copy, repeated compiles and a full rebuild verified clean
afterward. The `soft` mount option is still in place and turns transport trouble
into bad data rather than a hang - `hard,intr` would be safer.
