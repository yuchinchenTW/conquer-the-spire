"""A poor man's backtrace out of a Windows minidump, with no debugger.

    python Scripts/dumpwalk.py %LOCALAPPDATA%/CrashDumps/python.exe.NNNN.dmp

Then name the offsets it prints against a build of the same sources with -g
added (nothing else changed, so the layout matches):

    addr2line -f -C -i -e build-sym/bin/libconquer-the-spire.dll 0x<base+rva>

This is how the heap corruption of September 2026 was found: two dumps put
the same frames of our dll on the stack, a debug-container build named the
class of fault, and All for One into a full hand was the site.

The dump's module list says where each dll sat; the exception stream says
which thread fell over and where its stack pointer was; the memory list
holds the stack itself. Every eight-byte value on that stack that falls
inside a module is a candidate return address. Not every one is (some are
data), but the ones inside our own dll say which of our functions were on
the stack when the heap check fired - which is what a debugger would say,
minus the certainty.
"""

import struct
import sys

MDMP = b"MDMP"
THREAD_LIST, MODULE_LIST, EXCEPTION, MEMORY64_LIST = 3, 4, 6, 9


def u32(b, at):
    return struct.unpack_from("<I", b, at)[0]


def u64(b, at):
    return struct.unpack_from("<Q", b, at)[0]


def rva_string(b, rva):
    n = u32(b, rva)
    return b[rva + 4:rva + 4 + n].decode("utf-16-le", "replace")


def main(path):
    with open(path, "rb") as f:
        b = f.read()

    assert b[:4] == MDMP, "not a minidump"
    count, dir_rva = u32(b, 8), u32(b, 12)
    streams = {}

    for i in range(count):
        at = dir_rva + i * 12
        kind, size, rva = u32(b, at), u32(b, at + 4), u32(b, at + 8)
        streams[kind] = (rva, size)

    # modules: MINIDUMP_MODULE is 108 bytes; the name's rva sits at +20
    mods = []
    rva, _ = streams[MODULE_LIST]
    n = u32(b, rva)

    for i in range(n):
        at = rva + 4 + i * 108
        # base(8) size(4) checksum(4) timestamp(4) name rva(4)
        base, size, name_rva = u64(b, at), u32(b, at + 8), u32(b, at + 20)
        mods.append((base, size, rva_string(b, name_rva)))

    def whose(addr):
        for base, size, name in mods:
            if base <= addr < base + size:
                return name.rsplit("\\", 1)[-1], addr - base

        return None, None

    # exception: thread id (4) + align (4) + record (152) + context location
    rva, _ = streams[EXCEPTION]
    tid = u32(b, rva)
    code = u32(b, rva + 8)
    exc_addr = u64(b, rva + 8 + 16)
    ctx_size, ctx_rva = u32(b, rva + 8 + 152), u32(b, rva + 8 + 156)
    # CONTEXT (amd64): Rsp at offset 0x98, Rip at 0xF8
    rsp = u64(b, ctx_rva + 0x98)
    rip = u64(b, ctx_rva + 0xF8)

    print("exception 0x%08x in thread %d" % (code, tid))
    print("  rip %016x  -> %s+0x%x" % ((rip,) + whose(rip)))
    print("  rsp %016x" % rsp)

    # find the faulting thread's stack range
    rva, _ = streams[THREAD_LIST]
    n = u32(b, rva)
    stack = None

    for i in range(n):
        at = rva + 4 + i * 48
        # id(4) suspend(4) prio class(4) prio(4) teb(8) stack{start(8)
        # size(4) rva(4)} context{size(4) rva(4)}
        if u32(b, at) == tid:
            start = u64(b, at + 24)
            size, mem_rva = u32(b, at + 32), u32(b, at + 36)
            stack = (start, size, mem_rva)

    if stack is None or stack[2] == 0:
        # full dump: stack lives in the Memory64 list; find the range
        rva, _ = streams[MEMORY64_LIST]
        n, base_rva = u64(b, rva), u64(b, rva + 8)
        off = base_rva

        for i in range(n):
            at = rva + 16 + i * 16
            start, size = u64(b, at), u64(b, at + 8)

            if start <= rsp < start + size:
                # From the stack pointer up to the top of the range.
                stack = (rsp, (start + size) - rsp, off + (rsp - start))
                break

            off += size

    start, size, mem_rva = stack
    # The thread-list path hands back the whole stack range; the fallback
    # above hands back the part from rsp up. Read from rsp either way.
    mem_rva += rsp - start
    size -= rsp - start
    start = rsp

    print("  stack %016x .. %016x (%d bytes read from rsp up)"
          % (rsp, start + size, size))

    seen = []
    depth = min(size, 64 * 1024)

    for k in range(0, depth - 8, 8):
        v = u64(b, mem_rva + k)
        name, off = whose(v)

        system = ("ntdll", "kernel", "ucrt", "msvcp", "vcruntime")

        if name and not name.lower().startswith(system):
            seen.append((k, v, name, off))

    print()
    print("candidate return addresses on the stack (nearest the fault first):")
    ours = 0

    for k, v, name, off in seen[:60]:
        mark = "  <==" if "conquer" in name else ""
        ours += 1 if mark else 0
        print("  rsp+%05x  %-28s +0x%06x%s" % (k, name, off, mark))

    print()
    print("%d frames in our dll among the first 60 candidates" % ours)


if __name__ == "__main__":
    main(sys.argv[1])
