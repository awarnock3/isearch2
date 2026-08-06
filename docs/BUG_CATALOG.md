# Isearch2 Cleanup — Bug Catalog

## src/fc.hxx

1. **Header not self-contained** — `fc.hxx` declared `GPTYPE`/`PFILE`-typed
   members and methods, and a `friend ostream& operator<<(...)`, without
   including anything that defines those names. Its own
   `#include "defs.hxx"` was commented out, and `ostream` was left
   unqualified, relying on `using namespace std;` (declared in
   `string.hxx`, not this header) being in effect first. Confirmed real
   by compiling `fc.hxx` as the sole `#include` in a translation unit:
   it failed with nine errors (`GPTYPE`/`PFILE`/`ostream` undeclared).
   Currently silent in the tree only because every existing includer
   happens to include `defs.hxx` and `string.hxx` first — fragile, not
   guaranteed. Fixed by restoring the `defs.hxx` include and qualifying
   `std::ostream` in the friend declaration (no ABI/signature change:
   `std::ostream` and ambient-`using`-resolved `ostream` are the same
   type). See `BUGFIX #1` in source. Verified fixed by recompiling the
   same standalone reproduction, which now succeeds with zero warnings
   under `-Wall -Wextra`.

### Found but out of scope for this file (deferred to their own turns)

Discovered while reading dependencies for `fc.hxx`'s test; not fixed
here since they live in files that haven't reached their turn yet, and
public header signatures/behavior of files not currently being
processed are left alone:

- **`src/common.cxx`, `GpSwab`** — reads an uninitialized local
  (`GPTYPE Gp;`) whenever `CROSS_PLATFORM` isn't defined, which it never
  is anywhere in this build (`grep -r CROSS_PLATFORM` across the whole
  tree/build system turns up only this one `#ifdef`). So every call —
  including `FC::FlipBytes()`'s two calls — currently corrupts the
  target value with stack garbage instead of byte-swapping it. Confirmed
  by the compiler itself: `-Wuninitialized` flags it
  (`src/common.cxx:289`). `tests/src/test_fc.cxx`'s `FlipBytes` test
  deliberately only checks it doesn't throw, not that it swaps
  correctly, pending this fix.
- **`src/common.cxx`, `rename(const STRING, const STRING)`** — calls
  itself instead of the C library `::rename`, so any caller of this
  overload recurses until stack overflow. Confirmed by the compiler:
  `-Winfinite-recursion` at `src/common.cxx:335`.
- **`src/fc.cxx`, `Write()`** — `fprintf(fp, "%d\n%d\n", FieldStart,
  FieldEnd)` uses `%d` (signed) for `FieldStart`/`FieldEnd`, which are
  `GPTYPE` (`UINT4` = `unsigned int`). Values above `INT_MAX` (fields
  past the ~2GB byte-offset mark in a large indexed corpus) would print
  as negative.
- **`src/fc.cxx`, `Read()`** — parses the persisted value back via
  `STRING::GetInt()`, which returns a signed 32-bit `INT`; the same
  values that break `Write()` would also fail to round-trip correctly
  here even if `Write()` were fixed. `STRING::GetLong()` (already
  public, returns `LONG` — 64-bit on this platform) would round-trip
  the full unsigned 32-bit range without needing any `string.hxx`
  signature change.

## src/fct.hxx

1. **Header not self-contained** — same defect as `src/fc.hxx` `BUGFIX #1`,
   independently present here: `fct.hxx` declared `FC`/`INT`/`PFILE`/
   `GPTYPE`-typed members and methods, and a base class `VLIST`, without
   including anything that defines those names (`#include "defs.hxx"`,
   `"fc.hxx"`, `"vlist.hxx"` were all commented out), and left `ostream`
   unqualified. Confirmed real by compiling `fct.hxx` as the sole
   `#include` in a translation unit: it failed with 10 errors
   (`VLIST`/`FC`/`INT`/`PFILE`/`GPTYPE`/`ostream` undeclared). Currently
   silent in the tree only because every existing includer happens to
   include `defs.hxx`, `fc.hxx`, and `vlist.hxx` first — fragile, not
   guaranteed. Fixed by restoring the three includes, adding
   `#include <iostream>`, and qualifying `std::ostream` in the `Print`
   declaration and friend `operator<<` (no ABI/signature change). See
   `BUGFIX #1` in source. Verified fixed by recompiling the same
   standalone reproduction, which now succeeds with zero warnings under
   `-Wall -Wextra`.

### Found but out of scope for this file (deferred to their own turns)

Discovered while reading `src/fct.cxx` (the header's existing
implementation, needed to write `tests/src/test_fct.cxx`) — not fixed
here since `fct.cxx` hasn't reached its own turn yet (Order 128) and
its public behavior is left alone until then:

- **`src/fct.cxx`, `FctFcCompare`** — the `qsort` comparator used by
  `SortByFc()` returns `((FC*)x)->GetFieldStart() - ((FC*)y)->GetFieldStart()`,
  subtracting two `GPTYPE` (`unsigned int`) values and narrowing the
  result to the `int` `qsort` expects. This happens to sort correctly
  as long as the true difference between any two compared offsets fits
  in a signed 32-bit range, but silently misorders once two field
  offsets in the same table differ by more than ~2GB — the same
  byte-offset scale already flagged as a `GPTYPE`/`INT` boundary issue
  in `src/fc.cxx` above.
- **`src/fct.cxx`, `Write()`/`Read()`** — the same signed/unsigned
  round-trip mismatch already cataloged for `src/fc.cxx` above, one
  level up: `Write()` emits the entry count via `fprintf(fp, "%zu\n",
  TotalEntries)` (a `SIZE_T`), but `Read()` parses it back via
  `STRING::GetInt()` (signed 32-bit). A table with more than
  `INT_MAX` entries would fail to round-trip.

## src/dft.hxx

1. **Header not self-contained** — same defect as `src/fc.hxx`
   `BUGFIX #1`, independently present here: `dft.hxx` declared
   `INT`/`PFILE`/`DF`/`PDF`-typed members and methods without including
   anything that defines those names (`#include "defs.hxx"` and
   `"df.hxx"` were both commented out). Confirmed real by compiling
   `dft.hxx` as the sole `#include` in a translation unit: it failed
   with 10 errors. Currently silent in the tree only because every
   existing includer happens to include `defs.hxx` and `df.hxx` first —
   fragile, not guaranteed. Fixed by restoring the two includes. See
   `BUGFIX #1` in source. Verified fixed by recompiling the same
   standalone reproduction, which now succeeds with zero warnings under
   `-Wall -Wextra`.

2. **Missing copy constructor → double-free / use-after-free** — `DFT`
   owns a heap-allocated array (`Table`) and declares its own
   `operator=` and `~DFT()`, but never declared a copy constructor, so
   the compiler synthesized one that shallow-copies the `Table`
   pointer instead of the entries it points to. Confirmed real with a
   minimal standalone reproduction: default-construct a `DFT`,
   copy-construct a second from it (`DFT b = a;`), and let both go out
   of scope. Under ASan this aborted with a heap-use-after-free in
   `DFT::~DFT()` (`src/dft.cxx:110`) — the second destructor call
   reading/freeing memory the first destructor had already freed, i.e.
   a double-free. Real-world exposure isn't just theoretical: `RECORD`
   (`src/record.hxx:79`) holds a `DFT Dft;` member and itself declares
   no copy constructor either, so any copy of a `RECORD` (pass/return
   by value, container storage) would trigger this transitively. Per
   CLAUDE.md's rule to stop and ask before changing a header
   declaration, this was confirmed and then fixed with the user's
   explicit go-ahead: added `DFT(const DFT& OtherDft);` to `dft.hxx`
   and implemented it in `dft.cxx` (Order 126, ahead of its own turn,
   since the declaration and its only sane implementation are
   inseparable) by deep-copying entries the same way `operator=`
   already does. See `BUGFIX #2` in both files. Verified fixed by
   rerunning the same standalone reproduction under ASan+UBSan — exits
   cleanly — and by `tests/src/test_dft.cxx`'s copy-constructor test,
   which passes under `make tests-asan`.

### Found but out of scope for this file (deferred to their own turns)

- **`src/df.hxx`'s `DF` and `src/fct.hxx`'s `FCT`** (the latter already
  processed, Order 2) **have the same missing-copy-constructor smell as
  `DFT` above, unconfirmed** — both declare `operator=` without a copy
  constructor. Noticed via a `-Wdeprecated-copy` warning while writing
  this file's test (`DF MakeDf(...) { ...; return df; }` needed `DF`'s
  implicit copy constructor for the by-value return, which triggered
  the deprecation warning, which named `FCT`'s user-provided
  `operator=` as the reason `FCT`'s own implicit copy constructor is
  also deprecated — `DF` embeds an `FCT Fct;` member). Rewrote the test
  helper to fill an out-parameter instead of returning by value, so
  this doesn't block `dft.hxx`'s own turn. Unlike `DFT`, no concrete
  copy-construction call site (as opposed to assignment) was found for
  either class in the existing tree, so this is flagged as a latent
  risk rather than a confirmed active bug — worth checking for a real
  double-free repro (same technique as `BUGFIX #2` above) when
  `df.hxx` (Order 31) or `vlist.hxx` (Order 27, `FCT`'s base class,
  which has the identical pattern one level further down: a
  user-declared destructor and `Next`/`Prev` raw pointers but no copy
  constructor) reach their own turns, and considering whether `fct.hxx`
  needs a `PROCESS fct.hxx` re-run at that point.

## src/string.hxx

Processed out of order via `/process string.hxx` (its `Order` in
`PROCESSING_STATUS.md` is 26) at the user's request. Per CLAUDE.md's
history note, this is the STRING module previously flagged as having
been cleaned up once already in a parallel `isearch2-modern/` tree —
that directory doesn't exist anywhere in this environment, so this was
treated as a fresh, from-scratch pass with no prior work to port in.

1. **`ReadFile()` (both overloads) — confirmed double-free /
   use-after-free** — this is the exact "STRING::ReadFile double-free"
   referenced (but not yet re-validated in this environment) in
   `src/fc.hxx`'s catalog entry above. Both overloads unconditionally
   ran `if (Buffer) delete [] Buffer;` as their first action, *before*
   checking whether the target file even existed. On the common failure
   path — file doesn't exist — the function returned `GDT_FALSE`
   immediately afterward, leaving `Buffer` a dangling pointer to
   already-freed memory instead of `nullptr` or a fresh allocation. Any
   later use of that `STRING` — another method call, or simply letting
   it go out of scope — would then free the same memory a second time.
   Confirmed real with a standalone ASan repro: default-construct a
   `STRING`, call `ReadFile()` on a nonexistent path, let it go out of
   scope → `AddressSanitizer: attempting double-free` in `~STRING()`.
   A second, related defect lived one level deeper: on success, both
   overloads got the file's size via a **second**, redundant
   `GetFileSize()` call (which re-`stat()`s the same file) rather than
   reusing the `struct stat` already obtained by the first `stat()` a
   few lines above — a TOCTOU race (if the file vanished between the
   two `stat()` calls, `GetFileSize()` returns `-1`, which silently
   becomes a huge value once assigned to the unsigned `Length`
   (`STRINGINDEX`), passing the original's `if (Length >= 0)` check
   — always true for an unsigned type, and the exact dead code the
   `-Wtype-limits` warning on this line has been flagging in every
   build since `src/fc.hxx`'s turn — and overflowing `BufferSize` via
   `Length + 1`). Fixed by: not touching `Buffer` until `stat()`
   confirms the file exists; reusing `status.st_size` directly instead
   of a second racy `GetFileSize()` call (which also makes the
   `Length >= 0` dead code moot, since `st_size` from a `stat()` that
   just succeeded can't be negative); and resetting `Length` to 0
   (rather than leaving it at the pre-read file size over an
   all-garbage, never-written buffer) if `fopen()` fails after a
   successful `stat()`. The `STRING&` overload now delegates to the
   `CHR*` overload instead of duplicating the fix a second time. See
   `BUGFIX #1` in source. Verified fixed: the same standalone
   reproduction now exits cleanly, and
   `tests/src/test_string.cxx`'s two `ReadFile` regression tests pass
   under `make tests-asan`. `-Wtype-limits` no longer fires anywhere in
   this file.

2. **`transcode()` (used by `XmlCleanup()`) — undersized output buffer
   silently truncates the last entity** — `lennbuf = strlen(obuf)*6`
   sized the output buffer for the worst case (every character
   expanding to a 6-byte `&#NNN;` entity) with **no** slack for the
   trailing null terminator, while `maxipnt = nbuf+lennbuf-1` reserves
   the *last* byte of that buffer for the terminator by refusing to
   write into it. In the worst case those two facts collide: the byte
   reserved for the terminator is the same byte the last entity needed
   for its closing `;`, so that `;` is silently dropped. Confirmed real
   with a standalone program: transcoding three bytes ≥ 128 (forcing
   three 6-byte entities) produced `&#200;&#201;&#202` (17 chars)
   instead of the correct `&#200;&#201;&#202;` (18 chars) — malformed
   XML/HTML output. (An adjacent theory — that an empty input causes a
   heap-buffer-overflow via `new char[0]` — did not hold up under ASan
   in this toolchain: this environment's `operator new[]` rounds a
   0-byte request up to at least 1 usable byte, so writing the
   terminator into it doesn't overflow here. The fix below removes the
   reliance on that implementation-defined behavior anyway.) Fixed by
   allocating `strlen(obuf)*6 + 1`, giving every worst-case entity its
   full 6 bytes plus one guaranteed byte for the terminator. See
   `BUGFIX #2` in source. Verified fixed: the same standalone program
   now produces the correct, non-truncated output, and
   `tests/src/test_string.cxx` has both a targeted regression test for
   this exact scenario and a general `XmlCleanup` correctness test.

3. **`Replace()` (both overloads) — infinite loop on an empty search
   string** — `Search("")` matches at position 1 every time (`strstr()`
   semantics: an empty needle always matches at the start), and the
   subsequent `EraseBefore(Position + CSLen)` becomes `EraseBefore(1)`,
   which `EraseBefore` itself defines as a no-op (`if (Index <= 1)
   return;`). So `*this` never shrinks, `Search` never stops matching,
   and the loop runs forever, growing `NewString` without bound (an
   eventual `std::bad_alloc` or OOM, on however long that takes). Not
   exercised by any of the ~90 real call sites in this tree — all pass
   non-empty string literals — so this wasn't run to failure (doing so
   would just hang a build), but the mechanism is deterministic given
   `Search()`'s and `EraseBefore()`'s own documented behavior, both
   read directly rather than inferred. Fixed with an early return when
   the search string is empty. See `BUGFIX #3` in both overloads.
   Verified with a regression test that calls `Replace("", ...)` and
   asserts it returns immediately with the string unchanged — which,
   pre-fix, would have hung the test binary rather than failed an
   assertion, so this test is meaningful specifically because it
   terminates at all.

Also applied file-wide: `NULL` → `nullptr` (43 occurrences, including
the 256-entry `translate[]` table used by `transcode()`) and
`sprintf` → `snprintf` (6 call sites, all writing into fixed 256-byte
stack buffers — `snprintf`'s bound is real protection here, since
`%f` on an extreme `DOUBLE` can print hundreds of digits).

## src/string.cxx

Processed out of order via `/process string.cxx` (its `Order` is 173),
immediately after `src/string.hxx` above via `/process string.hxx`. All
three bugs found in this file — `ReadFile()`'s double-free,
`transcode()`'s undersized buffer, and `Replace()`'s infinite-loop trap
— were already discovered, fixed, and tested during that `string.hxx`
turn (`string.cxx` is where all three actually live; see
`## src/string.hxx` above for the full writeup, standalone repros, and
`BUGFIX #1`–`#3` in source). This turn re-read the file fresh end to
end specifically looking for anything the last pass missed, added the
`ISEARCH2-CLEANUP: processed` marker to `string.cxx` itself (only
`string.hxx` had it), and confirmed `make tests`/`make tests-asan`
still pass clean.

One additional finding, documented rather than fixed:

- **`Cmp()` compares with `strcmp()`, not `memcmp()`** — unlike
  `Equals()`/`CaseEquals()` just above it (both `memcmp()`-based,
  bounded by `Length`, so they correctly handle embedded null bytes),
  `Cmp()` stops at the first embedded null in either buffer. Every real
  caller (`src/thesaurus.cxx`, 4 call sites, all comparing thesaurus
  terms for sorting) only ever holds plain text, never embedded-null
  data, so this doesn't currently misbehave — flagged as a documented
  inconsistency rather than changed, since "fixing" a comparison
  function's semantics without a concrete failing case felt riskier
  than leaving it alone. Noted inline above `Cmp()` in source.

## src/record.hxx

1. **Header not self-contained** — same defect as `src/fc.hxx`
   `BUGFIX #1`, independently present here: `record.hxx` declared
   `STRING`/`PSTRING`/`DFT`/`PDFT`/`GPTYPE`/`PFILE`-typed members and
   methods without including anything that defines those names
   (`#include "defs.hxx"`, `"string.hxx"`, `"dft.hxx"` were all
   commented out). Confirmed real by compiling `record.hxx` as the sole
   `#include` in a translation unit: it failed with 24 errors.
   Currently silent in the tree only because every existing includer
   happens to include those headers first — fragile, not guaranteed.
   Fixed by restoring the three includes. See `BUGFIX #1` in source.
   Verified fixed by recompiling the same standalone reproduction,
   which now succeeds with zero warnings under `-Wall -Wextra`.

2. **Previously flagged `RECORD` copy risk — confirmed resolved, not a
   live bug** — `src/dft.hxx`'s catalog entry above flagged that
   `RECORD` holds a `DFT Dft;` member and declares no copy constructor
   of its own, so any copy of a `RECORD` would inherit whatever `DFT`'s
   copy semantics were at the time. That was written when `DFT` still
   had the shallow-copy double-free bug (`BUGFIX #2` in that section);
   since it's since been fixed with a proper deep-copying copy
   constructor, `RECORD`'s own (still compiler-generated, still
   undeclared) copy constructor is now correct by construction — each
   member (`STRING`×4, `DFT`, `GPTYPE`×2) has proper value semantics of
   its own. Verified with a standalone repro under ASan+UBSan:
   copy-construct a `RECORD` with a non-trivial `DFT`, let both copies
   destruct — exits cleanly, no double-free. (There's still a
   `-Wdeprecated-copy` warning, the same class of latent-but-unconfirmed
   finding already noted for `DF`/`FCT` under `src/dft.hxx` above — not
   fixed here for the same reason: no concrete failure to fix, and
   adding an explicit copy constructor would be a header change with no
   bug motivating it.)

### Found but out of scope for this file (deferred to its own turn)

Discovered while reading `src/record.cxx` (the header's existing
implementation, Order 165, needed to write `tests/src/test_record.cxx`)
— not fixed here since `record.cxx` hasn't reached its own turn yet:

- **`src/record.cxx`, `Write()`/`Read()`** — the same signed/unsigned
  round-trip mismatch pattern already cataloged for `src/fc.cxx` and
  `src/fct.cxx` above: `Write()` emits `RecordStart`/`RecordEnd` (both
  `GPTYPE`, i.e. unsigned) via `fprintf(fp, "%d\n", ...)` (signed format
  specifier), while `Read()` parses them back via `STRING::GetInt()`
  (signed 32-bit `atoi()`). Values above `INT_MAX` would print as
  negative and/or fail to round-trip.

## src/rcache.hxx

`RCACHE` (Result Set Cache) is currently dormant in this tree — its one
real call site, `SetCache=new RCACHE(Parent);` in `src/index.cxx`, is
commented out, and no other file constructs or calls it. All bugs below
were still confirmed with standalone repros independent of that dormant
caller, since the class's own logic is broken regardless of who (if
anyone) currently exercises it.

1. **Header not self-contained** — same defect as `src/fc.hxx`
   `BUGFIX #1`: `rcache.hxx` declared `STRING`/`IRSET`/`PIDBOBJ`/
   `INT`-typed members and methods without including anything that
   defines those names (`#include "defs.hxx"`, `"string.hxx"`,
   `"irset.hxx"`, `"idbobj.hxx"` were all commented out). Confirmed real
   by compiling `rcache.hxx` as the sole `#include` in a translation
   unit: it failed with 24 errors. Fixed by restoring the four includes.
   See `BUGFIX #1` in source. Note: `irset.hxx` itself isn't
   self-contained yet (its own turn, Order 8, hasn't come up) — both
   this fix and `tests/src/test_rcache.cxx` still need to pre-include
   its dependencies manually, the same way `rcache.cxx` itself always
   has.

2. **`Add()`'s eviction path deletes the wrong pointer — confirmed
   out-of-bounds access plus a memory leak** — when the cache is full,
   `Add()` loops over `ResultSet[0..Count)` to find `MinPos`, the index
   of the entry with the fewest total results (the one to evict), then
   ran `delete ResultSet[i]` — using the *loop variable* `i`, which by
   then equals `Count` (`== MAXCACHE`, since the loop always runs to
   completion), not `MinPos`. `ResultSet` is a fixed `IRSET*[MAXCACHE]`
   member array, so `ResultSet[MAXCACHE]` reads one element past its
   end and `delete`s whatever garbage pointer was there. Confirmed real
   with a standalone repro: fill the cache to `MAXCACHE` entries, add
   one more. Under ASan+UBSan this produced `runtime error: index 20
   out of bounds for type 'IRSET *[20]'` at the `delete` line, plus a
   LeakSanitizer-reported leak of the entry that should have been
   evicted (`ResultSet[MinPos]`) — never freed, since the wrong pointer
   was deleted, then silently overwritten and lost. Fixed by deleting
   `ResultSet[MinPos]` instead of `ResultSet[i]`. See `BUGFIX #2` in
   source. Verified fixed: the same standalone reproduction now exits
   cleanly under ASan+UBSan, and `tests/src/test_rcache.cxx`'s eviction
   test (which fills the cache and adds one more) passes under
   `make tests-asan`.

3. **`Fetch()` had no bounds check** — indexed `ResultSet[w]` directly
   with no validation that `w` was a real, currently-occupied slot.
   Callers are expected to pass a value returned by `Check()` (which
   returns `-1` for "not found"), but nothing enforced that contract,
   unlike every other indexed accessor already seen in this tree (e.g.
   `DFT::GetEntry`, `FCT::GetEntry`, both of which validate their index
   and no-op on out of range). No current caller exists to demonstrate
   misuse (the whole class is dormant, as noted above), so this wasn't
   a confirmed *active* bug the way `BUGFIX #2` was — but it's a public
   method with an obviously exploitable contract gap, cheap to close,
   and required no header change. Fixed by returning `nullptr` for
   `w < 0` or `w >= Count`. See `BUGFIX #3` in source.

Also applied: file-level and per-method doc comments in `rcache.hxx`,
including a note on the class's current dormancy so a future reader
doesn't assume its confirmed bugs were exercised in production.

## src/operand.hxx

1. **Header not self-contained** — same defect as `src/fc.hxx`
   `BUGFIX #1`: `operand.hxx` declared `INT`/`TypeOperand`/`ATTRLIST`/
   `PATTRLIST`/`OPOBJ`-typed members and a base class without including
   anything that defines those names (`#include "defs.hxx"`,
   `"string.hxx"`, `"opobj.hxx"` were all commented out). Confirmed real
   by compiling `operand.hxx` as the sole `#include` in a translation
   unit: it failed with 6 errors. Fixed by restoring the three includes.
   See `BUGFIX #1` in source. Verified fixed by recompiling the same
   standalone reproduction, which now succeeds with zero warnings under
   `-Wall -Wextra`.

### Found but out of scope for this file (deferred to their own turns)

Discovered while auditing `OPERAND`'s `ATTRLIST Attributes;` member for
the same missing-copy-constructor risk already flagged for `DF`/`FCT`
under `src/dft.hxx` above — not fixed here since neither class has
reached its own turn:

- **`src/attrlist.hxx`'s `ATTRLIST` has the identical missing-copy-
  constructor pattern as `DFT`'s confirmed bug** — a heap-allocated
  `PATTR Table;` array, a user-provided `operator=` and `~ATTRLIST()`,
  but no declared copy constructor. Unlike `DFT`, no concrete
  copy-construction call site was found in the tree, so this is a
  latent risk, not (yet) a confirmed active bug — same status as the
  `DF`/`FCT` finding.
- **`src/attrlist.cxx`, `operator=()` has no self-assignment guard** —
  unlike `STRING::operator=` (which explicitly checks `&OtherString ==
  this`), `ATTRLIST::operator=` unconditionally does
  `if (Table) delete [] Table; Init(); ... OtherAttrlist.GetTotalEntries()`.
  On self-assignment (`x = x;`), `OtherAttrlist` *is* `*this`, so by the
  time `GetTotalEntries()` runs, `Init()` has already reset it to 0 --
  every entry silently vanishes instead of being preserved. Found while
  verifying it would be safe to write an `OPERAND::operator=`
  self-assignment test (it isn't, for this reason -- so that test was
  deliberately not written; see `tests/src/test_operand.cxx`).
  **Confirmed reachable in practice, not just theoretical**, during
  `src/sterm.hxx`'s turn below: self-assigning a live `STERM` (through
  its `OPOBJ&` interface, the same way real code would) silently wiped
  its `Attributes` while correctly preserving its `Term` — see
  `## src/sterm.hxx` below for the standalone repro.
- **`src/irset.hxx`'s `IRSET` (Order 8, coming up next after
  `rset.hxx`) has its own, separate, *confirmed* version of the exact
  same missing-copy-constructor bug** — for its own `IRESULT* Table;`
  member (unrelated to the `ATTRLIST` finding above; `IRSET` derives
  from `OPERAND` but this is `IRSET`'s own bug, one level further down).
  Found and confirmed while testing whether copy-constructing an
  `OPERAND`-derived object was safe: `IRSET a(nullptr); IRSET b = a;`
  (implicit copy constructor) aborted under ASan with
  `heap-use-after-free` in `IRSET::~IRSET()` — the second destructor
  reading/freeing memory the first destructor had already freed, the
  same double-free shape as `DFT`'s confirmed-and-fixed bug.
  **Fixed** — see `## src/irset.hxx` below, `BUGFIX #2`.

Also applied: file-level and per-method doc comments in `operand.hxx`.

## src/rset.hxx

1. **Header not self-contained** — same defect as `src/fc.hxx`
   `BUGFIX #1`: `rset.hxx` declared `STRING`/`RESULT`/`PRESULT`/`INT`/
   `SIZE_T`/`DOUBLE`-typed members and methods without including
   anything that defines those names (`#include "defs.hxx"`,
   `"string.hxx"`, `"result.hxx"` were all commented out). Confirmed
   real by compiling `rset.hxx` as the sole `#include` in a translation
   unit. Fixed by restoring the three includes. See `BUGFIX #1` in
   source.

2. **Missing copy constructor *and* `operator=` → confirmed double-free
   / use-after-free** — `RSET` owns a heap-allocated array (`Table`)
   and declares a destructor, but declared *neither* a copy constructor
   *nor* an assignment operator at all (unlike `DFT`, which at least
   had a buggy `operator=` before its own fix). Confirmed real with two
   standalone ASan repros: `RSET b = a;` (copy construction) and
   `b = a;` (copy assignment) both aborted with heap-use-after-free in
   `RSET::~RSET()` — the classic shallow-copy-then-double-free shape.
   No current caller copy-constructs or copy-assigns an `RSET` (all
   real usage is via `RSET*`/`PRSET`), so — like `rcache.hxx`'s bugs —
   this was dormant, not actively triggered today. Per CLAUDE.md's rule
   to stop and ask before changing a header declaration, this was
   confirmed and then fixed with the user's explicit go-ahead: added
   `RSET(const RSET&)` and `RSET& operator=(const RSET&)` to `rset.hxx`
   and implemented both in `rset.cxx`, deep-copying entries and
   including a self-assignment guard (which `operator=` didn't have at
   all before, so self-assignment was never a distinct hazard the way
   it is for `ATTRLIST` — there was no `operator=` to guard). See
   `BUGFIX #2` in both files. Verified fixed: both standalone
   reproductions now exit cleanly under ASan+UBSan, and
   `tests/src/test_rset.cxx` has dedicated copy-constructor and
   `operator=` regression tests (plus a self-assignment test) that pass
   under `make tests-asan`.

3. **`GetEntry()`'s bounds check compared signed vs. unsigned** — was
   `Index <= TotalEntries` (`INT` vs `SIZE_T`), the same
   `-Wsign-compare` pattern already seen elsewhere in this tree. Fixed
   with an explicit cast, safe because `Index > 0` is already checked
   first in the same condition. See `BUGFIX #3` in source.

4. **`SaveTable()`/`LoadTable()` — confirmed heap-use-after-free from a
   raw memory dump of `Table`** — the most serious bug found in this
   file. Both functions used `fwrite`/`fread` to copy `RESULT`'s raw
   in-memory bytes directly to/from disk — but `RESULT` contains
   `STRING Key/DocumentType/PathName/FileName` members, each of which
   owns a heap-allocated `Buffer` pointer. A raw byte dump writes out
   those *pointer values themselves*, not the character data they point
   to; reading them back restores pointer values that point at memory
   the writing process has since freed (or, across a real process
   boundary, memory that never existed in the reading process at all).
   Confirmed real with a standalone repro: `SaveTable()` a populated
   `RSET`, `LoadTable()` it into a *fresh* `RSET` (forcing other heap
   allocations to happen in between so the stale pointer bytes can't
   coincidentally still be valid), then read back an entry's `Key` —
   aborted under ASan with `heap-use-after-free` inside
   `STRING::Copy()`'s `memcpy`. Fixed by rewriting both functions to
   serialize each `RESULT` field individually through its existing
   public getters/setters (`GetKey`/`SetKey`, etc.), text-based and
   newline-delimited, the same pattern already used throughout this
   tree (e.g. `RECORD::Write`/`Read`) — no `result.hxx` changes needed,
   since every field already had a public accessor. `GPTYPE` fields
   are now written with `%u` and round-tripped via `STRING::GetLong()`
   (not `GetInt()`), avoiding the signed/unsigned mismatch already
   cataloged for `fc.cxx`/`fct.cxx`'s own `Write`/`Read` pairs, since
   this is new code with no reason to repeat that mistake. See
   `BUGFIX #4` in source. Verified fixed: the same standalone
   reproduction now round-trips every field correctly (confirmed with a
   two-entry, all-fields version too), and
   `tests/src/test_rset.cxx`'s `SaveTable`/`LoadTable` test passes
   under `make tests-asan`.

5. **`SortByScore()`'s comparator sorted ascending, and truncated score
   differences through an `int` cast** — `RsetCompareScores` computed
   `(int)((Score1 - Score2) * 100)`, which (a) sorts *lowest* score
   first — the opposite of `IRSET::SortByScore`'s own comparator
   (`IrsetScoreCompare`, `src/irset.cxx`) for the same conceptual
   operation, which sorts descending (best match first, the sensible
   order for a search result set), and (b) truncates the difference
   through `int`, so a small-but-real score difference (e.g. 0.004)
   could round to 0 and be treated as a tie. `RSET::SortByScore()`
   isn't called anywhere in the current tree either (dormant, like
   `BUGFIX #2` above), so this wasn't actively producing bad search
   results today, but it's a genuine logic bug found by comparing
   against the sibling implementation. Fixed to sort descending using a
   sign check instead of a truncating subtraction, matching
   `IrsetScoreCompare`'s approach. See `BUGFIX #5` in source. Verified
   with a test asserting the actual sort order (highest score first).

6. **`SortByKey()`'s comparator returned a boolean, not a three-way
   comparison — `qsort` couldn't sort with it** — `RsetCompareKeys`
   returned `(Key1 == Key2)`, i.e. `STRING::operator==()`'s result: `1`
   if equal, `0` otherwise, *never* negative. A `qsort` comparator that
   can never return negative doesn't implement a valid ordering
   (undefined behavior per the C standard), so `SortByKey()` didn't
   reliably sort at all. Fixed to use `STRING::Cmp()`, which returns a
   proper `strcmp()`-style negative/zero/positive result. See
   `BUGFIX #6` in source. Verified with a test asserting the actual
   sorted order.

### Found but out of scope for this file (deferred to its own turn)

Discovered while reading `src/result.cxx` (needed to fix `BUGFIX #4`
above) — not fixed here since `result.cxx`/`result.hxx` haven't reached
their own turn yet:

- **`RESULT`'s default constructor doesn't initialize `DbNum`,
  `RecordStart`, `RecordEnd`, `Score`, or `MyMdt`** — only `HitTable`
  (under `#ifdef DO_HIGHLIGHTING`, not defined in this build) is set;
  every other non-`STRING` member is left uninitialized garbage until
  explicitly `Set*()`. Not a crash by itself (nothing reads these
  before setting them in the paths exercised so far), but a real
  uninitialized-read hazard for any caller that constructs a `RESULT`
  and reads a field before setting it.
- **`RESULT` has the same missing-copy-constructor smell as `DF`/`FCT`/
  `ATTRLIST`** — declares `operator=` but no copy constructor. Under
  this build (no `DO_HIGHLIGHTING`), `RESULT` owns no raw pointer
  directly (its only pointer member, `MyMdt`, isn't deleted by
  `~RESULT()`), so the compiler-generated copy constructor happens to
  be safe here — confirmed by this turn's own test helper
  (`MakeResult`) needing to avoid return-by-value only to dodge a
  `-Wdeprecated-copy` warning, not a crash. Would become a real
  double-free (of `HitTable`) under a `DO_HIGHLIGHTING` build, the same
  shape as `DFT`'s original bug.

## src/irset.hxx

The `IRSET` copy-constructor double-free flagged as "found but out of
scope" under `src/operand.hxx` above is now fixed here — see item 2
below.

1. **Header had no includes at all** — unlike every other file in this
   tree (which at least had a commented-out include block to restore),
   `irset.hxx` had *no* `#include`s whatsoever despite needing `OPERAND`
   (base class), `PIDBOBJ`, `IRESULT`/`PIRESULT`, `PRSET`, `MDT`, and
   more. It only "worked" because every real includer happened to bring
   in the right headers first, in the right order — confirmed by
   compiling it standalone (fails without the includes, succeeds with
   them). Fixed by adding the same include list `irset.cxx` itself
   already needed to use this header at all. See `BUGFIX #1` in source.

2. **Missing copy constructor → confirmed double-free / use-after-free**
   — `IRSET` owns a heap-allocated array (`Table`) and declares
   `operator=`/`~IRSET()`, but no copy constructor — the same pattern
   as `DFT`'s and `RSET`'s confirmed-and-fixed bugs (both with your
   prior approval). This one was already confirmed during the
   `operand.hxx` turn: `IRSET b = a;` aborted under ASan with
   heap-use-after-free in `~IRSET()`. Per your go-ahead, fixed by adding
   `IRSET(const IRSET&)` to `irset.hxx` and implementing it in
   `irset.cxx`. **Fixing this surfaced a second, deeper bug**: the
   first implementation attempt base-constructed via
   `OPERAND(OtherIrset)`, which invokes `OPERAND`'s own *implicit*
   copy constructor — and since `ATTRLIST` (the type of
   `OPERAND::Attributes`) has the identical missing-copy-constructor
   defect one level down (flagged as latent/unconfirmed under
   `src/operand.hxx` above), that shallow-copies `ATTRLIST`'s own
   `Table` and produces the exact same double-free, just one hop
   deeper — confirmed by hitting it: the standalone repro aborted in
   `ATTRLIST::~ATTRLIST()` instead of `IRSET::~IRSET()`. This
   *confirms* the `ATTRLIST` latent risk noted earlier is real and
   reachable, without needing to fix `attrlist.hxx` itself (out of
   scope, its own turn hasn't come up): `IRSET`'s copy constructor was
   reworked to default-construct the `OPERAND` base and copy
   `Attributes` through `GetAttributes()`/`SetAttributes()` instead,
   both of which go through `ATTRLIST::operator=` — which, unlike its
   copy constructor, already deep-copies correctly. See `BUGFIX #2` in
   source. Verified fixed: the standalone reproduction (both the
   original `IRSET`-level crash and the follow-up `ATTRLIST`-level one)
   now exits cleanly under ASan, and
   `tests/src/test_irset.cxx`'s copy-constructor test (which also
   verifies `Attributes` survive the copy correctly, not just avoid
   crashing) passes under `make tests-asan`.

3. **`Init()` initialized a local shadow of `ScoreSort`, not the member**
   — was `INT ScoreSort=0;`, declaring a same-named local that shadowed
   the `ScoreSort` member instead of setting it, leaving the member
   uninitialized garbage until the first `SortByScore()`/`SortByIndex()`
   call (both of which correctly assign the member, having no local to
   shadow it). No current caller reads `ScoreSort`, so this had no
   observable effect, but it's still a real uninitialized-member bug
   and was the source of a `-Wunused-variable` warning present in every
   build since this file first appeared in `TEST_ENGINE_SRCS`. Fixed by
   removing the `INT` type prefix so the assignment targets the member.
   See `BUGFIX #3` in source.

4. **`operator=()` had no self-assignment guard → confirmed silent data
   loss** — unconditionally freed `Table` and re-`Init()`'d *before*
   reading `OtherIrset.GetTotalEntries()`. On self-assignment (`x = x;`,
   which real code can reach through the virtual `OPOBJ&` interface),
   `OtherIrset` *is* `*this`, so by the time that count was read, `Init()`
   had already reset it to 0 — every entry silently vanished. The same
   shape as the `ATTRLIST::operator=` self-assignment bug already
   cataloged under `src/operand.hxx` above, just confirmed here with a
   standalone repro instead of by inspection: a 1-entry `IRSET`
   self-assigned through its `OPOBJ&` interface dropped to 0 entries.
   Fixed with an early return when `&OtherIrset == this`. See
   `BUGFIX #4` in source. Verified fixed: the same standalone repro now
   preserves the entry count, and `tests/src/test_irset.cxx` has a
   dedicated self-assignment regression test.

Also applied: 2 remaining `NULL` → `nullptr` in `And()`/`AndNot()`
(`bsearch()` result comparisons), and a dead `DOUBLE x;` local removed
from `FastAddEntry()` (another source of a standing warning).

## src/opstack.hxx

1. **Header not self-contained** — same defect as `src/fc.hxx`
   `BUGFIX #1`: `opstack.hxx` declared `OPOBJ`/`POPOBJ`/`PIRSET`-typed
   members and methods without including anything that defines those
   names (`#include "defs.hxx"`, `"string.hxx"`, `"opobj.hxx"`,
   `"irset.hxx"` were all commented out). Confirmed real by compiling
   `opstack.hxx` as the sole `#include` in a translation unit: it
   failed with 8 errors. Fixed by restoring the four includes. See
   `BUGFIX #1` in source.

2. **Missing copy constructor** — `OPSTACK` owns a linked chain of
   heap-allocated `OPOBJ*` entries but declared no copy constructor
   (only `operator=`, which was already correctly deep-copying).
   Harmless on its own only because `~OPSTACK()` didn't free anything
   (see `BUGFIX #3`) — a shallow copy of two stacks sharing the same
   node chain was inert as long as neither destructor ever touched
   those nodes. Fixing the destructor leak below would have turned this
   into the same double-free shape as `DFT`/`RSET`/`IRSET`'s confirmed
   bugs the moment anyone copy-constructed an `OPSTACK`. Per your
   go-ahead (bundled with `BUGFIX #3`, since the two are inseparable —
   fixing one without the other either leaves the leak or introduces a
   double-free), added `OPSTACK(const OPSTACK&)` to `opstack.hxx`,
   mirroring `operator=`'s already-correct push-then-`Reverse()` deep
   copy. See `BUGFIX #2` in source. Verified with a standalone repro:
   copy-construct a stack with two entries, drain and delete both
   copies independently — clean under ASan — and with
   `tests/src/test_opstack.cxx`'s dedicated copy-constructor test.

3. **`~OPSTACK()` was empty → confirmed memory leak** — any `OPSTACK`
   destroyed with entries still on it (`Head` non-null) leaked every
   remaining node, since nothing ever popped and deleted them. Live
   impact, not dormant: `OPSTACK` is constructed and destroyed
   constantly in `src/squery.cxx`'s query evaluation (`OPSTACK Stack,
   TempStack, NewStack;` as locals in multiple functions) and held as a
   value member of `SQUERY` itself. Confirmed real with a standalone
   repro: push one entry onto a stack, let it go out of scope without
   popping — LeakSanitizer caught the full leak (all 17 nested
   allocations reachable from the one un-popped `IRSET`, ~32KB).
   Fixed by popping and deleting everything still on the stack in the
   destructor, the same pattern `operator=` already used to clear
   `*this` before reassigning. See `BUGFIX #3` in source. Verified
   fixed: the same standalone reproduction now exits clean under
   LeakSanitizer, and `tests/src/test_opstack.cxx` has a dedicated
   regression test that deliberately leaves entries un-popped, passing
   under `make tests-asan` (which enables LeakSanitizer by default
   alongside ASan).

### Found but out of scope for this file (not fixed, no concrete evidence yet)

- **`operator>>(PIRSET&)` downcasts an popped `OPOBJ*` to `IRSET*` with
  a C-style cast and no type check** — if the actual popped object
  isn't an `IRSET` (e.g. it's an operator object elsewhere in the
  `OPOBJ` hierarchy), this is an invalid pointer used as if it were
  valid. Only one real call site uses this overload
  (`src/index.cxx:1300`, `TempStack >> NewIrset;`); verifying whether
  that call site can ever see a non-`IRSET` top-of-stack would require
  tracing `index.cxx`'s full RPN evaluation control flow, which wasn't
  done here. Flagged as a design smell, not a confirmed bug.

## src/filemap.hxx

Maps a "global" byte offset back to the on-disk file it falls within
(built from the parent database's main MDT), used by the indexing/merge
tools (`src/index.cxx`, `src/mergeunit.cxx`) — not on the CGI
search-request path.

1. **Header had no includes at all** — same defect as `src/irset.hxx`
   and `src/opstack.hxx`: not even a commented-out block, despite
   needing `GPTYPE`, `STRING`, `PSTRING`, `INT`, `PMDT`, and `PIDBOBJ`
   below. Confirmed real by compiling `filemap.hxx` as the sole
   `#include` in a translation unit: it failed with 10 errors. Fixed by
   adding the same include list `filemap.cxx` itself already needed to
   use this header at all. See `BUGFIX #1` in source.

2. **`printf("Lookup failed for %d\n", gp)` used a signed format
   specifier for `gp`, a `GPTYPE` (unsigned)** — the same
   signed/unsigned printf mismatch pattern already cataloged for
   `src/fc.cxx`/`src/fct.cxx`'s `Write()` functions, this time in a
   diagnostic message rather than a persisted file: a `gp` value above
   `INT_MAX` would print as negative. Low real-world severity (a
   cosmetic diagnostic, not a memory-safety issue, and `GetKeyByGlobal`/
   `GetNameByGlobal` are index-time-only), but a one-token fix directly
   in the file being processed. Fixed both occurrences to `%u`. See
   `BUGFIX #2` in source.

Also removed: a genuinely dead `STRING a;` local (declared, never used)
and two stale commented-out lines in the constructor, plus commented-out
`//  INT i;` / `//  key.GpEnd=0;` lines in both lookup functions — all
directly adjacent to code already being touched for the fixes above.

### Found but out of scope (latent, not confirmed active)

- **`FILEMAP` has the same missing-copy-constructor smell as `DF`/
  `FCT`/`ATTRLIST`/`RESULT`** — owns a heap array (`struct _table
  *Items`, each entry itself containing a `STRING Path`) and declares a
  correct destructor, but no copy constructor or `operator=` at all.
  Every real usage found (`src/index.cxx`, `src/mergeunit.hxx`) is via
  a local value or a `FILEMAP*` member, never a direct copy — no
  concrete copy-construction call site, so (like the other latent
  findings) this wasn't fixed without a confirmed bug to point to. Also
  unlike `IRSET`/`RSET`/`OPSTACK`, `FILEMAP`'s constructor
  unconditionally dereferences `Parent->GetMainMdt()`, making it
  impractical to build a minimal standalone repro the way those were
  confirmed (would need a real, populated `MDT` either way — see this
  turn's own test for what that setup requires).
- **`bsearch()`-based lookups assume `Items` is sorted ascending by
  `GpStart`**, which the constructor never explicitly sorts for — it
  relies on MDT entries already being added in that order (true by
  construction: each document's global start is the running total of
  bytes indexed so far). Not verified against MDT's actual indexing
  code path; flagged as an implicit assumption worth confirming
  whenever `mdt.hxx`/`mdt.cxx` or the indexing pipeline (`index.cxx`,
  Order 137) reach their own turns.

## src/termobj.hxx

`TERMOBJ` is a tiny abstract intermediate base (adds no state of its
own beyond `OPERAND`) for concrete search-term operand classes; `STERM`
(Order 14, not yet processed) is its only real subclass.

1. **Header not self-contained** — same defect as `src/fc.hxx`
   `BUGFIX #1`: commented-out includes for `defs.hxx`/`string.hxx`/
   `operand.hxx`. Confirmed real by compiling `termobj.hxx` as the sole
   `#include` in a translation unit: it failed with 2 errors. Fixed by
   restoring the three includes. See `BUGFIX #1` in source.

2. **`TERMOBJ` hid `OPERAND`'s virtual `operator=`** — the same
   `-Woverloaded-virtual` pattern already flagged as pre-existing noise
   under `src/operand.hxx` above (there, about `COLONDOC`/`SGMLNORM`/
   `TERMOBJ` itself, found but not fixed since none of those files had
   reached their own turn). Now that `termobj.hxx` *is* the file being
   processed, fixed for real: added `using OPERAND::operator=;`, a
   purely additive declaration that re-exposes the inherited virtual
   overload without changing or removing anything. Confirmed fixed by
   recompiling the same standalone reproduction, which no longer
   triggers the warning. See `BUGFIX #2` in source. `COLONDOC`/
   `SGMLNORM` (and any other undiscovered class in the same situation)
   still have this warning — it's specific to each derived class, not
   inherited transitively (confirmed while writing this turn's own test
   helper, which needed the identical `using` declaration itself even
   though it derives from the now-fixed `TERMOBJ`) — worth applying the
   same one-line fix whenever those files reach their own turns.

### Found but out of scope (pre-existing, not triggered by any real caller)

- **Direct same-type assignment fails to *link*, anywhere in the
  `OPOBJ` hierarchy** — discovered while drafting this turn's own test:
  `TESTTERMOBJ a, b; a = b;` (both the exact same concrete type) needs
  the compiler-generated `TESTTERMOBJ::operator=(const TESTTERMOBJ&)`,
  whose base-subobject assignment step chains down through `TERMOBJ`
  and `OPERAND`'s own compiler-generated same-type `operator=`s, and
  bottoms out needing `OPOBJ::operator=(const OPOBJ&)` — declared pure
  virtual (`= 0`) with no definition anywhere, since every real usage
  goes through the polymorphic `OPOBJ&`/`OPERAND&` interface instead
  (confirmed by grepping for direct same-type assignment across `src/`,
  `doctype/`, `Isearch-cgi/`: none found for any `OPOBJ` subclass).
  That's a link error (`undefined reference to OPOBJ::operator=`), not
  a runtime bug, and it's a structural property of the whole hierarchy
  predating this cleanup effort, not something introduced or fixable by
  `BUGFIX #2` above or by any single file's turn — flagging it here
  since `termobj.hxx`'s turn is where it was first actually triggered
  and confirmed. This turn's test file documents the finding instead of
  exercising the broken path.

## src/memcntl.hxx

A standalone C-linkage (`extern "C"`) malloc-style allocator tracking
every block it hands out in a linked list (`struct MemBlock`), so they
can all be freed together later. Ported from an original Amiga/Intuition
UI-toolkit memory tracker (per the file's own comments) to plain `new`/
`delete`. Used only by `src/marclib.cxx` (MARC bibliographic record
parsing) — a single call site each for `AllocSafe`/`FreeSafe`.

1. **Header had no includes at all** — same defect as `src/irset.hxx`/
   `src/opstack.hxx`/`src/filemap.hxx`: needs `INT4` (from `gdt.h`) for
   both the `MemBlock` struct and both function signatures, but included
   nothing. Confirmed real by compiling `memcntl.hxx` as the sole
   `#include` in a translation unit: it failed with 6 errors. Fixed by
   adding `#include "gdt.h"`, the same header `memcntl.cxx` itself
   already needed to use this header at all. See `BUGFIX #1` in source.

2. **`AllocSafe()` used plain `new`, making its own "not enough memory"
   error handling permanently unreachable dead code** — both
   allocations (`new (struct MemBlock)` and `new char[size]`) were
   plain, throwing `new`, which never returns `nullptr` on failure —
   it throws `std::bad_alloc` instead. The surrounding `if (block)`/
   `if (mem)` checks, and the `fprintf(stderr, "memcntl: Not enough
   memory...")` diagnostics they guard, were therefore dead code: on a
   real allocation failure, an exception would propagate out of this
   `extern "C"` function instead (this codebase has no exception
   handling anywhere, so that means an uncaught-exception crash via
   `std::terminate()` with no diagnostic at all) rather than the
   graceful `nullptr`-returning failure this code — and its only
   caller, `src/marclib.cxx`, which checks `AllocSafe`'s return value —
   were written to expect. Fixed by switching both allocations to
   `new (std::nothrow)`, restoring the intended contract. Also fixed a
   related gap the `nothrow` change made newly reachable: on the "data
   allocation failed" path, the just-linked `block` (allocation
   control struct) was left in the list with a null `data` pointer
   instead of being unlinked — a zombie node for later traversal to
   trip over. See `BUGFIX #2` in source.

3. **`FreeSafe()`'s "free everything" mode left `*base` dangling —
   confirmed heap-use-after-free / double-free** — after the `flag!=0`
   loop deletes every node in the list, `*base` (the caller's own
   list-head variable, passed by pointer specifically so this function
   can update it) was never reset to `nullptr`; it kept pointing at the
   first, now-deleted block. Confirmed real with a standalone repro:
   free everything, allocate one new block (which silently absorbs the
   dangling pointer into its own `->nextmem`, while `*base` itself gets
   patched back to something valid — so this step alone doesn't crash),
   then free everything a second time — the second pass walks into the
   dangling pointer, reads `->nextmem` from already-freed memory
   (heap-use-after-free, confirmed under ASan), and would go on to
   `delete` it a second time. Not triggered by any real caller today
   (the only call site in this tree always passes `flag=0`, the
   single-node-removal mode, which was already correct), but a real bug
   in the `flag!=0` path regardless, and a one-line fix. See `BUGFIX #3`
   in source. Verified fixed: the same standalone reproduction now
   exits cleanly, and `tests/src/test_memcntl.cxx` has a dedicated
   regression test exercising the exact free-all/reallocate/free-all
   sequence, passing under `make tests-asan`.

Also applied: all `NULL` → `nullptr` (6 occurrences).

## src/operator.hxx

`OPERATOR` is `OPERAND`'s sibling in the `OPOBJ` hierarchy (an AND/OR/
ANDNOT node for the RPN expression stack, vs. `OPERAND`'s search terms/
result sets), and — unlike `OPERAND`/`TERMOBJ` — fully concrete: it
implements every `OPOBJ` pure virtual itself.

1. **Header not self-contained** — same defect as `src/fc.hxx`
   `BUGFIX #1`: commented-out includes for `defs.hxx`/`string.hxx`/
   `opobj.hxx`. Confirmed real by compiling `operator.hxx` as the sole
   `#include` in a translation unit: it failed with 8 errors. Fixed by
   restoring the three includes. See `BUGFIX #1` in source.

2. **`OPERATOR` hid `OPOBJ`'s virtual `operator=`** — the same
   `-Woverloaded-virtual` pattern just fixed for `TERMOBJ`
   (`src/termobj.hxx`), and worth spelling out why it applies here too
   despite `OPERATOR` declaring its *own* `operator=(const OPOBJ&)`
   override: per the standard's precise definition, a declared
   `operator=` only counts as a class's "own" copy-assignment operator
   if the parameter type is that exact class, so `operator=(const
   OPOBJ&)` doesn't stop the compiler from *also* implicitly generating
   `OPERATOR::operator=(const OPERATOR&)` — which hides the explicit
   override from ordinary lookup, confirmed by the standing warning.
   Fixed the same way: added `using OPOBJ::operator=;`. See `BUGFIX #2`
   in source. Confirmed fixed by recompiling the same standalone
   reproduction, which no longer triggers the warning.

Checked but not a bug (unlike several other classes this cleanup has
found the same shape of issue in): **`OPERATOR` has no copy
constructor either, but it's safe** — its only member beyond `OPOBJ`
is a plain `INT OperatorType`, and `OPOBJ::~OPOBJ()` is empty (doesn't
delete the inherited `Next` pointer OPSTACK's friend access uses for
its own linked-list bookkeeping), so the compiler-generated copy
constructor here has nothing to double-free. Different from `DF`/
`FCT`/`ATTRLIST`/`RESULT`'s *unconfirmed* latent risk — this one was
checked and ruled out.

## src/sterm.hxx

`STERM` is `TERMOBJ`'s only real subclass: a concrete search term (a
single word/phrase) operand, storing the query text in a `STRING Term`.

1. **Header not self-contained** — same defect as `src/fc.hxx`
   `BUGFIX #1`: commented-out includes for `defs.hxx`/`string.hxx`/
   `termobj.hxx`. Confirmed real by compiling `sterm.hxx` as the sole
   `#include` in a translation unit: it failed with 6 errors. Fixed by
   restoring the three includes. See `BUGFIX #1` in source.

2. **`STERM` hid `TERMOBJ`'s (and transitively `OPERAND`'s) virtual
   `operator=`** — the same `-Woverloaded-virtual` pattern just fixed
   for `TERMOBJ` and `OPERATOR`. `STERM` declares its own
   `operator=(const OPOBJ&)` override, but per the standard's
   definition that still isn't `STERM`'s "own" copy-assignment operator
   (parameter type isn't `STERM`), so the compiler still generates an
   implicit `operator=(const STERM&)` that hides it. Fixed the same
   way: added `using TERMOBJ::operator=;`. See `BUGFIX #2` in source.
   Confirmed fixed by recompiling the same standalone reproduction,
   which no longer triggers the warning.

Also confirmed (not fixed here — see the cross-reference under
`src/operand.hxx` above): self-assigning an `STERM` through its
`OPOBJ&` interface silently discards its `Attributes` while correctly
preserving its `Term`. `STERM::operator=` calls `OPERAND::operator=`
(already processed, `src/operand.cxx`, Order 6, before this was
discovered), which copies `Attributes` via `OtherOp.GetAttributes
(&Attributes)` — on self-assignment that's `ATTRLIST::operator=`
self-assigning, and unlike `STRING::operator=` (which is why `Term`
survives), it has no self-assignment guard. The actual fix belongs to
`src/attrlist.hxx` (add a guard) or a `src/operand.cxx` reprocess (skip
the call when `&OtherOp == this`) — not `sterm.hxx`, which merely
inherits the behavior. `tests/src/test_sterm.cxx` deliberately has no
self-assignment test for the same reason `test_operand.cxx` doesn't.

## Isearch-cgi/config.hxx

Tiny header: just an `extern const CHR *IsearchCGIVersion;` declaration
for the version string the CGI frontends print, defined in
`Isearch-cgi/config.cxx` (Order 233, not yet processed) from the
build-time `VERS` macro.

1. **Header not self-contained** — same defect class as `src/fc.hxx`
   `BUGFIX #1` and `src/sterm.hxx` `BUGFIX #1`: `CHR` is used without
   including anything that defines it (`CHR` comes from `src/gdt.h`).
   Confirmed real by compiling `config.hxx` as the sole `#include` in a
   translation unit: it failed with `'CHR' does not name a type`.
   Currently silent in the tree only because every existing includer
   (`search_form.cxx`, `isrch_html.cxx`, `isrch_srch.cxx`,
   `isrch_fetch.cxx`) happens to include `gdt.h` first — fragile, not
   guaranteed, and the same pattern already fixed elsewhere in this
   tree. Fixed by adding `#include "gdt.h"`. See `BUGFIX #1` in source.
   Confirmed fixed by recompiling the same standalone reproduction,
   which now succeeds with zero warnings under `-Wall -Wextra`.

### Build infrastructure bug found and fixed while adding this file's test

2. **`make tests-asan` silently reused non-instrumented objects** — none
   of the `.o` pattern rules in the top-level `Makefile` depend on
   `TEST_CXXFLAGS`, and `tests`/`tests-asan` write to the same object
   paths. Running the documented pipeline order — `make tests` then
   `make tests-asan` in the same tree, exactly what GENERAL step 9 does
   for every file — left every prior `tests-asan` run silently relinking
   the plain objects instead of recompiling under `-fsanitize=address,
   undefined`. Confirmed via `nm`: objects built this way had zero
   `asan` symbols. Not specific to `config.hxx`; it's shared test
   tooling, so fixed in place rather than deferred. Fixed by adding a
   `clean-test-objs` prerequisite (`.PHONY`) to both `tests` and
   `tests-asan` that removes `$(TEST_OBJS)` before every build, forcing
   a real from-scratch recompile under whichever flags are active.
   Confirmed fixed: after the fix, `nm` on the same object built via
   `make tests-asan` shows real `__asan_*`/`__ubsan_*` symbols, and the
   full suite (275 assertions, 126 cases) still passes under genuine
   instrumentation.
