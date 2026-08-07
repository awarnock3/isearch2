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

## src/marcdefs.hxx

Plain C structures (`extern "C"`) for MARC bibliographic records: two
on-disk overlays (`MARC_LEADER_OVER`, `MARC_DIRENTRY_OVER`) and the
in-memory linked-list parse tree MARCLIB builds from them
(`MARC_SUBFIELD`/`MARC_FIELD`/`MARC_REC`). No member functions.

1. **Header not self-contained** — same defect class as `src/fc.hxx`
   `BUGFIX #1` and `Isearch-cgi/config.hxx` `BUGFIX #1`: `MARC_REC::length`
   is declared `INT4` without including anything that defines it (`INT4`
   comes from `src/gdt.h`). Confirmed real by compiling `marcdefs.hxx`
   as the sole `#include` in a translation unit: it failed with `'INT4'
   does not name a type`. Currently silent in the tree only because
   both direct includers that supply `gdt.h` themselves
   (`src/marc.cxx`, `src/marclib.cxx`) happen to include it first — and
   the third, `src/marclib.hxx` (Order 19, not yet processed), doesn't
   include `gdt.h` at all, relying entirely on *its own* includers
   having done so already. Fixed by adding `#include "gdt.h"` directly
   to `marcdefs.hxx`, which also removes `marclib.hxx`'s fragile
   transitive dependency without needing to touch `marclib.hxx` itself.
   See `BUGFIX #1` in source. Confirmed fixed by recompiling the same
   standalone reproduction, which now succeeds with zero warnings under
   `-Wall -Wextra`.

Also checked, not a bug: the two overlay structs are laid out entirely
in `char`/`char[]` fields (`alignof` 1), so no compiler padding can
sneak between members. Verified `sizeof(MARC_LEADER_OVER) == 24` and
`sizeof(MARC_DIRENTRY_OVER) == 12`, matching the real MARC leader and
directory-entry widths — the raw-byte overlay is sound as written.

## src/hash.hxx

`HASH` (open-addressed, quadratic-probing hash table) is **not**
dormant like `RCACHE` was — `IDBOBJ::FieldTypes`/`FileNames`
(`src/idbobj.hxx`, Order 18) are live `HASH` members that every
doctype module populates from `name=value` config lines (`FieldTypes.
AddEntry(...)` appears throughout `doctype/*.cxx`). Following the same
precedent as `src/rcache.hxx` above (fixing confirmed bugs in a header's
own not-yet-processed `.cxx` pair, already anticipated by
`TEST_ENGINE_SRCS` already listing `src/hash.cxx`), all bugs below were
fixed now rather than deferred to `src/hash.cxx`'s own turn, since they
sit directly in the parsing path every doctype module's config
loading already exercises. `src/hash.cxx` itself is left `pending` in
`docs/PROCESSING_STATUS.md` and carries no processed marker — its own
turn still owes it a dedicated pass (further modernization, its own
doc comments, etc.).

1. **Header not self-contained** — same defect class as `src/fc.hxx`
   `BUGFIX #1`: `hash.hxx` used `INT`/`CHR` (from `src/gdt.h`) and
   `STRING` (from `src/string.hxx`) without including either. Confirmed
   real by compiling `hash.hxx` as the sole `#include` in a translation
   unit: it failed with a cascade of `'INT'`/`'CHR'`/`'STRING'` "does
   not name a type" errors. Fixed by adding both includes. See
   `BUGFIX #1` in source. Confirmed fixed by recompiling the same
   standalone reproduction, which now succeeds with zero warnings under
   `-Wall -Wextra`.

2. **`HASH::Setup` divides by zero for `Size<=0`** — `HASH(INT Size)`
   passes `Size` straight through to `Setup`, which stores it as
   `TableSize` with no validation; `HashFunction`'s `abs(s % TableSize)`
   then divides by it on every `Insert`/`Find`/`Check`. No current
   caller constructs a `HASH` with an explicit size (both live members
   use the default-997 constructor), but the sized constructor is
   public API. Confirmed real with a standalone repro: `HASH h(0);
   h.Insert(item);` — crashed under ASan+UBSan with `runtime error:
   division by zero` / `SEGV` (`FPE`) at `HashFunction`. Fixed by
   falling back to the documented default (997) for `Size<=0` in
   `Setup`. See `BUGFIX #2` in source. Verified fixed: the same repro
   now exits cleanly.

3. **`AddEntry` null-pointer write when the entry has no `=`** —
   parses a `"name=value"` string via `strchr(d,'=')`, then
   unconditionally wrote `*p='\0'` to split it, with no check that
   `strchr` found anything. Any config line missing `=` (malformed
   input, truncated file, etc.) made `p` null and crashed. Confirmed
   real with a standalone repro: `HASH h; h.AddEntry(STRING(
   "no_equals_sign_here"));` — ASan reported `SEGV` (`store to null
   pointer`) at this line. Fixed by returning early when `strchr`
   returns `nullptr`. See `BUGFIX #3` in source. Verified fixed: the
   same repro now exits cleanly.

4. **`AddEntry` stack-buffer-overflow via unbounded `strcpy`** —
   `name`/`Value` are fixed `CHR[256]` buffers, but they're filled via
   plain `strcpy` from `d`, a `CHR[513]` buffer that `STRING::
   GetCString` can fill with up to 512 characters on either side of
   `=`. Any config entry with a name or value longer than 255
   characters overflowed the corresponding stack buffer. Confirmed real
   with a standalone repro: a 300-character name followed by `=value`
   — ASan reported a `stack-buffer-overflow` (`WRITE of size 301`) in
   `strcpy` at this line, correctly identifying it smashed into the
   adjacent `Value` buffer's frame slot. Fixed by switching both copies
   to `strncpy` bounded to `sizeof(buffer)-1`, with explicit
   null-termination. See `BUGFIX #4` in source. Verified fixed: the
   same repro now exits cleanly.

Also checked, not a bug requiring a fix: the class's own doc comment
documents a `State==2` ("Deleted Slot — Continue Probe") tombstone case
that `Insert`/`Find`/`Check`'s probing logic all correctly handle, but
no public method ever sets `State` to 2 — there's no `Delete`/`Remove`.
Currently dead logic, not a defect (nothing depends on it), so left
alone; documented directly in the header's class comment so a future
reader doesn't go looking for a way to trigger it. Also noted, not
fixed (would require a header signature change, and doesn't cause
incorrect behavior — see GENERAL step 4): `Insert`/`Find`/`Check`/
`GetValue`/`AddEntry`/`HashFunction` are all declared `const` despite
`Insert`/`AddEntry` mutating the table through the `Item_type *H`
member — compiles fine since `const` only applies to the pointer
itself, not what it points to, but the `const` is misleading about
what these methods actually do.

## src/idbobj.hxx

`IDBOBJ` is the abstract interface INDEX/IRSET/NUMERICFLDMGR/
MERGEUNIT/FILEMAP all program against; `IDB` (`src/idb.hxx`/`.cxx`,
both still pending) is the only current subclass.

1. **Header not self-contained** — same defect class as `src/fc.hxx`
   `BUGFIX #1`: all nine of `defs.hxx`/`string.hxx`/`mdt.hxx`/
   `dfdt.hxx`/`dfd.hxx`/`result.hxx`/`strlist.hxx`/`record.hxx`/
   `dtreg.hxx` were commented out, leaving `DFD`/`RESULT`/`STRLIST`/
   `STRING`/`GDT_BOOLEAN`/`DOUBLE`/`RECORD`/`MDT`/`DFDT` all
   undeclared. Confirmed real by compiling `idbobj.hxx` as the sole
   `#include` in a translation unit: it failed with a long cascade of
   "does not name a type" / "has not been declared" errors. Fixed by
   restoring eight of the nine includes — `dtreg.hxx` stays out, since
   nothing in this header actually names `DTREG` (a stale leftover).
   All eight restored headers are themselves still `pending` in
   `docs/PROCESSING_STATUS.md` (their own turns haven't come up), the
   same situation `src/rcache.hxx`'s `BUGFIX #1` hit with `irset.hxx` —
   `idbobj.hxx` now compiles standalone regardless, since the tree's
   real, working include order (e.g. `src/opobj.hxx`, which reaches
   `idbobj.hxx` after all eight) already proves these headers resolve
   correctly in that order today. See `BUGFIX #1` in source. Confirmed
   fixed by recompiling the same standalone reproduction, which now
   succeeds with zero errors (only pre-existing `-Wunused-parameter`
   warnings from the class's many default no-op overrides).

2. **`GpFwrite`/`GpFread`'s "must-override" stubs closed `stdout` and
   `stderr` for the rest of the process, then returned normally** —
   both are meant as a loud signal that a subclass forgot to override
   them (compare `IDB::GpFwrite`/`GpFread` in `src/idb.cxx`, the only
   current subclass, which both do override), but instead of stopping
   the program, the default body printed a "Bad call" message to
   `stderr` and then called `fclose(stdout); fclose(stderr);` before
   returning `0` — as if `0` elements were read/written, not signaling
   any error to the caller beyond that. Confirmed real with a
   standalone repro: a minimal `IDBOBJ` subclass overriding only the
   pure-virtual methods, calling the inherited `GpFwrite`, then a plain
   `fprintf(stdout, ...)` — the follow-up write failed outright (return
   value < 0), and even the repro's own diagnostic `printf` reporting
   that failure never appeared, since `stdout` was already closed.
   Fixed by replacing both `fclose` pairs with `abort()`, matching this
   tree's existing must-not-happen convention (`panic()` in
   `src/common.cxx`, which logs then calls `abort()`). See `BUGFIX #2`
   in source. Verified fixed: the same repro now aborts immediately
   (`SIGABRT`) right after printing the diagnostic, instead of
   corrupting global I/O and returning as if nothing were wrong.

Also applied: a class-level doc comment explaining `IDBOBJ`'s role and
its all-but-three-methods-optional override contract.

## src/marclib.hxx

Free-function C-style MARC record/field parsing library; `src/marc.cxx`
(Order 143, still pending) is its only current caller. Unlike every
other `.hxx` processed so far, this header was **already**
self-contained — no `BUGFIX #1` needed here, because the previous
turn's fix to `src/marcdefs.hxx` (adding `#include "gdt.h"` there)
transitively closed the exact gap this header would otherwise have had
(confirmed: compiling `marclib.hxx` as the sole `#include` in a
translation unit succeeds with zero errors). Following the same
precedent as `src/rcache.hxx` and `src/hash.hxx` above (fixing
confirmed bugs in a header's own not-yet-processed `.cxx` pair rather
than deferring), all bugs below were found and fixed in
`src/marclib.cxx`. `src/marclib.cxx` itself stays `pending` in
`docs/PROCESSING_STATUS.md` with no processed marker.

1. **`SetSubF` wrote past `subfcodes[21]` for a field with more than 20
   subfields** — `subfcodes[0]` is a running count with no bound check
   against the fixed-size array it indexes into (`MARC_FIELD`'s
   `subfcodes[21]`, from `src/marcdefs.hxx`: index 0 is the count, so
   only indices 1..20 are valid data slots). Every `$`-delimited
   subfield in a field's raw data incremented the count and wrote a
   code byte, with nothing stopping it past 20. Confirmed real with a
   standalone repro: a field built with 25 subfields — UBSan reported
   `index 21 out of bounds for type 'char [21]'` at the write. Past
   that point it corrupts `MARC_FIELD`'s own `length` member and then
   heap memory beyond the struct (it's always heap-allocated via
   `AllocSafe`, per `SetField`/`GetMARC`). Fixed by capping the count
   at the array's usable size; subfield *data* past the 20th is still
   linked into the field's subfield list, just no longer indexable by
   code via `GetSubf()` (matches this tree's established
   degrade-rather-than-corrupt pattern, e.g. `HASH::Insert`'s table-
   overflow return code). See `BUGFIX #1` in source. Verified fixed:
   the same 25-subfield repro now exits cleanly under ASan+UBSan.

2. **`SetField` trusted a directory entry's field-start offset with no
   bounds check, reading out of the record buffer** — `f->data =
   rec->BaseAddr + GetNum(dir->fstart,5)` took the offset straight from
   the (possibly corrupted or adversarial) MARC record's own directory,
   then immediately read two bytes through it for `indicator1`/
   `indicator2`, with no check that the offset actually lands inside
   the record. Confirmed real with a standalone repro: a 37-byte record
   whose directory entry claims a field starts at offset 9000 — ASan
   reported a `heap-buffer-overflow READ` at the `indicator1` line.
   Since `ReadMARC` reads records straight off disk, a truncated or
   adversarially-crafted MARC file reaches this path directly. Fixed by
   validating the computed offset against `rec->length` (available via
   the `MARC_REC*` already passed in) and rejecting the field — same
   "return `nullptr`" convention this function already uses for
   allocation failure — when it doesn't fit. See `BUGFIX #2` in source.
   Verified fixed: the same repro now safely prints "bad field start
   offset" and rejects the record instead of reading out of bounds; a
   separate repro with a well-formed record (real leader/directory/
   subfields) still parses correctly and finds both subfields, so nothing
   legitimate regressed.

3. **`normalize()` used `sprintf` into a caller-owned buffer with no
   size parameter** — modernization target (`sprintf`-family →
   `snprintf`), sharpened by GENERAL step 4 keeping this function's
   signature frozen (it takes no `out`-buffer-size parameter to pass
   through). `mainclass`/`decimal`/`subcutter`, the pieces fed into the
   format string, are all built from fixed-size local buffers whose own
   loops already bail out (returning `nullptr`) if the source would
   overflow them — so the format's total output is provably under 64
   bytes for any input that reaches the `sprintf` call. Switched to
   `snprintf(out, 64, ...)`: identical output for every realistic
   caller, safe truncation instead of a theoretical overflow for a
   caller with an undersized buffer. Currently dead code (no caller in
   this tree — confirmed via `grep`), same as `RCACHE` was, so this is
   a contract-gap close rather than a live-exploit fix. See `BUGFIX #3`
   in source.

Also modernized throughout `marclib.cxx`: every code-position `NULL` →
`nullptr` (comments describing "returns NULL" behavior left as prose).

## src/md5.hxx

Classic public-domain MD5 (RFC 1321) implementation by Colin Plumb,
1993; `src/md5sum.cxx` (Order 146, still pending) is this tree's only
current caller, and only through the opaque `MD5Init`/`MD5Update`/
`MD5Final` trio — nothing external touches `MD5Context`'s fields or
calls `MD5Transform` directly, which is what made fixing `uint32`
in-place safe without a header-signature stop-and-ask: no caller's
visible contract changes, only this header's own internal (and
currently wrong) implementation detail.

1. **`uint32` was 64 bits wide on this platform, not 32** — typedef'd
   from `unsigned long`, correct on the 32-bit-`long` platforms this
   file targeted in 1993 (with a `__alpha` carve-out for one of the
   first 64-bit exceptions of that era), but every 64-bit Unix/Linux
   target this tree actually builds on today uses the LP64 model, where
   `long` is 64 bits too — so the 1990s heuristic now picks the wrong
   branch everywhere. This broke two separate things at once: (a) MD5's
   bit-rotation macro (`w<<s | w>>(32-s)`) no longer wraps at the
   32-bit boundary the algorithm requires, silently producing
   non-standard digests; (b) `MD5Transform`'s `(uint32*)ctx->in` cast,
   meant to view the 64-byte `in` buffer as 16 32-bit words, instead
   reads/writes 16 *8*-byte words — 128 bytes into a 64-byte buffer.
   Confirmed real two ways: `sizeof(uint32)` was 8 on this platform, and
   a standalone repro running `MD5Init`/`MD5Update`/`MD5Final` crashed
   under ASan with a stack-buffer-overflow at the `MD5Transform` write.
   Fixed by using the real fixed-width `uint32_t` from `<stdint.h>`
   instead of guessing from platform macros. See `BUGFIX #1` in source.
   Verified fixed two ways: the same repro now runs clean under
   ASan+UBSan, and — more importantly, since a memory-safe wrong answer
   is still wrong — a standalone repro checking all 7 RFC 1321 test
   vectors (`MD5("") == d41d8cd9...`, `MD5("abc") == 900150983c...`,
   etc.) now matches every one exactly; before the fix these were never
   checked anywhere in this tree; `sizeof(struct MD5Context)` also
   correctly reports 88 now (was 112).

2. **`MD5Final`'s scrub-on-completion cleared 8 bytes, not the whole
   context** — `memset(ctx, 0, sizeof(ctx))`, but `ctx` is a
   `struct MD5Context *`; `sizeof(ctx)` is the pointer's size, not the
   88-byte struct it points to. The comment right there ("In case it's
   sensitive") states the intent plainly — this line has never actually
   fulfilled it. GCC's `-Wsizeof-pointer-memaccess` already flags this
   exact mistake. Confirmed real with a standalone repro: after
   `MD5Final`, `memcmp`ing the whole context against a zeroed buffer of
   the same size failed before the fix, succeeded after. Fixed by
   changing to `sizeof(*ctx)`. See `BUGFIX #2` in source.

## src/conf.h

No bugs found. `conf.h` is machine-generated by `configure` from
`src/conf.h.in` (the `@VAR@` placeholders there), substituting four
platform type-width constants (`SIZEOF_SHORT_INT`, `SIZEOF_INT`,
`SIZEOF_LONG_INT`, `SIZEOF_LONG_LONG_INT`). Verified all four match
this platform's actual `sizeof(short)`/`sizeof(int)`/`sizeof(long)`/
`sizeof(long long)` via `static_assert` (see
`tests/src/test_conf.cxx`) — the checked-in snapshot is accurate, not
stale. Already fully self-contained (plain `#define`s, no undeclared
types). Added a doc comment and the processed marker to both `conf.h`
and `conf.h.in` — mirrored into the `.in` template too so a future
`./configure` re-run doesn't silently drop them from the regenerated
output and cause ANALYZE to re-queue an already-audited file.

## src/confwin.h

No bugs found — though this took a second look to be sure. First
impression: the `SIZEOF_INT`/`SIZEOF_LONG_INT` values looked swapped
between the `#ifndef _WIN32` and `#else` branches (2/4 vs. 4/4), since
"non-Windows" claiming `sizeof(int) == 2` is wrong for any modern
Unix. But `gdt.h` only `#include`s this file at all inside
`#if defined(_MSDOS) || defined(_WIN32)` (falling back to `conf.h`
otherwise) — so within `confwin.h`, `#ifndef _WIN32` doesn't mean
"Unix"; it means "`_MSDOS` defined without `_WIN32`", i.e. classic
16-bit real-mode MS-DOS, where `int` genuinely was 2 bytes. The `#else`
branch (`_WIN32` defined) is Win32/Win64's LLP64 data model, where
`int`/`long` both stay 32 bits even on 64-bit Windows (unlike Unix's
LP64, where `long` is 64-bit — see `src/md5.hxx` `BUGFIX #1` above for
exactly that distinction mattering). Both branches are correct for
their respective targets. Verified by compiling this header standalone
under both `-D_WIN32` and without it — clean either way (pure macros,
no other includes to fail). Not confirmable end-to-end against a real
Windows/DOS toolchain from this Linux environment, but the LLP64/16-bit
values themselves are well-established platform ABI facts, not
something specific to this codebase.

Also added: an explanatory comment (this file had none, which is
exactly what made the false-positive above easy to reach), and an
`#ifndef`/`#define` include guard for consistency with every other
header in this tree — harmless in practice today since `gdt.h`'s own
guard already prevents `confwin.h` from being pulled in twice in one
translation unit, but not guaranteed if something ever includes it
directly.

## src/gdt.h

No bugs found. This is the foundational typedef header nearly every
other file in the tree depends on (`INT`/`CHR`/`UINT4`/`GDT_BOOLEAN`/
etc.), so it got a thorough pass: already self-contained (own
`#ifndef`/`#define` guard, includes `<stdio.h>`/`<stdlib.h>` directly,
pulls in `conf.h`/`confwin.h` itself — confirmed by compiling it
standalone). Verified via `static_assert` that every size-derived
typedef actually matches its documented width on this platform
(`INT2`/`UINT2` == 2 bytes, `INT4`/`UINT4` == 4, `INT8`/`UINT8` == 8,
`CHR` == 1, `GDT_TRUE`/`GDT_FALSE` round-trip through `bool`
correctly) — the `#if (SIZEOF_INT == N)` / `#else #if (SIZEOF_LONG_INT
== N)` cascades all resolve to the branches this platform's `conf.h`
values (verified under `src/conf.h`'s own entry above) actually
predict.

One line checked closely but left alone, not confirmed as a bug: `#if
!defined(_MSDOS) || !defined(WINAPI)` guarding the `LONG`/`ULONG`
typedefs. Every *other* DOS/Windows-family check in this same file uses
`defined(_MSDOS) || defined(_WIN32)` ("any DOS/Windows build"), which
by analogy suggests this line's intent was
`!defined(_MSDOS) && !defined(WINAPI)` (skip only on a real Unix build
with neither macro present) — as written, the typedefs are skipped only
when `_MSDOS` and `WINAPI` are *both* defined at once, a narrower and
arguably backwards condition. But `WINAPI` is never defined anywhere in
this tree's own build files (`grep -rn WINAPI` across `src/`,
`doctype/`, `Isearch-cgi/`, and every `*.mak`/`configure.ac` turns up
only this one line), so it's unreachable on every config this codebase
actually ships — and even where it might trigger on some external
build, redeclaring `LONG` as `long` a second time is legal C++ (same
underlying type), not a compile error. Not enough to confirm as a live
defect, so left as-is with a comment explaining the ambiguity for
whoever looks next.

## src/defs.hxx

Tree-wide grab bag of `extern` declarations (defined in `defs.cxx`,
Order 122, still pending), GDT-derived typedefs, Z39.50/GILS
attribute/structure-type numbers, on-disk file-extension/size
constants, and a handful of convenience macros. No functions, so
nothing to modernize there (no `NULL`/`sprintf`); the two findings
below are both macro-related.

1. **`COUT` macro relied on an unqualified `cout` — not self-contained**
   — `#define COUT cout` expands to a bare, unqualified name that this
   header does nothing to declare; it only ever compiled because every
   real caller (confirmed for `src/registry.cxx`, transitively via
   `registry.hxx` → `common.hxx`/`string.hxx`, and `doctype/uspat.cxx`,
   directly via its own `#include <iostream>`) happened to already have
   `using namespace std;` in effect first — the same "fragile, not
   guaranteed" shape as every `BUGFIX #1` header-self-containment entry
   elsewhere in this catalog, just via a macro instead of an undeclared
   type. Fixed by adding `#include <iostream>` and qualifying the
   expansion as `std::cout`. See `BUGFIX #1` in source. Verified with
   `tests/src/test_defs.cxx`'s dedicated test, which redirects
   `std::cout`'s streambuf and confirms `COUT << ...` actually writes
   through it, without the test file itself ever doing
   `using namespace std;`. One small, non-blocking ripple noted, not
   fixed here: `doctype/uspat.cxx:102` independently does its own
   `#define COUT cout` after already inheriting this header's
   definition (via `common.hxx` → `defs.hxx`, included earlier in that
   file) — harmless today only because both definitions were textually
   identical; after this fix they differ, so compiling `uspat.cxx` will
   start emitting a (non-fatal, no `-Werror` anywhere in the real build)
   "COUT redefined" warning until `uspat.cxx` reaches its own turn
   (Order 231) and its now-fully-redundant local `#define` is removed.
   `doctype/` isn't part of `TEST_ENGINE_SRCS`, so `make tests`/
   `make tests-asan` don't see this either way.

### Found but out of scope for this file (deferred, not fixed)

- **`EXIT_ERROR`/`RETURN_ERROR`/`RETURN_ZERO` aren't wrapped in
  `do { ... } while(0)`** — the standard hygiene fix for multi-statement
  macros; without it, a use like `if (x) EXIT_ERROR; else ...` can
  silently misparse (the macro's own closing `}` ends the `if`, leaving
  a stray `;` and then an orphaned `else`). Confirmed this isn't merely
  theoretical for this exact codebase: `grep` across `src/`/`doctype/`/
  `Isearch-cgi/` for these three names finds ~50 call sites, all but one
  followed by a normal trailing `;`; the one exception,
  `src/result.cxx:229`, invokes `EXIT_ERROR` with **no** trailing
  semicolon at all, relying on today's bare-`{}` expansion being a
  self-contained compound statement. `result.cxx` is already compiled
  by `make tests` (it's in `TEST_ENGINE_SRCS`), so wrapping these macros
  in `do/while(0)` here — which requires a terminating `;` at every call
  site — would break that build today. Left unfixed pending
  `result.cxx` reaching its own turn (Order 167), at which point both
  should change together: add the missing semicolon there, wrap the
  three macros here.

## src/common.cxx

Tree-wide grab bag of free functions: filesystem path helpers, file/db
existence and size checks, endianness helpers, and a few string/date
utilities (see the file-level comment added to `common.hxx`). Two of
the six findings below were already flagged in advance, with root
cause identified, during `fc.hxx`'s turn — see
`docs/BUG_CATALOG.md#srcfchxx`, "Found but out of scope for this file".

1. **`IsFile`'s Windows branch(es)** — both overloads had
   `if (_S_IFREG && status.st_mode)`: a nonzero constant logically
   ANDed with the mode word, true for anything `stat()`/`_stat()` could
   report — directories included — not just regular files. Needs the
   actual bitwise AND against the file-type bits,
   `status.st_mode & _S_IFREG`. This is the file/bug used as this
   project's own worked example for this catalog's format (see
   CLAUDE.md). Fixed identically in both the `STRING` and `CHR*`
   overloads; see `BUGFIX #1` in source. Windows-only code, so not
   exercised by this Linux build either way — `tests/src/test_common.cxx`
   covers `IsFile`'s `S_ISREG` (UNIX) branch instead, which was already
   correct.
2. **`GpSwab`** — read an uninitialized `GPTYPE Gp;` whenever
   `CROSS_PLATFORM` isn't defined, which is always in this build
   (confirmed by `-Wuninitialized` and, independently, by `grep -r
   CROSS_PLATFORM` finding only this one `#ifdef` tree-wide). Simply
   seeding `Gp` from `*GpPtr` wasn't sufficient on its own: the
   function's word-swap step only reverses the two 16-bit halves of
   `Gp`, not its 4 individual bytes — a full byte-order reversal (what
   every real caller needs; `FC::FlipBytes()`/`MDTREC::FlipBytes()`
   only ever call it when the on-disk data's endianness doesn't match
   the host's) only happened when `CROSS_PLATFORM` was defined, because
   `swab()` additionally byte-swapped each half before the word-swap
   ran. Replaced the whole two-step trick with a direct, unconditional
   4-byte reversal that's correct regardless of `CROSS_PLATFORM` or
   `swab()` availability. See `BUGFIX #2` in source. Verified with
   `tests/src/test_common.cxx` (byte-level reversal and
   swap-is-its-own-inverse) and by strengthening
   `tests/src/test_fc.cxx`'s `FlipBytes` test from "doesn't throw" to
   an actual round-trip check, per the note left there during `fc.hxx`'s
   turn.
3. **`rename(const STRING, const STRING)`** — `return rename(From, To);`
   inside this very overload was an exact-match call to itself (`STRING`
   needs a user-defined conversion to reach the C library's
   `rename(const char*, const char*)`, and overload resolution always
   prefers an exact match over one needing a conversion), so every call
   recursed until the stack overflowed. Confirmed by
   `-Winfinite-recursion`. Fixed by casting both arguments to
   `const char*` first, making the C library overload the exact match.
   See `BUGFIX #3` in source. Verified with a real
   `mkstemp`-created file in `tests/src/test_common.cxx`: renames it,
   confirms the new path exists and the old one doesn't.
4. **`RemoveFileExtension`** — `SearchReverse('.')` returns 0 when
   there's no `.`, and `EraseAfter(0)` truncates to zero characters — so
   a filename with no extension had its entire name wiped, instead of
   being left alone the way the analogous `RemovePath()` leaves a
   slash-less name alone. Fixed by guarding the erase behind the same
   "found" check `RemovePath()` already uses. See `BUGFIX #4` in
   source. (Note: `RemoveFileExtension` is declared but never called
   anywhere else in this tree today, so this was latent rather than an
   active miscompile of real behavior. Its complementary
   `EraseAfter(x)` — keeping the matched `.` itself rather than
   stripping it too — is unrelated pre-existing behavior, left as-is;
   documented in `common.hxx`'s new doc comment and covered by
   `tests/src/test_common.cxx` so it doesn't regress silently.)
5. **`ParseIsoDate`'s `date_val`/`time_val`** — both declared with no
   initializer and only conditionally assigned (`date_val` inside
   `if (TmpDate.Search('-'))`; `time_val` inside either the no-`'T'`
   branch or `if (TmpTime.Search(':'))`), then unconditionally read by
   the final `return (date_val + time_val)`. A date with no `-` or a
   `T`-bearing input whose time half has no `:` reads one or both
   uninitialized. Fixed by initializing both to `0.0` at declaration.
   See `BUGFIX #5` in source.
6. **`ParseIsoDate`'s non-digit-date path leaked `tDate`** — the
   `else { return -99999999.0; }` branch returned before reaching the
   `delete [] tDate;` a few lines down, leaking the `NewCString()`
   buffer on every call with a `-`-containing but non-digit-leading
   date. Confirmed with LeakSanitizer via
   `tests/src/test_common.cxx`'s non-digit-date test before the fix.
   Fixed by deleting `tDate` on both paths. See `BUGFIX #6` in source.

Also modernized: the one `sprintf` call (in `ParseIsoDate`, formatting
the normalized `YYYY-MM-DD` digits) is now `snprintf`, bounded by the
`NewCString()`-allocated buffer's actual size
(`TmpDate.GetLength() + 1`). No `NULL` usages were present.

## src/strlist.cxx

`STRLIST`: an ordered list of `STRING` entries, each entry its own
`VLIST` node (see the file-level comment added to `strlist.hxx`). No
`NULL`/`sprintf` usages to modernize.

1. **`operator=` had no self-assignment guard** — `Clear()` ran first,
   deleting every node; `GetTotalEntries()` was then read off
   `OtherStrlist` afterward, which for `list = list;` is the same
   object `Clear()` had just emptied, so the copy loop below saw 0
   entries and copied nothing back — silently wiping the whole list.
   Fixed with a `this == &OtherStrlist` guard. See `BUGFIX #1` in
   source; regression test in `tests/src/test_strlist.cxx`.
2. **`Split(const CHR*, const STRING&)` looped forever on an empty
   Separator** — `S.Search("")` matches at position 1 every call
   (documented empty-needle behavior), and
   `S.EraseBefore(Position + SLen)` with `SLen == 0` is
   `EraseBefore(1)`, a documented no-op — so `S` never shrinks and the
   loop never terminates. Confirmed with a standalone repro linking the
   real `STRING`/`VLIST`/`STRLIST` implementations, run under a
   timeout: it hung and had to be killed. The identical failure mode
   was already found and fixed in `STRING::Replace`
   (`docs/BUG_CATALOG.md#srcstringhxx`, `BUGFIX #3`); fixed here the
   same way, with an early-return guard for `SLen == 0`. See
   `BUGFIX #2` in source; regression test bounds the call and asserts
   it returns a single whole-string entry rather than hanging.
3. **The two `Split` overloads disagreed on a trailing empty
   segment** — `Split(const CHR*, ...)` only appends the remainder if
   it's non-empty; `Split(const CHR, ...)` appended it unconditionally,
   so splitting `"a,b,"` produced `["a","b",""]` on the single-char
   overload but `["a","b"]` on the string overload for the same
   separator. Both overloads are exercised across the tree (e.g.
   `src/Iget.cxx:151` uses the `CHR` overload, `src/squery.cxx:104` the
   `CHR*` overload) with no indication either relies on a trailing
   empty entry. Matched the `CHR` overload to the `CHR*` overload's
   behavior. See `BUGFIX #3` in source.

### Found but out of scope for this file (deferred, not fixed)

- **Missing copy constructor, inherited from `VLIST`** — `STRLIST` has
  no explicit copy constructor (only `operator=`, fixed above), so
  copy-construction (as opposed to assignment) falls through to
  `VLIST`'s compiler-generated shallow copy, corrupting the circular
  list exactly as described for `VLIST` itself
  (`docs/AUTOPILOT_LOG.md#srcvlisthxx`, which already named `STRLIST`
  as one of the two derived classes carrying this risk today). Every
  call site in this tree already avoids triggering it — see `Split()`
  above, which default-constructs `NewList` and assigns rather than
  copy-constructing. Not fixed here because the fix belongs in
  `vlist.hxx` (adding a declaration there), which is already `blocked`
  pending a human header-signature decision; duplicating that block on
  this row would just be the same open question asked twice. Documented
  in `strlist.hxx`'s new file-level comment instead.

## src/attr.cxx

No bugs found. `ATTR` is a small value type (a Z39.50/GILS search
attribute: set id, type, value) wrapping two `STRING`s and an `INT`,
with no raw pointers or manual memory management of its own. Its
`operator=` copies all three fields by value — safe under
self-assignment, unlike `STRLIST`'s `Clear()`-then-read pattern (see
`docs/BUG_CATALOG.md#srcstrlistcxx`, `BUGFIX #1`) — and, unlike the
`VLIST`-derived classes, `ATTR` has no explicit copy constructor but
doesn't need one: its members are `STRING` (has its own correct copy
constructor) and `INT` (trivially copyable), so the compiler-generated
one is already correct. No `NULL`/`sprintf` usages. Added file-level
and per-function doc comments plus `tests/src/test_attr.cxx`, including
a self-assignment regression test.

## src/df.cxx

`DF`: a field name paired with an `FCT` of byte-offset occurrences (see
the file-level comment added to `df.hxx`). No `NULL`/`sprintf` usages.

1. **`operator=` had no self-assignment guard** — `df = df;` would
   reach `Fct = OtherDf.Fct;` with `OtherDf.Fct` being the very same
   `FCT` as `Fct`. Fixed with a `this == &OtherDf` guard, the same
   pattern as `STRLIST::operator=`'s fix this batch (see
   `docs/BUG_CATALOG.md#srcstrlistcxx`, `BUGFIX #1`). See `BUGFIX #1`
   in source; regression test in `tests/src/test_df.cxx`.

### Found but out of scope for this file (deferred, not fixed)

- **`FCT::operator=` itself has the identical missing-self-assignment-
  guard bug** (`src/fct.cxx`) — `Clear()` runs first, then
  `OtherFct.GetTotalEntries()` is read, so `fct = fct;` (directly, or
  transitively via `DF::operator=` before this turn's fix above) Clears
  itself before "copying" its own now-empty contents back, silently
  losing every entry. Confirmed by inspection while reading `FCT` as a
  dependency of `DF`; not reproduced standalone here since `FCT` isn't
  the file on this turn. Unlike every other "found but out of scope"
  note elsewhere in this catalog, `src/fct.cxx` (Order 2) is already
  marked `done` (2026-08-05) rather than still-`pending` — this predates
  the self-assignment class of bug being on anyone's radar (`STRLIST`'s
  copy of the same bug, above, is what surfaced the pattern). Left
  unfixed here rather than reopening an already-`done` file's row
  mid-turn on a different file; recommend `/process src/fct.cxx` to
  pick it up deliberately. `DF`'s own guard (`BUGFIX #1` above) closes
  the hole for `DF` callers in the meantime, but any other direct
  `FCT`-copying caller remains exposed until `fct.cxx` is revisited.
- **Missing copy constructor, inherited from `VLIST` (via `FCT`)** —
  same shape as `STRLIST`'s deferred finding
  (`docs/BUG_CATALOG.md#srcstrlistcxx`): `DF` has no explicit copy
  constructor, so copy-constructing a `DF` falls through to `FCT`'s
  (also absent, therefore `VLIST`'s compiler-generated shallow) copy
  constructor for the `Fct` member. Tracked at
  `docs/AUTOPILOT_LOG.md#srcvlisthxx`; not duplicated as a second
  blocked row here for the same reason given in `strlist.hxx`'s turn.

## src/dfd.cxx

`DFD`: a file number plus an `ATTRLIST` of attributes — field name and
field type are themselves stored as attributes rather than as their own
members (see the file-level comment added to `dfd.hxx`). No
`NULL`/`sprintf` usages.

1. **`operator=` had no self-assignment guard** — `dfd = dfd;` would
   reach `Attributes = OtherDfd.Attributes;` with `OtherDfd.Attributes`
   being the very same `ATTRLIST` as `Attributes`, hitting
   `ATTRLIST::operator=`'s own missing-self-assignment-guard bug
   (`docs/BUG_CATALOG.md#srcoperandhxx`, confirmed reachable during
   `sterm.hxx`'s turn) and silently emptying it — which, since field
   name/type live inside `Attributes` here, would also have wiped
   those. Fixed with a `this == &OtherDfd` guard, the same pattern used
   for `DF::operator=` (`docs/BUG_CATALOG.md#srcdfcxx`, `BUGFIX #1`).
   See `BUGFIX #1` in source; regression test in
   `tests/src/test_dfd.cxx`.

### Found but out of scope for this file (deferred, not fixed)

- **Missing copy constructor, inherited from `ATTRLIST`** — same shape
  as `DF`'s deferred finding just above: `DFD` has no explicit copy
  constructor, so copy-constructing a `DFD` falls through to
  `ATTRLIST`'s (also absent, therefore compiler-generated shallow) copy
  constructor for the `Attributes` member — the exact bug that got
  `attrlist.hxx` blocked this batch (`docs/AUTOPILOT_LOG.md#srcattrlisthxx`).
  Latent rather than confirmed here specifically: every `DFD`-copying
  call site found in the tree (e.g. `DFDT::AddEntry`/`FastAddEntry` in
  `src/dfdt.cxx`) default-constructs then assigns
  (`DFD Dfd; Dfd = DfdRecord;`), the same avoidance pattern already
  seen for `STRLIST`/`DFD`-shaped classes elsewhere, never
  copy-constructing directly. Tracked at
  `docs/AUTOPILOT_LOG.md#srcattrlisthxx`; not duplicated as a second
  blocked row here for the same reason given in `df.cxx`'s turn.

## src/mdtrec.cxx

`MDTREC`: one indexed document's key, doctype, path/file name, and its
byte-offset spans (see the file-level comment added to `mdtrec.hxx`).
All string fields are fixed-size `CHR` buffers, not `STRING` — this
class mirrors its on-disk, fixed-length record layout, since
`MDT::GetEntry`/`AddEntry` (`src/mdt.cxx`, Order 36, still pending)
`fread()`/`fwrite()` it as a raw block. No `NULL`/`sprintf` usages.

1. **`operator=` and every string getter assumed the fixed buffers were
   already null-terminated within their size** — `strcpy` in
   `operator=` and the implicit `STRING::operator=(const CHR*)` (which
   calls `strlen`) in `GetKey`/`GetDocumentType`/`GetPathName`/
   `GetFileName`/`GetFullFileName` all scan for a null byte with no
   bound. Confirmed this isn't just theoretical: `MDT::GetEntry`
   (`src/mdt.cxx:240`) does `fread((char*)MdtrecPtr, 1, sizeof(MDTREC),
   MdtFp);` — a raw byte-level read directly into an `MDTREC`'s memory,
   bypassing the constructor's zeroing entirely. A corrupt or truncated
   on-disk record (short reads are already handled with a `memset`
   fallback right after that `fread`, but a *full*-size read of
   corrupt/incompatible content isn't) could leave any of these buffers
   with no null byte anywhere in it, and the unbounded scan would then
   read past the end of the array into whatever memory follows —
   silently absorbing adjacent fields' bytes into the returned `STRING`
   rather than crashing (the over-read stays within the same object, so
   ASan's redzones don't catch it; confirmed by directly `memset`-filling
   an `MDTREC`'s raw memory with non-null bytes, matching what a
   corrupt `fread()` would produce, and observing pre-fix `GetKey()`
   pull in bytes past `Key`'s 16-byte bound). Fixed: `operator=` now
   `memcpy`s the full fixed size and forces the last byte to `'\0'`
   (also picking up a self-assignment guard, cheap to add and avoids
   `memcpy`'s technically-undefined same-pointer-src/dst case); the
   getters now use `strnlen(buf, BufSize)` and `STRING::Set`/`Cat`'s
   explicit-length overloads instead of relying on an implicit,
   unbounded `strlen`. See `BUGFIX #1`/`BUGFIX #2` in source; regression
   tests in `tests/src/test_mdtrec.cxx` `memset` an `MDTREC`'s raw
   memory the same way a corrupt `fread()` would and assert every
   getter's result stays within its buffer's bound.

Also applied: file-level and per-method doc comments in `mdtrec.hxx`,
including documenting the fixed-buffer/raw-I/O design rationale so a
future reader doesn't mistake it for an oversight.

## src/registry.cxx

`REGISTRY`: a first-child/next-sibling tree of named nodes for
structured config/profile data, addressable by a `STRLIST` path (see
the file-level comment added to `registry.hxx`). Also home to the free
function `parseMetaDefaults()`. No `NULL` usages; no `sprintf` in the
class itself (`GetUniqueKey` wasn't touched — see below).

1. **`operator=` was a non-functional stub that aliased the source's
   subtree instead of copying it** — the body literally printed
   `"WARNING: REGISTRY::operator=() not yet implemented!"`, then did
   `Next = OtherRegistry.Next; Child = OtherRegistry.Child;`: pointer
   assignment, not a copy. Two `REGISTRY` objects would end up owning
   the same `Next`/`Child` nodes; since `~REGISTRY()` recursively
   `delete`s both, destroying either object frees nodes the other still
   references. `clone()` (just above it in the header) already
   implements correct recursive deep-copy semantics for this exact
   `Next`/`Child` shape, so `operator=` now reuses it instead of
   guessing at new logic. Confirmed real with a standalone repro before
   the fix — assign one populated `REGISTRY` into another, then let
   both go out of scope — and confirmed clean after. See `BUGFIX #1` in
   source; regression tests (deep-copy independence and
   self-assignment) in `tests/src/test_registry.cxx`.
2. **`parseMetaDefaults()`'s tokenizer had no bound on its 1024-byte
   stack buffer** — `char token[1024];` was written via
   `*(tokenEnd++) = c;` with nothing capping `tokenEnd` at the buffer's
   end, so any tag name or text run longer than 1024 bytes overflowed
   the stack. Confirmed with a standalone repro (a file with a
   >1024-byte unbroken text run): AddressSanitizer reported a
   `stack-buffer-overflow` at the write. Fixed by capping writes at a
   `tokenLimit` one byte short of the buffer's end (reserving room for
   the `'\0'` terminator written after the loop), silently truncating
   an oversized token the same way `STRING::GetCString` already does
   elsewhere in this tree. See `BUGFIX #2` in source; regression test
   in `tests/src/test_registry.cxx` feeds a 2000-byte text run through
   and confirms it doesn't crash.
3. **`parseMetaDefaults()`'s `STRLIST* data` was read and `delete`'d
   uninitialized, and leaked on every iteration but the last** — `data`
   had no initializer and was only assigned inside the loop when a
   text/data token was found, yet `delete data;` ran unconditionally
   after the loop — undefined behavior on whatever garbage pointer
   value was on the stack for any input with zero data tokens (tags-only
   content, or an unopenable/empty file). Confirmed reachable with a
   standalone repro (a well-formed but data-less `<a></a>` file), though
   it happened not to crash *this* run — consistent with reading an
   uninitialized pointer being UB rather than something ASan's
   heap/stack instrumentation reliably catches, not evidence it's safe.
   Separately, every `data = new STRLIST();` before the last one was
   never freed (`data` was simply overwritten next iteration) —
   confirmed as a real leak under LeakSanitizer with a three-data-token
   input (272 bytes leaked in 8 allocations). Fixed by initializing
   `data = 0` and `delete`-ing the previous value before each
   reassignment (`delete` on a null pointer is a documented no-op, so
   this is correct on the very first assignment too). See `BUGFIX #3`
   in source; regression tests in `tests/src/test_registry.cxx` cover
   both the tags-only case and a multi-data-token case.
4. **`ProfileWrite()` silently ignores both its `FileName` and
   `Position` parameters** — it always walks every direct child of
   `this` regardless of `Position` (unlike `SaveToFile()`'s analogous
   `FindNode(Position)` lookup just above it), and never touches
   `FileName` at all (it writes to the `ostream&` argument instead).
   Not called anywhere in this tree today, so this is left as an
   unresolved-design finding rather than guessed at — it's unclear
   whether the intent was to filter by `Position` (matching
   `SaveToFile`) or something else entirely, and inventing behavior for
   an unused public method isn't a mechanical fix. The parameter names
   were dropped in the definition (values are unused either way) purely
   to compile clean under `-Wunused-parameter`; no behavior changed.
   See `BUGFIX #4` in source.

Also applied: file-level and per-method doc comments in `registry.hxx`.
`GetUniqueKey()`'s `sprintf` (formatting two `INT`s into a 30-byte
buffer, comfortably bounded given `INT`'s range) wasn't modernized —
the function wasn't otherwise touched this turn, per "don't restyle
code you're not otherwise touching."

## src/result.cxx

`RESULT`: one search hit (key/doctype/path/file name, byte span, score,
and — for virtual databases — which `MDT` it came from). See the
file-level comment added to `result.hxx`. Already used `snprintf` in
`GetVKey()`; no `NULL` usages.

1. **Constructor left `DbNum`/`RecordStart`/`RecordEnd`/`Score`/`MyMdt`
   indeterminate** — none of these five primitive/pointer members had
   an in-class initializer or were set in the constructor body (only
   the `DO_HIGHLIGHTING`-gated `HitTable`, not defined anywhere in this
   build, was). Unlike the `STRING` members, which self-initialize to
   empty via `STRING`'s own default constructor, a default-constructed
   `RESULT` used to start with garbage in all five until a caller
   happened to `Set` every one of them explicitly. Fixed by initializing
   all five in the constructor body. See `BUGFIX #1` in source;
   regression test in `tests/src/test_result.cxx` checks a
   default-constructed `RESULT` reads back zero/null for all five.
2. **`operator=` never copied `MyMdt`** — every other field was copied
   field-by-field except the saved `MDT*`, so after `a = b;`, `a`'s
   `GetMdt()` kept returning whatever it already had (indeterminate
   before `BUGFIX #1`, or just stale) instead of `b`'s, silently
   pointing `GetRecordData()`/highlighting at the wrong virtual
   database, or an invalid one. Fixed by adding the missing assignment.
   See `BUGFIX #2` in source; regression test in
   `tests/src/test_result.cxx` confirms a copy's `GetMdt()` matches the
   source's.
3. **`GetRecordData()` narrowed `GPTYPE` (unsigned 32-bit)
   `RecordStart`/`RecordEnd` to local `INT` (signed 32-bit) variables**
   — an offset past `INT_MAX` (~2GB into a large indexed corpus) would
   go negative, breaking both the `fseek()` offset and the computed read
   size. The same `GPTYPE`-narrowed-to-`INT` category already found,
   but not yet fixed pending its own turn, in `FC::Write()`/`Read()`
   (`docs/BUG_CATALOG.md#srcfchxx`). `GetRecordSize()` (just above) was
   already correctly typed (`LONG`); `GetRecordData()` now reuses it
   instead of recomputing narrowed. See `BUGFIX #3` in source;
   regression test in `tests/src/test_result.cxx` confirms a real
   read still round-trips a specific byte range correctly.

### Found but out of scope for this file (deferred, not fixed)

- **Missing copy constructor** — `RESULT` declares `operator=` but no
  copy constructor (already flagged as a `DF`/`FCT`/`ATTRLIST`-shaped
  risk in the comment above `MakeResult()` in
  `tests/src/test_rset.cxx`, written during `rset.hxx`'s turn). Unlike
  those three, though, this isn't currently exploitable in *this*
  tree's actual build: the only member that would be unsafely
  shallow-copied by the compiler-generated copy constructor —
  `PFCT HitTable`, owned and `delete`d in the destructor — only exists
  when `DO_HIGHLIGHTING` is defined, and nothing in the real `Makefile`
  or `make tests`/`make tests-asan` ever defines it (only
  `Makefile.asf`, an alternate/legacy build file not used by this
  cleanup effort, does). `MyMdt`, the only other pointer member, is
  documented as an intentionally-shared, non-owned reference (see the
  new doc comment on `SetMdt()`), so shallow-copying it is correct, not
  a bug. Left unfixed since adding a copy constructor is a header
  change (GENERAL step 4), and — unlike `attrlist.hxx`/`dfdt.hxx`/
  `mdt.hxx` this batch — no standalone repro under this tree's actual
  build flags reproduces a real defect to justify blocking the whole
  file over it.

## src/fprec.cxx

`FPREC`: one entry in `FPT`'s (`src/fpt.hxx`, Order 42, still pending)
open-file table — a file name, its `FILE*`, open mode, and an
LRU-style `Priority`/`Closed` pair `FPT` uses to pick which file to
close when the table is full. See the file-level comment added to
`fprec.hxx`. Constructor already initializes every member (`FilePointer
= 0; Priority = 0; Closed = GDT_FALSE;`); no `NULL`/`sprintf` usages.

1. **`operator=` never copied `Priority` or `Closed`** — only
   `FileName`/`FilePointer`/`OpenMode` were assigned. Confirmed live,
   not just latent: `src/fpt.cxx:158` does `Fprec = Table[z-1];` into a
   freshly default-constructed local (`Closed == GDT_FALSE` from
   `FPREC`'s own constructor), then reads `Fprec.GetClosed()` two lines
   later expecting `Table[z-1]`'s actual value — it silently got the
   default instead every time. Fixed by adding the two missing
   assignments. See `BUGFIX #1` in source; regression test in
   `tests/src/test_fprec.cxx` confirms a copy's `Priority`/`Closed`
   match the source's.

Also applied: file-level and per-method doc comments in `fprec.hxx`.
Added `fprec.cxx` to `TEST_ENGINE_SRCS` (it wasn't linked into the test
binary before this turn).

## src/iresult.cxx

No bugs found. `IRESULT` (an internal-scoring counterpart to `RESULT`,
see the file-level comment added to `iresult.hxx`) already initializes
every member in its constructor — including `Mdt`, unlike `RESULT`'s
constructor this same batch (`docs/BUG_CATALOG.md#srcresultcxx`,
`BUGFIX #1`) — and `operator=` already copies every field, including
`Mdt` (again unlike `RESULT`'s `operator=` before this batch's
`BUGFIX #2`). One `NULL` usage modernized: `Mdt = (MDT*)NULL;` →
`Mdt = nullptr;`. No `sprintf` usages. Added file-level and per-method
doc comments plus `tests/src/test_iresult.cxx`.

## src/opobj.cxx

`OPOBJ`: the base class every `OPSTACK` node (leaf operand or operator)
derives from — see the file-level comment added to `opobj.hxx`. No
`NULL`/`sprintf` usages.

1. **Constructor left `Next` indeterminate** — no in-class initializer,
   not set in the constructor body. `Next` is private with
   `friend class OPSTACK;` the only accessor, and every current
   `OPSTACK::Push()` call path already calls `SetNext()` immediately
   after constructing/duplicating a node, before `Next` is ever read
   back — so this wasn't reachable as a live bug through `OPSTACK`'s
   existing push/pop/`Reverse()` logic (confirmed by reading
   `src/opstack.cxx`'s `Push`/`Pop`/`Reverse`, not by a repro, since
   there's no way to trigger the gap from outside). Fixed anyway, the
   same way as the identical finding in `RESULT`'s constructor this
   batch (`docs/BUG_CATALOG.md#srcresultcxx`, `BUGFIX #1`): leaving a
   raw pointer indeterminate rather than null is a trap for whatever
   future caller doesn't happen to follow `OPSTACK`'s exact
   set-before-read discipline. See `BUGFIX #1` in source.

Also applied: a file-level doc comment on `OPOBJ` explaining its role
and that most methods defaulting to a no-op/zero return is deliberate,
not an oversight. Left the `-Wunused-parameter` warnings on those
default virtual bodies (`SetAttributes`, `GetAttributes`, `SetTerm`,
etc.) as-is, consistent with how the identical pattern was already
left in place across `idbobj.hxx` (Order 18, already `done`, ~90 such
warnings) rather than inconsistently cleaning up only this file's ~10;
none of them are newly introduced by this turn. Added
`tests/src/test_opobj.cxx` via a minimal concrete subclass (`OPOBJ` is
abstract) covering the default virtuals' documented no-op behavior;
`Next`'s initialization isn't unit-testable from outside `OPSTACK` for
the same friend-access reason it wasn't reachable as a bug.

## src/reclist.hxx

Reprocessed via `/reprocess-blocked` after being blocked at GENERAL step
4 (see `docs/AUTOPILOT_LOG.md#srcreclisthxx`); the user chose deep-copy
semantics over making `RECLIST` non-copyable.

1. **Header not self-contained** — same defect as `src/fc.hxx`
   `BUGFIX #1` and `src/record.hxx`/`src/irset.hxx`'s own instances of
   it: `reclist.hxx` declared `RECORD`/`PRECORD`/`INT`-typed members and
   methods with its `#include`s commented out. Confirmed real by
   compiling `reclist.hxx` as the sole `#include` in a translation
   unit: it failed with 8 errors. Fixed by restoring `defs.hxx` (for
   `INT`) and `record.hxx` (for `RECORD`/`PRECORD`); `string.hxx` was
   left out of the restored set — nothing in this header uses `STRING`
   directly, and `record.hxx` already brings it in transitively for
   anything that does. Verified fixed by recompiling the same
   standalone reproduction, which now succeeds with zero warnings under
   `-Wall -Wextra`. See `BUGFIX #1` in source.
2. **No copy constructor or `operator=`** — `RECLIST` owns a
   heap-allocated `PRECORD Table` array (`new RECORD[...]` in the
   constructor/`Resize`, `delete [] Table` in the destructor/`Resize`)
   but declared neither, so the compiler-generated ones did a shallow
   pointer copy. Confirmed with a standalone repro: copy-constructing a
   second `RECLIST` and destroying both triggered a heap-use-after-free
   in `RECLIST::~RECLIST()` (a second `delete []` on the already-freed
   `Table`) under AddressSanitizer. `RECLIST` is currently dormant in
   the live tree (its only two references, in `src/Iindex.cxx` and
   `src/idb.hxx`, are both commented out), so this wasn't an active
   crash, but a real latent defect. Fixed by adding
   `RECLIST(const RECLIST&)` and `operator=(const RECLIST&)` that
   deep-copy `Table` (sized to the source's `MaxEntries`),
   `TotalEntries`, and `MaxEntries`; `operator=` guards against
   self-assignment before freeing the old `Table`, deliberately not
   repeating the missing-guard bug already found (though not yet fixed)
   in `ATTRLIST`'s/`DFDT`'s hand-written `operator=`'s this same batch.
   Verified fixed by turning the original standalone repro into a
   permanent regression test, which now passes clean under
   `make tests-asan`. See `BUGFIX #2` in source; regression test in
   `tests/src/test_reclist.cxx`.

Also applied: a file-level doc comment on `RECLIST` explaining its role
and documenting `GetEntry()`'s 1-based indexing as this file's own
convention, independent of `STRING`'s, per GENERAL step 7. No
`NULL`/`sprintf` usages to modernize. The new `BUGFIX #2` bodies
necessarily live in `src/reclist.cxx`, whose own full turn (Order 164)
is still `pending` — only that minimal, scoped addition was made there;
nothing else in that file was touched, and it was not marked
`processed`. `src/reclist.cxx` was added to `TEST_ENGINE_SRCS` in the
Makefile so `tests/src/test_reclist.cxx` can link against it.

## src/vlist.hxx

Reprocessed via `/reprocess-blocked` after being blocked at GENERAL step
4 (see `docs/AUTOPILOT_LOG.md#srcvlisthxx`); the user chose splice-as-
a-new-circle semantics over making `VLIST` non-copyable.

1. **No copy constructor and no working `operator=`** — `VLIST` (the
   doubly linked circular list base class) declared neither; the only
   trace of one was a commented-out, pure-virtual sketch
   (`virtual VLIST& operator=(const VLIST&) = 0;`) that never compiled.
   The compiler-generated copy operations shallow-copied the raw
   `Next`/`Prev` pointers instead of properly relinking the copy into
   (or out of) a circle. Confirmed with a standalone repro:
   copy-constructing a node from a one-node circle and destroying both
   triggered a heap-use-after-free/double-free cascade in `~VLIST()`
   under AddressSanitizer. Unlike `reclist.hxx`, this was **not
   dormant**: `FCT` (Order 2, already `done`) and `STRLIST` (Order 29,
   still `pending`) both derive from `VLIST` without declaring their
   own copy constructor, so copy-*constructing* either subclass hit
   this transitively today (their hand-written `operator=`'s, e.g.
   `FCT::operator=`, already avoid the bug independently by rebuilding
   via `AddNode()` rather than copying pointers — only construction was
   exposed). Fixed by adding `VLIST(const VLIST&)` and
   `operator=(const VLIST&)` where a copied/assigned node becomes the
   sole member of its own new one-node circle — the source's own
   `Next`/`Prev` are deliberately never read, since a literal deep copy
   doesn't have a coherent meaning for a node embedded in a specific
   circle. `operator=` additionally detaches `this` from whatever
   circle it currently belongs to before going solo, so its old
   neighbors are left correctly linked rather than dangling; this makes
   self-assignment automatically safe too, since neither function ever
   reads the source's members. Not made virtual, unlike the abandoned
   sketch — nothing in the tree assigns/copy-constructs through a
   `VLIST&`/`VLIST*` today. Verified fixed by turning the original
   standalone repro into a permanent regression test, plus targeted
   tests for the detach-on-assign and self-assignment behavior; all
   pass clean under `make tests-asan`. See `BUGFIX #1` in source;
   regression tests in `tests/src/test_vlist.cxx`.

Found but out of scope — a general destructor hazard, not new in this
turn and not reachable through the tree's current usage: `~VLIST()`
unconditionally cascades `delete Next` through the rest of the circle
on the assumption that every attached node is heap-allocated and that
exactly one entry point into a circle is ever deliberately destroyed.
That assumption holds for every live call site found (`FCT`/`STRLIST`
anchors are stack- or member-allocated, e.g. `FCT fct;` in
`src/index.cxx`, but every node they attach via `AddEntry`/`AddNode` is
heap-allocated via `new`, exactly the pattern the cascade handles
correctly). It breaks only if *multiple* stack-allocated nodes
belonging to the same circle are each destroyed independently — no
code in this tree does that, but nothing stops future code from doing
so, and the resulting failure (an invalid `delete` on a stack address)
is a hard crash, not a subtle one. Confirmed via the test-writing
process for this turn: an earlier draft of `tests/src/test_vlist.cxx`
built multi-node circles entirely out of stack-allocated `TestNode`
locals and crashed with "double free or corruption" on scope exit —
unrelated to the `BUGFIX #1` fix above, reproducible on `vlist.cxx` as
it stood before this turn's changes too. Worth a closer look, and worth
keeping in mind, when `vlist.cxx`'s own turn (Order 180) comes up —
possibly hardening `~VLIST()` to tolerate this, or just documenting the
heap-only-attachment contract explicitly on the class.

Also documented: a file-level doc comment on `VLIST` explaining its
role (a payload-free circular-list base class) per GENERAL step 7. No
`NULL`/`sprintf` usages to modernize; `vlist.hxx`'s own `#include`s were
already complete (verified by compiling it standalone), unlike
`reclist.hxx`'s analogous defect above.

## src/attrlist.hxx

Reprocessed via `/reprocess-blocked` after being blocked at GENERAL step
4 (see `docs/AUTOPILOT_LOG.md#srcattrlisthxx`); the user chose deep-copy
semantics over making `ATTRLIST` non-copyable, since `ATTRLIST` is a
live member of both `OPERAND` and `DFD` and non-copyable would have
required auditing every copy site on both.

1. **No copy constructor** — `ATTRLIST` owns a heap-allocated
   `PATTR Table` array and already has a correct `operator=`, but
   declared no copy constructor, so the compiler-generated one did a
   shallow pointer copy. Confirmed with a standalone repro:
   copy-constructing a second `ATTRLIST` and destroying both triggered
   a heap-use-after-free in `~ATTRLIST()`. Fixed by adding
   `ATTRLIST(const ATTRLIST&)` that deep-copies `Table` (sized to the
   source's `MaxEntries`), `TotalEntries`, and `MaxEntries`, mirroring
   `operator=`'s own logic. See `BUGFIX #1` in source; regression test
   in `tests/src/test_attrlist.cxx`.
2. **`operator=` had no self-assignment guard** — `delete [] Table;
   Init();` ran before `OtherAttrlist.GetTotalEntries()` was read; for
   `x = x;` that's the same object `Init()` had just reset to 0
   entries, so the rebuild loop below copied nothing back, silently
   emptying the list. Same shape of bug as the one already found (and
   fixed) in `STRLIST`'s `operator=` (`docs/BUG_CATALOG.md#srcstrlistcxx`,
   `BUGFIX #1`) and previously flagged here as "found, not fixed" at
   `sterm.hxx`'s turn. Fixed with a `this == &OtherAttrlist` guard. See
   `BUGFIX #2` in source; regression test in `tests/src/test_attrlist.cxx`.
3. **`Init()` allocated `Table` one element short of `MaxEntries`** —
   `Table = new ATTR[7]` while `MaxEntries` was set to `8`, so
   `AddEntry`'s `TotalEntries == MaxEntries` bounds check let the 8th
   entry write to `Table[7]`, one past the end of the 7-slot
   allocation — a real heap buffer overflow on every `ATTRLIST` that
   ever grows past 7 entries, not a latent/dormant one. Confirmed real:
   a loop adding 20 plain entries crashed with SIGSEGV before this fix
   (found via this turn's own test, not a pre-existing repro in
   `docs/AUTOPILOT_LOG.md` — this bug wasn't part of what blocked the
   file). Fixed by computing the allocation size from `MaxEntries`
   itself (`new ATTR[MaxEntries]`) instead of duplicating the number as
   a separate literal, so the two can't drift apart again. See
   `BUGFIX #3` in source; covered incidentally by the "grows past
   initial capacity" test in `tests/src/test_attrlist.cxx`, which
   crashed before this fix and passes clean now.

Also applied: a file-level doc comment on `ATTRLIST` per GENERAL step
7. No `NULL`/`sprintf` usages to modernize; `attrlist.hxx`'s own
`#include`s were already complete (verified by compiling it standalone).

## src/dfdt.hxx

Reprocessed via `/reprocess-blocked` after being blocked at GENERAL step
4 (see `docs/AUTOPILOT_LOG.md#srcdfdthxx`); the user chose deep-copy
semantics over making `DFDT` non-copyable.

1. **No copy constructor** — `DFDT` owns a heap-allocated `PDFD Table`
   array and already has a correct `operator=`, but declared no copy
   constructor, so the compiler-generated one did a shallow pointer
   copy. Confirmed with a standalone repro: copy-constructing a second
   `DFDT` and destroying both triggered a heap-use-after-free in
   `~DFDT()`. The third instance of this exact pattern this batch
   (after `reclist.hxx` and `attrlist.hxx`); no confirmed live
   copy-construction call site was found, so this one is latent, not
   actively reachable. Fixed by adding `DFDT(const DFDT&)` that
   deep-copies `Table` (sized to the source's `MaxEntries`),
   `TotalEntries`, `MaxEntries`, and `Changed`, mirroring `operator=`'s
   own logic. See `BUGFIX #1` in source; regression test in
   `tests/src/test_dfdt.cxx`.
2. **`operator=` had no self-assignment guard** — identical shape to
   `ATTRLIST`'s already-fixed instance of this bug this batch
   (`docs/BUG_CATALOG.md#srcattrlisthxx`, `BUGFIX #2`): `delete []
   Table; Initialize();` ran before `OtherDfdt.GetTotalEntries()` was
   read, so `x = x;` silently emptied the table. Fixed with a `this ==
   &OtherDfdt` guard. See `BUGFIX #2` in source; regression test in
   `tests/src/test_dfdt.cxx`.
3. **`GetDfdRecord`'s not-found path was dead code** — on a failed
   lookup it ran `DfdRecord=(PDFD)NULL;`, which assigns to the local
   copy of the by-value pointer *parameter*, not to `*DfdRecord` — the
   caller can never observe this write, so the line did nothing.
   Checked the sole live call site (`src/idb.cxx:441`): it never checks
   for a null/sentinel result and simply relies on `*DfdRecord` being
   left as whatever the caller passed in, which is exactly what
   already happens once the no-op line is understood as such — so this
   is a dead-code cleanup, not a behavior change. Removed the line; the
   function's contract (leave `*DfdRecord` untouched if `FieldName`
   isn't found, same convention as `GetEntry()`) is now documented on
   the declaration in `dfdt.hxx`. See `BUGFIX #3` in source and test in
   `tests/src/test_dfdt.cxx`.

Also applied: `LoadTable`'s five `strtok((CHR*)NULL,"\n")` calls
modernized to `strtok(nullptr,"\n")`, per this file's own
`docs/AUTOPILOT_LOG.md` entry flagging it as ready for the next
reprocessing. A file-level doc comment on `DFDT` per GENERAL step 7.
`dfdt.hxx`'s own `#include`s were already complete (verified by
compiling it standalone).

## src/mdt.hxx

Reprocessed via `/reprocess-blocked` after being blocked at GENERAL step
4 (see `docs/AUTOPILOT_LOG.md#srcmdthxx`); the user chose non-copyable
over deep-copy, since nothing in the tree copies an `MDT` by value
today and there's no well-defined answer for what a copy of an open
`FILE*` should mean.

1. **No copy constructor and no `operator=` at all** — unlike every
   other class blocked this batch, `MDT` declared neither, not even a
   hand-written (if buggy) one. It owns two heap-allocated arrays
   (`KeyIndex`, `GpIndex`) *and* a raw `FILE* MdtFp`, so the
   compiler-generated copy operations did a member-wise shallow copy of
   all three. Confirmed with a standalone repro (construct against a
   real temp file stem, add one entry, copy-construct a second,
   destroy both): AddressSanitizer reported a heap-use-after-free one
   level inside `~MDT()`'s call to `FlushMDTIndexes()` →
   `SortGpIndex()` → `qsort()` on the already-freed `GpIndex`; the
   shared `MdtFp` double-`fclose()` is real by the same mechanism but
   wasn't hit by this specific repro. No confirmed live
   copy-construction call site exists today, so this was latent. Fixed
   by declaring `MDT(const MDT&) = delete;` and
   `MDT& operator=(const MDT&) = delete;` — no body to write, so no
   `BUGFIX #1` comment in `.cxx`, just the declarations in `mdt.hxx`.
   Compile-time regression test (`std::is_copy_constructible`/
   `is_copy_assignable`) in `tests/src/test_mdt.cxx`.
2. **Three `qsort`/`bsearch` comparators used unsigned subtraction** —
   `MdtCompareKeysByIndex`, `MdtCompareGpByIndex`, and
   `MdtCompareGpStarts` computed their result as a plain subtraction of
   `GPTYPE` (`UINT4`, unsigned) fields narrowed to `int` — the classic
   unsigned-subtraction-in-a-comparator bug: for a pair far enough
   apart, the unsigned wraparound produces the wrong sign once
   narrowed, misordering the sort/search. Not exercised by this
   batch's test data (small `Index` values), but `GpStart`/`GpEnd` are
   byte offsets that can realistically span the affected range in a
   large database. Fixed with explicit `<`/`>` comparisons instead of
   subtraction. See `BUGFIX #2` in source.
3. **`GetUniqueKey` still used `sprintf`** — modernized to
   `snprintf(s, sizeof(s), ...)`, per this file's own
   `docs/AUTOPILOT_LOG.md` entry flagging it as ready for the next
   reprocessing (`y`/`x` are `INT`, so the worst case comfortably fits
   the existing `CHR s[30]` buffer — not a live overflow, just
   modernization). See `BUGFIX #3` in source.
4. **`GetEntry`'s not-found path used `memset` on a non-trivial
   class** — `memset(MdtrecPtr, 0, sizeof(MDTREC))` is safe in practice
   (`MDTREC` holds only fixed-size `CHR` arrays and `GPTYPE`/`CHR`
   scalars, no owned pointers), but `MDTREC`'s user-declared
   `operator=` makes it non-trivial from the type system's point of
   view, so GCC flags the `memset` under `-Wclass-memaccess` — a
   pre-existing warning, not introduced this turn, that GENERAL step 9
   requires resolving before the file compiles clean. Fixed by using
   the class's own default constructor (`*MdtrecPtr = MDTREC();`),
   which already zero-initializes the same fields (confirmed by reading
   `MDTREC::MDTREC()`) — equivalent result, warning-free. See
   `BUGFIX #4` in source.
5. **`Dump()`'s loop variable was signed, compared against unsigned
   `TotalEntries`** — another pre-existing `-Wall`/`-Wextra` warning
   (`-Wsign-compare`), not introduced this turn. Changed `INT x;` to
   `SIZE_T x;`, matching `TotalEntries`'s type (and `GetEntry`'s own
   parameter type, removing an implicit conversion at the call site
   too). See `BUGFIX #5` in source.

Also applied: a file-level doc comment on `MDT` per GENERAL step 7.
`mdt.hxx`'s own `#include`s were already complete (verified by
compiling it standalone). `MDT` has no default constructor — it always
opens/creates real on-disk `.mdt`/`.mdg`/`.mdk` files via a file stem —
so `tests/src/test_mdt.cxx` uses a `TempMdt` fixture, the same pattern
already established in `tests/src/test_filemap.cxx`.

## src/fpt.hxx

Reprocessed via `/reprocess-blocked` after being blocked at GENERAL step
4 (see `docs/AUTOPILOT_LOG.md#srcfpthxx`); the user chose non-copyable,
the same choice made for `MDT` this batch and for the same reason —
copying a table of open file handles has no single obviously-correct
meaning.

1. **No copy constructor and no `operator=` at all** — the same
   "no custom copy semantics whatsoever" shape as `MDT` this batch.
   `FPT` owns a heap-allocated `FPREC* Table` array where each entry
   caches a live `FILE*`, so the compiler-generated copy operations did
   a member-wise shallow copy. Confirmed with a standalone repro
   (construct, open one real file through it, copy-construct a second,
   destroy both): AddressSanitizer reported a heap-use-after-free
   inside `FPT::CloseAll()` during destruction, from the shared
   `Table` pointer being `delete []`'d twice. Unlike `MDT`, this one
   has a concrete live call site today: `IDB::MainFpt`
   (`src/idb.hxx:224`) is a plain, non-pointer `FPT` member, so a
   future copy of `IDB` (still pending) without its own copy semantics
   would hit this transitively. Fixed by declaring
   `FPT(const FPT&) = delete;` and `FPT& operator=(const FPT&) = delete;`
   — no body to write, so no `BUGFIX #1` comment in `.cxx`, just the
   declarations in `fpt.hxx`. Compile-time regression test
   (`std::is_copy_constructible`/`is_copy_assignable`) in
   `tests/src/test_fpt.cxx`.
2. **`ffopen`'s cache-hit branch read `Closed` and never used it** — a
   pre-existing `-Wunused-but-set-variable` warning, not introduced
   this turn, that GENERAL step 9 requires resolving before the file
   compiles clean. Traced every branch before removing it (not just
   silencing the warning): the "w"/"a" branches unconditionally
   `fclose()` and reopen; the "r" branch always reuses the cached
   `Fp`, which is safe regardless of `Closed`'s value because
   `ffclose()` never physically closes an entry that's still reachable
   via `Lookup()` — only `CloseAll()` (which also zeroes
   `TotalEntries`, hiding every slot from `Lookup()`) or an
   eviction/mode-change (which replaces the slot's `FilePointer`
   before anyone could reuse a stale one) actually call `fclose()` on
   a live slot. So the read was genuinely dead, not a missing check —
   removed rather than "fixed" into using it. See `BUGFIX #2` in
   source.

Also applied: a file-level doc comment on `FPT` per GENERAL step 7,
including how `ffclose()`/`CloseAll()`/eviction divide up when a
handle is actually physically closed (the context that made `BUGFIX
#2` provable rather than speculative). `fpt.hxx`'s own `#include`s
were already complete (verified by compiling it standalone).
`src/fpt.cxx` was added to `TEST_ENGINE_SRCS` in the Makefile so
`tests/src/test_fpt.cxx` can link against it.

## src/nfield.cxx

`NUMERICFLD`: one numeric-field entry, a byte offset paired with a
numeric value (see the file-level comment added to `nfield.hxx`). No
`operator=` declared — the compiler-generated one is already correct,
since both members are plain primitives with no owned resources. No
`NULL`/`sprintf` usages.

1. **Constructor left `GlobalStart`/`NumericValue` indeterminate** —
   `NUMERICFLD::NUMERICFLD() {}` had an empty body, no in-class
   initializers on either member. Confirmed live, not just latent:
   `src/nlist.cxx:62/74` does `table = new NUMERICFLD[50*Ncoords];`,
   array-default-constructing every slot, and code elsewhere in that
   same file reads `table[x].GetGlobalStart()`/`GetNumericValue()` (via
   `qsort` comparators and direct indexing) before every slot is
   necessarily filled. Fixed by initializing both to `0`/`0.0`. See
   `BUGFIX #1` in source; regression test in
   `tests/src/test_nfield.cxx` checks both a single default-constructed
   instance and every slot of a default-constructed array.

Also applied: a file-level doc comment on `nfield.hxx`. Added
`nfield.cxx` to `TEST_ENGINE_SRCS` in the Makefile (it wasn't linked
into the test binary before this turn).

## src/date.cxx

`SRCH_DATE`/`DATERANGE`: a date stored as YYYY/YYYYMM/YYYYMMDD in a
single `DOUBLE`, plus a precision tag, and a `[start,end]` pair of
them. See the file-level comment added to `date.hxx`. One `NULL`
modernized to `nullptr` (`time((time_t *)NULL)` → `time(nullptr)`).

1. **`SRCH_DATE`'s default constructor left `d_date`/`d_prec`
   indeterminate** — empty body, no in-class initializers. Since
   `DATERANGE`'s own default constructor is also empty and relies on
   `SRCH_DATE`'s default state for `d_start`/`d_end`, this affected both
   classes. Fixed by initializing to `DATE_ERROR`/`BAD_DATE`, mirroring
   the sentinel this same file already uses elsewhere for "no date" (see
   the not-a-range fallback in `DATERANGE`'s parsing constructors). See
   `BUGFIX #1` in source; regression tests in `tests/src/test_date.cxx`
   check both a default-constructed `SRCH_DATE` and a default-constructed
   `DATERANGE` read as invalid.
2. **`GetTodaysDate()`'s error branch fell through instead of
   returning** — on a `strftime()` failure, it set `d_date = -1.0;
   d_prec = BAD_DATE;` but then unconditionally continued into
   `d_date = atof(Hold); SetPrecision();`, silently overwriting the
   error state. Worse, a `strftime()` failure that returns 0 leaves
   `Hold` (a local, uninitialized `CHR` buffer) untouched, so the
   overwrite would have called `atof()` on uninitialized stack memory.
   Not reachable with today's fixed `"%Y%m%d"` format and a buffer sized
   for it (confirmed: a normal call succeeds and returns exactly 8,
   matching `ConvertLen`), so this wasn't an active crash, but a real
   defect that would surface the moment either changed. Fixed by adding
   the missing `return;`. See `BUGFIX #2` in source.
3. **`DATERANGE::Contains()` had its `BEFORE`/`AFTER` comparison
   backwards** — `DateCompare(TestDate)` compares *this* to `TestDate`,
   so `d_start.DateCompare(TestDate) == BEFORE` means `d_start` is
   before `TestDate` — exactly the condition for `TestDate` validly
   being past the range's start, not a reason to reject it. The old
   condition (`d_start ... BEFORE || d_end ... AFTER`) was true for
   `TestDate > d_start` OR `TestDate < d_end`, which for any normal
   range (`d_start <= d_end`) covers nearly every possible `TestDate` —
   confirmed by tracing a concrete example (range `[2020,2025]`,
   `TestDate` `2022`, squarely inside): the old code returned
   `GDT_FALSE`. Fixed by swapping to the correct rejection condition:
   `d_start` is *after* `TestDate`, or `d_end` is *before* it. No caller
   of `DATERANGE::Contains()` was found anywhere in this tree, so this
   wasn't an active-search regression, but a real, confirmed-wrong
   defect in a public method. See `BUGFIX #3` in source; regression
   tests in `tests/src/test_date.cxx` cover inside/before/after/
   boundary cases, verified against a standalone repro before the fix.

Also applied: file-level and per-method doc comments in `date.hxx`.
Added `date.cxx` to `TEST_ENGINE_SRCS` in the Makefile (it wasn't
linked into the test binary before this turn).

## src/intfield.cxx

`INTERVALFLD`: one numeric-interval entry, a byte offset paired with a
`[StartValue, EndValue]` range, derived from `NUMERICFLD`. See the
file-level comment added to `intfield.hxx`. No bugs fixable without a
header change were found in the `.cxx` body itself — the default
constructor already initializes every one of `INTERVALFLD`'s own
members, and the copy constructor/`operator=` bodies correctly copy
them. No `NULL`/`sprintf` usages.

### Found but out of scope for this file (deferred, not fixed)

Both require a header change (GENERAL step 4), and neither is confirmed
reachable in the live tree, the same bar used for `result.hxx`'s
deferred missing-copy-constructor finding this batch:

- **`GlobalStart`/`GetGlobalStart()`/`SetGlobalStart()` shadow
  `NUMERICFLD`'s own same-named member and methods** instead of reusing
  the inherited ones — `INTERVALFLD` ends up carrying two separate
  `GlobalStart` fields (its own, actually used, and `NUMERICFLD`'s,
  always left at whatever `NUMERICFLD`'s own default/setters leave it).
  Only observable if something calls `Get`/`SetGlobalStart` through a
  `NUMERICFLD&`/`NUMERICFLD*` referring to an `INTERVALFLD` (non-virtual
  dispatch would then resolve to the base's own hidden copy) — no such
  call site was found anywhere in the tree. The natural fix (drop the
  redundant member/methods here, rely on the inherited ones) shrinks the
  object layout and is a real header change, so left for a human
  decision rather than guessed at.
- **`operator=` has a non-standard signature** —
  `INTERVALFLD operator=(INTERVALFLD& OtherField);` returns by value
  (a full copy of `*this`, not the usual reference) and takes a
  non-const reference, unlike every other `operator=` in this tree. Its
  body is a correct field-by-field copy, but the signature means it
  can't be chained (`a = b = c;`) or assigned from a temporary/rvalue —
  both would fail to compile, since a prvalue can't bind to a non-const
  lvalue reference parameter. Not called anywhere in the tree today
  (confirmed by search), so this is a latent API footgun, not an active
  bug; the standard-idiom fix
  (`INTERVALFLD& operator=(const INTERVALFLD&)`) is a header change, so
  left for a human decision.

Also applied: a file-level doc comment on `INTERVALFLD` documenting
both findings above inline, so a future reader (or whoever resolves
them) doesn't have to rediscover them. Added `intfield.cxx` to
`TEST_ENGINE_SRCS` in the Makefile (it wasn't linked into the test
binary before this turn).

## src/soundex.cxx

`SoundexEncode`: the classic Soundex name-matching algorithm, called
from `src/index.cxx` (still pending). No `NULL`/`sprintf` usages.

1. **Wrong Soundex code for two classic edge cases** — the original
   two-pass approach (strip every `'0'` first, then collapse adjacent
   duplicate digits) gets both wrong: (1) it never compares the kept
   first letter's own digit against the next letter's, so a name like
   "Pfister" (P and F share digit 1) kept both instead of collapsing
   them (`"P123"` instead of the correct `"P236"`); (2) stripping zeros
   *before* deduplicating merges same-digit letters that were
   originally separated by a vowel into a false adjacency, undercounting
   them (`"Honeyman"` → `"H500"` instead of `"H555"`; `"Tymczak"` →
   `"T520"` instead of `"T522"`). Confirmed against standard reference
   Soundex test vectors via a standalone repro before the fix — these
   aren't edge cases invented for this catalog entry, they're the
   textbook examples used to test Soundex implementations precisely
   because of this failure mode. `"Robert"`/`"Rupert"` (no first-letter
   collision) already matched and still do. Fixed by replacing the
   two-pass strip-then-collapse with a single pass that tracks the
   *previous letter's own digit* (seeded from the first letter's digit,
   even though the first letter itself is kept literally) and only
   appends a new digit when it's non-zero and differs from that running
   previous digit — the textbook algorithm, applied uniformly. The
   letter-to-digit `switch` itself was unchanged, just factored into a
   small `SoundexDigit()` helper so it could be reused for the first
   letter's own digit too. See `BUGFIX #1` in source; regression tests
   in `tests/src/test_soundex.cxx` cover all five vectors above plus a
   truncation case (`"Washington"` → `"W252"`).
2. **Empty input produced a null-byte-containing result instead of an
   empty string** — `s2 += s1.GetChr(1);` on an empty `EnglishWord`
   read `GetChr(1)`'s documented out-of-range return value (`0`, a null
   byte — not the character `'0'`), which then survived the old
   zero-stripping pass (a null byte `!= '0'`) and got padded out to 4
   characters. Fixed with an explicit empty-input check returning `""`.
   See `BUGFIX #2` in source; regression test in
   `tests/src/test_soundex.cxx`.

Also applied: a doc comment on `SoundexEncode()`'s declaration. Added
`soundex.cxx` to `TEST_ENGINE_SRCS` in the Makefile (it wasn't linked
into the test binary before this turn).
