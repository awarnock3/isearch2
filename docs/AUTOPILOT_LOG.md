# Isearch2 Cleanup — Autopilot Log

One running file, one `##` section per file GENERAL blocked at step 4
while running unattended (see AUTONOMY in CLAUDE.md).

## src/reclist.hxx

**2026-08-07** — blocked at GENERAL step 4. `RECLIST` owns a
heap-allocated `PRECORD Table` array (`new RECORD[...]` in the
constructor/`Resize`, `delete [] Table` in the destructor/`Resize`) but
declares no copy constructor or copy-assignment operator, so the
compiler-generated ones do a shallow pointer copy. Confirmed real with
a standalone repro: copy-construct a second `RECLIST` from an existing
one, let the copy go out of scope, then destroy the original — ASan
reported a `heap-use-after-free` in `RECLIST::~RECLIST()` (the second
`delete []` on the already-freed `Table`). `RECLIST` is currently
dormant (its only two references in the live tree, `src/Iindex.cxx`
and `src/idb.hxx`, are both commented out), so this isn't an active
crash today, but it's a real defect in the class as declared.

Fixing it requires adding two declarations to `reclist.hxx` that don't
exist today — `RECLIST(const RECLIST&);` and
`RECLIST& operator=(const RECLIST&);` (deep-copying `Table`,
`TotalEntries`, `MaxEntries`), or alternatively `= delete`-ing both to
make the class explicitly non-copyable if deep-copy semantics aren't
wanted. Either is a public signature change to a class with two
(commented-out, so currently invisible to the compiler, but real once
uncommented) external references — exactly the kind of call GENERAL
step 4 reserves for a human. Row set to `blocked`; needs a
signature-change decision (deep-copy vs. non-copyable) before
reprocessing via `/process src/reclist.hxx`.

## src/vlist.hxx

**2026-08-07** — blocked at GENERAL step 4. `VLIST` (doubly linked
circular list base class) declares a virtual destructor but no copy
constructor and no `operator=` — only a commented-out
`virtual VLIST& operator=(const VLIST&) = 0;` — so the compiler
generates both, shallow-copying the raw `Next`/`Prev` pointers instead
of splicing the copy into (or out of) the circle. This exact risk was
flagged in advance during the `dft.hxx` turn (see
`docs/BUG_CATALOG.md#srcdfthxx`, "Found but out of scope" section:
"`vlist.hxx` (Order 27, `FCT`'s base class)... has the identical
pattern one level further down").

Confirmed real with a standalone repro (`VLIST* a = new VLIST();
VLIST* b = new VLIST(*a); delete b; delete a;` — copy-construct one
node from another default-constructed "circle of one" node, then
delete both): `b`'s implicit copy ctor makes `b->Next == b->Prev ==
a`, so `delete b` runs `~VLIST()`, which nulls `a->Next` and then
recursively `delete`s `a` (its `Next`/`Prev` both alias `a`, which is
not code that owns `a`'s allocation). AddressSanitizer reported a
heap-use-after-free with the free happening inside the very
`VLIST::~VLIST()` cascade the copy triggered, then the outer `delete
a;` would be a second free of the same block. Unlike `reclist.hxx`,
`VLIST` isn't dormant: `FCT` (Order 2, already processed) and
`STRLIST` (Order 29, still pending) both derive from it (`class FCT :
public VLIST`, `src/strlist.hxx:56`), and neither declares its own
copy constructor either, so a copy of either subclass would hit this
transitively today.

Fixing it requires adding `VLIST(const VLIST&);` (and/or
`VLIST& operator=(const VLIST&);`) to `vlist.hxx` — no declaration
that exists today — or explicitly `= delete`-ing both to make the
class non-copyable, mirroring the deep-copy-vs-non-copyable choice
already pending on `reclist.hxx`. Note a deep-copy fix here is
semantically odd for a *circular* list: copying a `Next`/`Prev` pair
only makes sense relative to the specific circle the node lives in, so
"deep copy" likely means "splice as a new one-node circle" rather than
literally duplicating the chain — a design call, not just a mechanical
fix, which is itself an argument for a human decision here. Row set to
`blocked`; needs a signature-change decision before reprocessing via
`/process src/vlist.hxx`.
