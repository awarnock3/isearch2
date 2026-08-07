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
