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

## src/attrlist.hxx

**2026-08-07** — blocked at GENERAL step 4. `ATTRLIST` owns a
heap-allocated `PATTR Table` array (`new ATTR[...]` in `Init()`/
`Resize()`, `delete [] Table` in the destructor/`Resize()`/`operator=`)
but declares no copy constructor — only `operator=` — so the
compiler-generated copy constructor does a shallow pointer copy of
`Table`. Exactly the same pattern already blocked at
`docs/AUTOPILOT_LOG.md#srcreclisthxx` (`RECLIST`), and extensively
forward-flagged across three earlier turns before reaching its own:
first noted as a latent risk during `operand.hxx`'s turn
(`docs/BUG_CATALOG.md#srcoperandhxx`, "Found but out of scope" —
`OPERAND::Attributes` is an `ATTRLIST`), then hit and worked around
during `irset.hxx`'s turn (`IRSET::IRSET(const IRSET&)` deliberately
base-constructs `OPERAND` rather than copy-constructing it, specifically
to avoid triggering this bug transitively — see the `BUGFIX #2` comment
at `src/irset.cxx:117`), then confirmed reachable in practice (not just
theoretical) during `sterm.hxx`'s turn, where self-assigning a live
`STERM` through its `OPOBJ&` interface silently wiped its `Attributes`.

Confirmed real again here with a standalone repro specific to the
copy-constructor path (as opposed to the self-assignment path already
confirmed at `sterm.hxx`'s turn): build an `ATTRLIST`, add one entry,
copy-initialize a second (`ATTRLIST b = a;` — copy constructor, not
`operator=`, since `b` doesn't exist yet), let `b` go out of scope,
then let `a` be destroyed at end of scope. AddressSanitizer reported a
`heap-use-after-free` in `ATTRLIST::~ATTRLIST()` (`attrlist.cxx:356`):
`b`'s implicit shallow copy shared `a`'s `Table` pointer, `b`'s
destructor freed it first, and `a`'s destructor then read/freed the
same already-freed block.

Fixing it requires adding `ATTRLIST(const ATTRLIST&);` to
`attrlist.hxx` — no declaration exists today — deep-copying `Table`,
`TotalEntries`, `MaxEntries` (mirroring `operator=`'s already-correct
logic), or alternatively `= delete`-ing it to make the class explicitly
non-copyable, the same deep-copy-vs-non-copyable choice already pending
on `reclist.hxx` and `vlist.hxx`. Unlike those two, non-copyable is a
harder sell here: `ATTRLIST` is a live member of `OPERAND`
(`src/operand.hxx:65`) and `DFD` (`src/dfd.hxx:72`), both of which are
copy-constructed/assigned in the tree today, so `= delete` would need
each of those call sites re-audited too — exactly the kind of ripple
GENERAL step 4 reserves for a human, not an autopilot guess.

Also found, not fixed here since step 4 gates the rest of this file's
pipeline for this turn: `ATTRLIST::operator=` (`attrlist.cxx:61`) has
no self-assignment guard (`delete [] Table; Init();` runs before
`OtherAttrlist.GetTotalEntries()` is read, so `x = x;` silently empties
the list) — already documented at
`docs/BUG_CATALOG.md#srcoperandhxx` and confirmed reachable at
`sterm.hxx`'s turn. This one doesn't need a header change (`operator=`'s
signature is unchanged, only its body) and can be fixed the next time
this file is reprocessed, alongside the copy-constructor decision.

Row set to `blocked`; needs a signature-change decision (deep-copy vs.
non-copyable, and if non-copyable, an audit of `OPERAND`'s and `DFD`'s
copy sites) before reprocessing via `/process src/attrlist.hxx`.
