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

## src/dfdt.hxx

**2026-08-07** — blocked at GENERAL step 4. `DFDT` owns a
heap-allocated `PDFD Table` array (`new DFD[...]` in `Initialize()`/
`Resize()`, `delete [] Table` in the destructor/`Resize()`/`operator=`)
but declares no copy constructor — only `operator=` — so the
compiler-generated copy constructor does a shallow pointer copy of
`Table`. The third instance of the exact pattern already blocked at
`docs/AUTOPILOT_LOG.md#srcreclisthxx` (`RECLIST`) and
`docs/AUTOPILOT_LOG.md#srcattrlisthxx` (`ATTRLIST`) this same batch —
unlike `DF`'s/`DFD`'s *inherited* versions of this risk (via their
`FCT`/`ATTRLIST` members, documented and deferred without blocking
those files), `DFDT` owns the raw array directly, so the fix belongs in
`dfdt.hxx` itself.

Confirmed real with a standalone repro, same shape as `ATTRLIST`'s:
build a `DFDT`, add one entry, copy-initialize a second (`DFDT b = a;`
— copy constructor, not `operator=`, since `b` doesn't exist yet), let
`b` go out of scope, then let `a` be destroyed at end of scope.
AddressSanitizer reported a `heap-use-after-free` in `DFDT::~DFDT()`
(`dfdt.cxx:371`): `b`'s implicit shallow copy shared `a`'s `Table`
pointer, `b`'s destructor freed it first, and `a`'s destructor then
read/freed the same already-freed block. No confirmed copy-construction
call site was found in the live tree (every site found uses `DFDT*`,
default-construction, or `*DfdtBuffer = *MainDfdt;` — assignment, not
construction), so this is latent rather than actively crashing today,
the same status `RECLIST` and `ATTRLIST` had when they were blocked.

Fixing it requires adding `DFDT(const DFDT&);` to `dfdt.hxx` — no
declaration exists today — deep-copying `Table`, `TotalEntries`,
`MaxEntries`, `Changed` (mirroring `operator=`'s already-correct logic),
or alternatively `= delete`-ing it to make the class explicitly
non-copyable, the same deep-copy-vs-non-copyable choice already pending
on `reclist.hxx`, `vlist.hxx`, and `attrlist.hxx`.

Also found, not fixed here since step 4 gates the rest of this file's
pipeline for this turn: `DFDT::operator=` (`dfdt.cxx:61`) has no
self-assignment guard — the identical `delete [] Table; Initialize();`
-before-reading-`OtherDfdt.GetTotalEntries()` shape as `ATTRLIST`'s and
`STRLIST`'s already-fixed/blocked instances of the same bug (see
`docs/BUG_CATALOG.md#srcstrlistcxx`, `BUGFIX #1`, and
`docs/AUTOPILOT_LOG.md#srcattrlisthxx`). Doesn't need a header change
and can be fixed the next time this file is reprocessed, alongside the
copy-constructor decision. Two more pre-existing findings, not fixed
for the same reason: `DFDT::LoadTable` casts `NULL` to `(CHR*)NULL` for
`strtok`'s second-and-later calls rather than using `nullptr` (ordinary
modernization, not a bug); `DFDT::GetDfdRecord`
(`dfdt.cxx:295`) assigns `DfdRecord=(PDFD)NULL;` to the local
out-parameter *pointer itself* on a not-found lookup rather than to
`*DfdRecord`, which has no effect the caller can observe (the pointer
argument is passed by value) and leaves `*DfdRecord` holding whatever
the caller passed in — likely meant to signal "not found" but silently
doesn't; worth a closer look alongside the copy-constructor fix.

Row set to `blocked`; needs a signature-change decision (deep-copy vs.
non-copyable) before reprocessing via `/process src/dfdt.hxx`.

## src/mdt.hxx

**2026-08-07** — blocked at GENERAL step 4. `MDT` owns two
heap-allocated arrays (`KEYREC* KeyIndex`, `GPREC* GpIndex`, both
`new`'d in the constructor/`Resize()`, `delete []`'d in the destructor/
`Resize()`) *and* a raw `FILE* MdtFp` (opened in the constructor,
`fclose()`'d in the destructor) — but, unlike every other class blocked
this batch, declares **no** copy constructor and **no** `operator=` at
all, not even a (buggy) hand-written one. `MDT` has a user-declared
destructor but no user-declared copy operations or move operations, so
under C++11 rules the compiler still implicitly generates both a copy
constructor and a copy-assignment operator (merely deprecated, not
suppressed) — both doing a member-wise shallow copy of `KeyIndex`,
`GpIndex`, *and* `MdtFp` together.

Confirmed real with a standalone repro, same shape as `ATTRLIST`'s/
`DFDT`'s: construct an `MDT` against a real temp file stem (mirroring
`tests/src/test_filemap.cxx`'s `TempMdt` fixture), add one entry,
copy-initialize a second (`MDT b = *a;` — copy constructor), let `b` go
out of scope, then destroy `a`. AddressSanitizer reported a
`heap-use-after-free` — not even in `MDT::~MDT()` itself this time, but
one level further in: `a`'s destructor calls `FlushMDTIndexes()` →
`SortGpIndex()` → `qsort()` on `GpIndex`, which `b`'s destructor had
already `delete []`'d. The `MdtFp` sharing is real too by the same
mechanism (both copies' destructors call `fclose()` on the same
`FILE*`) but wasn't reached in this repro — the array free hit first.
No confirmed copy-construction call site was found in the live tree
(every site found uses `MDT*`/`new MDT(...)`, never a bare `MDT` value
or an assignment between two `MDT`s), so this is latent rather than
actively crashing today, the same status the other three raw-resource
classes had when they were blocked.

Fixing it requires adding `MDT(const MDT&);` and
`MDT& operator=(const MDT&);` to `mdt.hxx` — deep-copying `KeyIndex`/
`GpIndex` and, for the `FILE*`, either re-opening `MdtFp` against the
same `FileStem` or deciding copies shouldn't share live file state at
all — or `= delete`-ing both to make the class explicitly non-copyable,
which seems like the more natural fit here specifically: unlike
`ATTRLIST`/`DFDT`, nothing in the live tree ever copies an `MDT` by
value already (see above), and a "copy" of an open file handle plus
in-memory indexes is a much less obviously well-defined operation than
copying a `Table` array. Still a call for a human, not an autopilot
guess, per GENERAL step 4.

Also found while reading, not fixed here since step 4 gates the rest of
this file's pipeline for this turn: `GetUniqueKey()`
(`mdt.cxx:439`) still uses `sprintf` (ordinary modernization to
`snprintf`, not a correctness bug — `y`/`x` are `INT`, so the worst
case comfortably fits `CHR s[30]`); `MdtCompareKeysByIndex`/
`MdtCompareGpByIndex`/`MdtCompareGpStarts` (`mdt.cxx:182-188,360-362`)
compute `qsort` comparator results as a plain subtraction
(`ThisA->Index - ThisB->Index`, etc.) of `GPTYPE`/`SIZE_T`-typed
(unsigned) fields narrowed to `int` — the classic unsigned-subtraction-
in-a-comparator bug, wrong for any pair far enough apart to wrap when
narrowed, though not exercised by the repro above; worth a closer look
alongside the copy-semantics fix.

Row set to `blocked`; needs a signature-change decision (deep-copy vs.
non-copyable) before reprocessing via `/process src/mdt.hxx`.

## src/fpt.hxx

**2026-08-07** — blocked at GENERAL step 4. `FPT` owns a
heap-allocated `FPREC* Table` array (`new FPREC[TableSize]` in
`Init()`, `delete [] Table` in the destructor) but declares no copy
constructor and no `operator=` at all — the same "no custom copy
semantics whatsoever" shape as `mdt.hxx` this batch
(`docs/AUTOPILOT_LOG.md#srcmdthxx`), not even a hand-written (if buggy)
`operator=` like `attrlist.hxx`/`dfdt.hxx` had. The compiler-generated
copy constructor and copy-assignment operator both do a member-wise
shallow copy of `Table`.

Confirmed real with a standalone repro, same shape as the others this
batch: construct an `FPT`, open one file through it (`ffopen()`),
copy-initialize a second (`FPT b = a;` — copy constructor), let `b` go
out of scope, then let `a` be destroyed at end of scope. AddressSanitizer
reported a `heap-use-after-free` in `FPREC::GetClosed()` called from
`FPT::CloseAll()` called from `FPT::~FPT()`: `b`'s implicit shallow
copy shared `a`'s `Table` pointer, `b`'s destructor `delete []`'d it
first, and `a`'s destructor then read the same freed block while
closing any still-open files. Unlike `mdt.hxx`, this one has a
concrete, non-pointer live call site already in the tree:
`src/idb.hxx:224` declares `FPT MainFpt;` as a plain (not pointer)
member of `IDB` (Order 64, still pending) — if `IDB` is ever
copy-constructed or assigned without `IDB` defining its own copy
semantics first, `MainFpt` would be silently, shallowly copied right
along with it. Worth flagging concretely when `idb.hxx` reaches its own
turn.

Fixing it requires adding `FPT(const FPT&);` and
`FPT& operator=(const FPT&);` to `fpt.hxx` — deep-copying `Table`,
`TotalEntries`, `MaximumEntries` — or `= delete`-ing both to make the
class explicitly non-copyable. Given `FPT`'s `Table` entries each cache
a live `FILE*` (`FPREC::FilePointer`), a "deep copy" would need to
decide what a copied-but-still-open file handle even means (duplicate
the fd? reopen from the file name? leave it closed?) — a design
question, not just a mechanical copy, so this is a call for a human,
not an autopilot guess, per GENERAL step 4.

Row set to `blocked`; needs a signature-change decision (deep-copy vs.
non-copyable, and if deep-copy, a decision on open-`FILE*` semantics)
before reprocessing via `/process src/fpt.hxx`.

## src/nlist.hxx

**2026-08-07** — blocked at GENERAL step 4. `NUMERICLIST` owns a
heap-allocated `PNUMERICFLD table` array (`new NUMERICFLD[50*Ncoords]`
in both constructors, `delete [] table` in the destructor) but declares
no copy constructor and no `operator=` at all — the same "no custom
copy semantics whatsoever" shape as `mdt.hxx`/`fpt.hxx` earlier this
batch. The compiler-generated copy constructor and copy-assignment
operator both do a member-wise shallow copy of `table`.

Confirmed real with a standalone repro, same shape as the others this
batch: default-construct a `NUMERICLIST`, copy-initialize a second
(`NUMERICLIST b = a;` — copy constructor, not `operator=`, since one
isn't declared either way), let `b` go out of scope, then let `a` be
destroyed at end of scope. AddressSanitizer reported a
`heap-use-after-free` in `NUMERICLIST::~NUMERICLIST()` (`nlist.cxx:879`):
`b`'s implicit shallow copy shared `a`'s `table` pointer, `b`'s
destructor `delete []`'d it first, and `a`'s destructor then read the
same already-freed block. No confirmed copy-construction call site was
found in the live tree (every site found — `src/index.cxx`,
`src/numsearch.cxx`, `src/idb.cxx`, `src/intlist.cxx` — either
default-constructs a plain `NUMERICLIST` or `new`'s an array of them in
`src/nfldmgr.cxx:136`, never copy-constructs one), so this is latent
rather than actively crashing today. One live subclass, though:
`INTLIST` (`src/intlist.hxx:68`, Order 47, still pending) derives from
`NUMERICLIST` without declaring its own copy constructor either, so
copy-constructing an `INTLIST` would hit this transitively — worth
flagging concretely when `intlist.hxx` reaches its own turn, the same
way `STRLIST`/`FCT` deriving from `VLIST` was flagged before `vlist.hxx`
was resolved.

Fixing it requires adding `NUMERICLIST(const NUMERICLIST&);` and
`NUMERICLIST& operator=(const NUMERICLIST&);` to `nlist.hxx` — deep-
copying `table` (sized to the source's `MaxEntries`), `Count`,
`Attribute`, `Pointer`, `MaxEntries`, `StartIndex`, `EndIndex`,
`Relation`, `FileName`, `Ncoords` — or `= delete`-ing both to make the
class explicitly non-copyable, mirroring the deep-copy-vs-non-copyable
choice already resolved (case by case) for `reclist.hxx`/`attrlist.hxx`/
`dfdt.hxx`/`mdt.hxx`/`fpt.hxx`. A call for a human, not an autopilot
guess, per GENERAL step 4.

Also found while reading, not fixed here since step 4 gates the rest of
this file's pipeline for this turn: **both constructors leave
`Attribute` and `Relation` (plain `INT` members) uninitialized** —
`Ncoords`/`table`/`Count`/`MaxEntries`/`FileName`/`Pointer`/
`StartIndex`/`EndIndex` are all set, but `Attribute`/`Relation` are
not, matching the same "indeterminate primitive member" category
already found (and fixed) in `RESULT`'s constructor
(`docs/BUG_CATALOG.md#srcresultcxx`, `BUGFIX #1`) and `NUMERICFLD`'s
(`docs/BUG_CATALOG.md#srcnfieldcxx`, `BUGFIX #1`) earlier this batch.
Doesn't need a header change and can be fixed the next time this file
is reprocessed, alongside the copy-semantics decision.

Row set to `blocked`; needs a signature-change decision (deep-copy vs.
non-copyable) before reprocessing via `/process src/nlist.hxx`.

## src/intlist.hxx

**2026-08-07** — blocked at GENERAL step 4. `INTERVALLIST` (derives
from `NUMERICLIST`, itself blocked this same batch at
`docs/AUTOPILOT_LOG.md#srcnlisthxx`) owns its own heap-allocated
`PINTERVALFLD table` array (`new INTERVALFLD[50*Ncoords]` in both
constructors, `delete [] table` in the destructor) but declares no copy
constructor and no `operator=` — the same "no custom copy semantics
whatsoever" shape as `mdt.hxx`/`fpt.hxx`/`nlist.hxx` earlier this
batch. This is `INTERVALLIST`'s own, first-party defect (it owns the
array directly), not just an inherited risk from `NUMERICLIST` the way
`DF`'s risk from `FCT` was — though it inherits *that* risk too, doubly:
a compiler-generated copy would shallow-copy both `INTERVALLIST`'s own
`table` and (via `NUMERICLIST`'s own compiler-generated copy
constructor, since neither class declares one) the base class's
`table` as well.

Confirmed real with a standalone repro, same shape as the others this
batch: default-construct an `INTERVALLIST`, copy-initialize a second
(`INTERVALLIST b = a;`), let `b` go out of scope, then let `a` be
destroyed at end of scope. AddressSanitizer reported a
`heap-use-after-free` in `INTERVALLIST::~INTERVALLIST()`
(`intlist.cxx:1180`): `b`'s implicit shallow copy shared `a`'s own
`table` pointer, `b`'s destructor `delete []`'d it first, and `a`'s
destructor then read the same already-freed block (the base class's
separately-owned `table` would fail the identical way one level up, in
`~NUMERICLIST()`, if destruction got that far). No confirmed
copy-construction call site was found in the live tree (every site
found in `src/index.cxx`/`src/numsearch.cxx`/`src/idb.cxx` either
default-constructs a plain `INTERVALLIST` or assigns through its own
methods), so this is latent rather than actively crashing today.

Fixing it requires adding `INTERVALLIST(const INTERVALLIST&);` and
`INTERVALLIST& operator=(const INTERVALLIST&);` to `intlist.hxx` —
deep-copying `table` (sized to the source's `MaxEntries`) and every
other member listed below — or `= delete`-ing both, mirroring the
deep-copy-vs-non-copyable choice already resolved case by case for
`reclist.hxx`/`attrlist.hxx`/`dfdt.hxx`/`mdt.hxx`/`fpt.hxx`. Either way,
`NUMERICLIST`'s own copy-semantics decision (still pending) needs
settling first, since `INTERVALLIST`'s base subobject would otherwise
still be vulnerable to the identical bug even after `INTERVALLIST`'s
own copy operations are fixed. A call for a human, not an autopilot
guess, per GENERAL step 4.

Also found while reading, not fixed here since step 4 gates the rest of
this file's pipeline for this turn — a much larger-scale version of the
`GlobalStart`-shadowing finding already documented (not fixed, same
reason) at `docs/BUG_CATALOG.md#srcintfieldcxx`: **`INTERVALLIST`
redeclares its own private copies of nearly every member `NUMERICLIST`
already has** — `Count`, `Attribute`, `Pointer`, `MaxEntries`,
`StartIndex`, `EndIndex`, `Relation`, `FileName`, `Ncoords` are all
identically-named, identically-typed members shadowing the base
class's own, plus `table` (a different, `INTERVALFLD`-typed, array).
Every `INTERVALLIST` method reads/writes its own shadowed copies, never
the inherited ones (which `NUMERICLIST` declares `private`, making them
inaccessible to `INTERVALLIST` even if it wanted to reuse them) — so
every `INTERVALLIST` instance allocates and fully initializes *two*
separate, independent table arrays (the base's own 100-entry
`NUMERICFLD[]`, entirely unused after construction, plus the derived
class's real `INTERVALFLD[]`), wasting an allocation and ~1.2KB per
instance. The `public` inheritance itself appears to contribute nothing
functional beyond the (unused, since always shadowed) inherited method
names — a real design/encapsulation problem, but fixing it means either
making `NUMERICLIST`'s members `protected` (a header change to a
different, already-blocked file) or dropping the inheritance entirely,
both squarely a human design call, not a mechanical fix.

Row set to `blocked`; needs a signature-change decision (deep-copy vs.
non-copyable, and ideally revisited alongside `nlist.hxx`'s own
decision given the shadowing above) before reprocessing via
`/process src/intlist.hxx`.
