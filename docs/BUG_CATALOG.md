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

## doctype/doctype.cxx

`DOCTYPE`: base class for every document-type parser (see the
file-level comment added to `doctype.hxx`). Not abstract — every
virtual has a default body — so it's directly testable without a
subclass. No `NULL`/`sprintf` usages (the one `'\0'` is a character
literal, not the macro).

1. **`ParseWords()` called `exit(1)` on GP-buffer overflow, aborting
   the entire `Iindex` process** — instead of returning to its caller.
   Confirmed this was live, reachable, and defeating an existing
   recovery path: `INDEX::BuildGpList()` (`src/index.cxx`, Order 54,
   still pending) calls this function and already checks
   `if (GpListSize == -1) { Break = GDT_TRUE; break; }` on the result —
   `GPTYPE` is unsigned, so that comparison means the caller was always
   prepared to receive a `(GPTYPE)-1` sentinel and flush/stop cleanly
   whenever a single document has more matched terms than fit in the
   currently allocated GP buffer, but the `exit(1)` here made that path
   unreachable, killing the whole indexing run (losing all progress)
   instead. Fixed by returning `(GPTYPE)-1` in place of the `exit(1)`.
   See `BUGFIX #1` in source; regression test in
   `tests/doctype/test_doctype.cxx` triggers the overflow with a
   1-entry `GpBuffer` against 4 matching words and confirms the
   sentinel comes back (and the test process doesn't exit).
2. **`Present()` had a dead `FieldName` local and an assigned-but-
   never-checked `Status`** — leftovers from the commented-out older
   implementation directly below it, which *did* check `Status` and
   explicitly cleared the buffer on failure
   (`if (Status) ... else *StringBufferPtr = "";`). The current
   function already resets `*StringBufferPtr = "";` unconditionally at
   the top, so the not-found behavior is unchanged; only the dead
   variables (and the resulting `-Wunused-but-set-variable` warning)
   were removed. See `BUGFIX #2` in source; regression test in
   `tests/doctype/test_doctype.cxx` confirms the not-found path still
   returns an empty string.

Also applied: file-level and per-method doc comments on `doctype.hxx`,
including making `ReplaceWithSpace()`'s `data[length] = '\0'`
buffer-capacity contract explicit (it writes one byte past `length`,
by design — confirmed safe for its one real caller,
`src/index.cxx:581`, which always reserves that byte, but wasn't
documented anywhere before this). Left the many named-but-unused
parameters on `doctype.hxx`'s no-op virtual default bodies alone,
matching the same pattern already accepted on `idbobj.hxx`/`opobj.hxx`
(both already `done`) — removing the names would hurt
self-documentation of the interface each override implements, and
wrapping ~20 trivial one-line stubs in `(void)` casts is exactly the
kind of restyling GENERAL step 6 says not to do to code that isn't
otherwise being touched. `GetMetadata()`'s unused `mdType` parameter
was left for the same reason: the base implementation is a generic
default meant to be overridden per format (`doctype/html.cxx` already
does), not a bug.

Also added this turn: `doctype/` support in the test build.
`TEST_ENGINE_DOCTYPE_SRCS` (mirroring the existing `TEST_ENGINE_CGI_SRCS`
pattern for `Isearch-cgi/`) and matching `tests/obj/doctype-%.o`/
`tests/obj-asan/doctype-%.o` pattern rules were added to the Makefile —
`doctype/doctype.hxx` is the first file in that directory to reach its
own turn, so no prior turn had needed this. Every later `doctype/*.cxx`
turn should add itself to `TEST_ENGINE_DOCTYPE_SRCS` rather than
inventing a new mechanism. The test file also reuses (as its own local
copy, matching how `tests/src/test_filemap.cxx` already does this) a
minimal `TESTIDBOBJ` implementing `IDBOBJ`'s 3 pure virtuals
(`DfdtAddEntry`/`IsStopWord`/`ParseWords`), extended with a
configurable stop-word list and a configurable `GetFieldData()` so
`ParseWords()`/`Present()`/`GetMetadata()` could be tested against
controlled data instead of just linking.

## src/index.cxx

`class INDEX` (declared in `src/index.hxx`) owns and searches one
on-disk inverted index. Its implementation is split across five
translation units — `src/index.cxx` (this turn's target),
`src/numsearch.cxx`, `src/datesearch.cxx`, `src/geosearch.cxx`, and
`src/multiterm.cxx` — all still `pending` in `docs/PROCESSING_STATUS.md`
except this one; only `index.cxx`/`index.hxx` were reviewed line-by-line
this turn, though all five now had to be added to `TEST_ENGINE_SRCS`
(along with `src/tokengen.cxx`, `src/thesaurus.cxx`, `src/nlist.cxx`,
`src/squery.cxx`, `src/mergeunit.cxx`, `src/intlist.cxx`) just to link
`INDEX` at all — see BUILD note below.

1. **`INDEX::IsStopWord()` was permanently disabled** — the function
   body opened with `return 0; // added for testing`, before the real
   binary-search lookup against `stoplist[]` (`src/sw.hxx`) ever ran.
   `IDB::IsStopWord()` (`src/idb.cxx:1050`) delegates directly to
   `MainIndex->IsStopWord()`, and every `DOCTYPE` parser (`doctype.cxx`,
   `usmarc.cxx`, `taglist.cxx`) calls `Db->IsStopWord()` while
   tokenizing real documents — so this wasn't a latent/theoretical bug,
   it silently disabled stop-word filtering for every document ever
   indexed through the real `IDB`, bloating every index with "the",
   "a", "an", "of", etc. Fixed by deleting the early return and
   restoring the lookup; `BUGFIX #7` in source. Covered by two new
   tests (`INDEX::IsStopWord recognizes stop words...` /
   `...returns 0 for ordinary words`) that would have failed against
   the old body.

2. **`INDEX::CollapseIndexFiles()` read 2 elements of `MERGEUNIT A[2]`
   out of bounds** — the final merge-loop iterated `for(++k;
   k<IndexNum; k++)`, using the *whole index's* sub-index count
   (`IndexNum`, a member field that can be much larger than 2) as the
   bound for a stack array declared `MERGEUNIT A[2]`, clearly
   copy-pasted from the analogous loop in `MergeIndexFiles()` (where
   `A` really is sized `IndexNum`). Whenever `IndexNum > 2`, this read
   `A[k]` past the end of the 2-element array and called `.Empty()`/
   `.Smallest()` on whatever garbage stack memory followed it — a real
   out-of-bounds read, not just a style issue. Fixed by changing the
   bound to the literal `2`, matching the other two loops in the same
   function. `BUGFIX #4` in source.

3. **`delete` vs `delete[]` mismatch in `SoundexSearch()`** — `Cache`
   is allocated with `new GPTYPE[CacheSize*2]` (array form) but was
   freed with scalar `delete Cache;`, an allocator mismatch that's
   undefined behavior per the standard (silently tolerated by most
   allocators for a POD type like `GPTYPE`, which is why it never
   crashed in practice, but still wrong). Fixed to `delete [] Cache;`.
   `BUGFIX #5` in source.

4. **`INDEX::MergeIndexFiles()` over-allocated `A` by a factor of
   `sizeof(MERGEUNIT)`** — `A = new MERGEUNIT[sizeof(MERGEUNIT)*IndexNum];`
   allocated `sizeof(MERGEUNIT)` times more elements than the `IndexNum`
   actually used (indices `0..IndexNum-1`, confirmed by the loops right
   below it). Not out-of-bounds — the oversized array still covers
   every real access — just a large, pointless memory allocation scaled
   by an unrelated constant. Fixed to `new MERGEUNIT[IndexNum];`.
   `BUGFIX #6` in source.

5. **`INDEX::INDEX()` constructor left several members uninitialized**
   — `SetCache`, `DocTypePtr`, `TheThesaurus` (raw pointers) and
   `MergeStatus`, `Accesses`, `InCache`, `OutCache` (counters used by
   `ValidateInField()`'s cache-slide logic) were never assigned in the
   constructor. `SetCache` and `TheThesaurus` are confirmed (by
   tree-wide grep) to be 100% dead members — never assigned or read
   anywhere else — so removing them outright would be the more thorough
   fix, but that's a header change (GENERAL step 4 freezes public
   header signatures) and neither is a live bug on its own since
   nothing dereferences them; left declared, just safely initialized to
   `nullptr`/`GDT_FALSE`/`0`. `DocTypePtr` *is* live (read via the
   public `GetDocTypePtr()`), so this one is a genuine fix, not just
   hardening — confirmed via the new
   `INDEX constructor safely initializes pointer members` test, which
   checks `GetDocTypePtr() == nullptr` right after construction.
   `BUGFIX #3` in source.

Also removed (this turn, `index.cxx` only): the file-scope globals
`FILE *flist[40]; STRING Names[40]; INT fcount=0;` were declared but
never read or written anywhere in the tree (confirmed by grep, and
distinct from `MemoryData`/`MemoryDataLength`, which *are* used
throughout `AddRecordList()` and were kept). Not a header change — pure
dead file-scope state — so removed rather than just left alone per
GENERAL step 6.

Modernization: all 12 `sprintf` call sites converted to `snprintf`
(each already had a same-scope fixed-size buffer to pass as the size
argument); the handful of `(FILE*)NULL`/`(PFILE)NULL`/`NULL` comparisons
converted to `nullptr` (two remaining `NULL` references are inside
comments, left alone).

**Header note:** `src/index.hxx` needed one small addition to compile
standalone — a `class DOCTYPE;` forward declaration. It was already
using `DOCTYPE*` (for `DocTypePtr`/`SetDocTypePtr()`/`GetDocTypePtr()`)
but relying on every real includer having already pulled in the full
`doctype/doctype.hxx` transitively via `dtreg.hxx` first (`index.cxx`
itself includes `dtreg.hxx` before `index.hxx`). This is not a
signature change — no type, parameter, or return type changed, it's
exactly the same `DOCTYPE*` as before — just what let
`tests/src/test_index.cxx` include `index.hxx` directly without pulling
in the entire not-yet-processed `doctype/` tree.

**BUILD note:** `INDEX`'s implementation spans 5 `.cxx` files (see
above), all of which had to be added to `TEST_ENGINE_SRCS` for the
linker to resolve `INDEX::NumericSearch()`, `::DoDateSearch()`,
`::BoundingRectangle()`, `::MultiTermSearch()`, `::SortNumericFieldData()`,
etc. — none of those methods are defined in `index.cxx`. Being listed in
`TEST_ENGINE_SRCS` only means "linked for tests," not "processed" —
`numsearch.cxx`/`datesearch.cxx`/`geosearch.cxx`/`multiterm.cxx` are
still `pending` in `docs/PROCESSING_STATUS.md` and get their own
line-by-line review, `BUGFIX` treatment, and doc comments on their own
future turns, same as `tokengen.cxx`/`thesaurus.cxx`/`nlist.cxx`/
`intlist.cxx`/`squery.cxx`/`mergeunit.cxx`, which were also added purely
to satisfy the link.

Not otherwise pursued this turn (documented here rather than acted on,
to keep this turn's diff reviewable): `ValidateInField()`'s
`GpS=Cache[y-CacheBase]; GpE=Cache[y+1-CacheBase];` indexing depends on
a `CacheBase`/`X` invariant that's easy to misjudge without deeper
tracing across `SoundexSearch()`'s only caller of it; the many
unchecked `Parent->ffopen(...)` call sites that would `fgets`/read from
a null `FILE*` if the open failed (e.g. `MergeIndexFiles()`,
`CollapseIndexFiles()`) — this is a tree-wide pattern (20+ sites even in
this one file), not specific to `index.cxx`, so fixing it here alone
would be inconsistent; and a stray `"found", "",` empty-string entry in
`src/sw.hxx`'s `stoplist[]` that breaks its strict sort order (relevant
now that `BUGFIX #7` re-enables the binary search over it) — that's a
different file, out of scope for this turn.

## doctype/sgmlnorm.cxx

`class SGMLNORM` (base for the tag-parsing `DOCTYPE`s, e.g. `SGMLTAG`/
`HTML`) hand-rolls its own SGML tag scanner (`parse_tags()`) and
attribute parser (`store_attributes()`). No `BUGFIX`-worthy defect was
found this turn: the two functions' bounds checks were traced by hand
against several edge cases --- a `<!--...-->` comment or `<!DECL...>`
ending exactly at the buffer's last byte, a tag with no closing `>`,
and a tag whose content is entirely trailing whitespace --- and each
stays within the buffer (`RecBuffer` in `ParseFields()` is always
allocated with one extra byte and null-terminated at the actual read
length, which the scanner's `while (i < len)`/short-circuited
`&&` checks rely on). `store_attributes()`'s hand-rolled state machine
for `<tag attr="val" ...>` was read closely too; its handling of a
bare (no `=`) leading attribute is unusual (the "value" it stores is
the rest of the tag string, not just up to the next space) but that
reads as an original design choice for this legacy parser, not an
introduced regression, so it was left alone rather than "fixed" on a
guess.

Modernization: all 17 `NULL` uses in code converted to `nullptr`
(4 more, in doc-comment prose describing the nullptr-terminated tag
list convention, correctly left as English "NULL"). No `sprintf` calls
present.

Added doc comments to `SGMLNORM` (class-level), `UnifiedName()`,
`ParseFields()`, and expanded the existing `parse_tags()`/
`find_end_tag()`/`store_attributes()` header comments in
`sgmlnorm.hxx` with ownership/lifetime notes (e.g. `parse_tags()`'s
caller owns the returned array but not what it points into).

Tests (`tests/doctype/test_sgmlnorm.cxx`) cover `UnifiedName()`
(identity passthrough), `parse_tags()` (a simple tag pair, and the
unterminated-tag error path), `find_end_tag()` (match and no-match),
and one `ParseFields()` integration test against a real temp file
verifying both the extracted field names and that leading/trailing
whitespace inside a tag's content is trimmed from the stored field
coordinates. That last test initially failed for two build-your-own-
fixture reasons worth noting for future turns, neither an `SGMLNORM`
bug: `RECORD::SetFileName()` runs `RemovePath()` on whatever it's
given, so a temp file's full path has to go through `SetPathName()`
(directory) and `SetFileName()` (basename) separately, not
`SetFileName()` alone; and `DF::SetFieldName()` (`src/df.cxx`)
uppercases internally regardless of what the caller passes in, so
`<title>` legitimately becomes field name `TITLE`, not `title`.

## doctype/sgmltag.cxx

`class SGMLTAG` is a second, independent tag-parsing `DOCTYPE` (not a
subclass of `SGMLNORM`) with a stricter contract: only tag pairs whose
open/close text matches exactly (besides the closing `/`) count as a
field.

1. **`SGMLTAG::ParseFields()` leaked its filename buffer on every
   single return path** — `file = fn.NewCString();` allocates a
   heap C-string near the top of the function (used by two `perror()`
   calls), but nothing ever `delete []`s it: not on any of the 9 early
   `return`s (bad open, seek failures, zero-length record, allocation
   "failures", short/failed `fread`, failed `sgml_parse_tags()`), and
   not on the normal-completion path either. Every call to
   `SGMLTAG::ParseFields()` — i.e. every SGML-tagged record indexed
   through this doctype — leaked one allocation the length of the
   record's file name. Fixed by adding `delete [] file;` immediately
   before all 10 exit points. `BUGFIX #1` in source. Covered by
   `SGMLTAG::ParseFields does not leak the file-name buffer on a
   missing file` (exercises the file-not-found path;
   `make tests-asan`'s LeakSanitizer is what actually verifies this,
   not the assertions in the test itself).

2. **Dead `OrigRecBuffer` copy, and unused `val_len2`** — `ParseFields()`
   allocated a second `RecLength+1`-byte buffer, `memcpy`'d the record
   into it, and null-terminated it (per the file's own 1.02 changelog
   entry, this used to matter — "OrigRecBuffer was being overwritten" —
   implying some earlier version read from it), but nothing in the
   current function ever reads `OrigRecBuffer` again; it's allocated,
   populated, and immediately `delete[]`'d on every path, achieving
   nothing observable. `val_len2` was declared and never used at all.
   Not a correctness bug — no wrong behavior resulted — but a confirmed
   100%-dead allocation/copy on every call, removed per GENERAL step 6
   (same rationale as the dead `flist`/`Names`/`fcount` globals removed
   from `src/index.cxx` this batch).

Not otherwise pursued: `sgml_parse_tags()`'s `case '<': t[tc] = &b[i+1];`
doesn't skip leading whitespace after `<` (unlike `SGMLNORM::parse_tags()`,
which does) — this looks like a genuine behavioral difference between the
two SGML parsers, not an obvious regression, so left alone rather than
"fixed" on a guess; and `*numtags`'s `UsefulSearchField()`-gated count is
computed correctly but never read by `ParseFields()`, its only caller
(tested directly in `test_sgmltag.cxx` since the function's own contract
is still worth verifying, even though nothing consumes it yet).

Modernization: all 9 code-level `NULL` uses converted to `nullptr` (6
more, in doc-comment prose, correctly left as English "NULL"). No
`sprintf` calls present. Added class/function doc comments to
`sgmltag.hxx`.

Tests (`tests/doctype/test_sgmltag.cxx`) cover `sgml_parse_tags()`
(tag extraction and the `numtags` count), `find_end_tag()` (match,
and SGMLTAG's stricter no-match case for attribute-bearing tags), a
`ParseFields()` integration test against a real temp file, and the
leak regression test described above.

## src/strstack.hxx

`class STRSTACK` is a small LIFO stack of `STRING`s backed by `STRLIST`
(used by `src/infix2rpn.cxx` and `doctype/dif.cxx`). No functional
`BUGFIX` this turn -- `Push()`/`Pop()`/`Examine()`'s "cursor into an
array-like list" implementation was checked against `STRLIST::SetEntry`/
`GetEntry`'s 1-based/grow-on-set semantics and is correct, including the
non-obvious part: `Pop()` only moves `CurrIndex` back, it doesn't erase
the `STRLIST` node, so a later `Push()` must (and does, via `SetEntry`)
overwrite that stale slot rather than leaving or skipping it --
confirmed with a dedicated test (`STRSTACK reuses popped slots
correctly on a later push`).

1. **Header not self-contained** — `strstack.hxx` used `STRING` (in
   `Push()`'s parameter) and `STRLIST` (`StackList`'s type) without
   including either; both `#include`s were commented out, leaving only
   `gdt.h`. Same class of issue as `src/fc.hxx` (see that entry, first
   in this catalog): silently fine only because every real includer
   happens to pull in `string.hxx`/`strlist.hxx` first. Restored both
   includes. `BUGFIX #1` in source. Not a signature change (no
   declaration's type changed) so didn't need GENERAL step 4's header
   freeze.

No `NULL`/`sprintf` usages present. Added class/method doc comments.
Tests (`tests/src/test_strstack.cxx`) cover empty-stack behavior,
LIFO Push/Pop ordering, `Examine()` (peek without popping, and on an
empty stack), and the popped-slot-reuse case above.

## src/infix2rpn.cxx

`class INFIX2RPN` translates an infix boolean query (terms, AND/OR/
ANDNOT/NEAR, parens) to RPN via shunting-yard, using a fixed-size
`CHR DefaultOp[MAX_OP_LEN]` (8 bytes) member for the implicit operator
inserted between two adjacent terms with no explicit operator.

1. **3-arg constructor: unchecked `strcpy` overflowed `DefaultOp`** —
   `INFIX2RPN(const STRING&, STRING*, const CHR *Op)` did
   `strcpy(DefaultOp, Op);` directly, with no length check, even though
   `SetDefaultOp()` (same class) already has the correct check
   (`if (strlen(Op) < MAX_OP_LEN) strcpy(...); else strcpy(DefaultOp,
   "AND");`) for exactly this same buffer. Confirmed real with a
   standalone repro (`INFIX2RPN p(in, &out, "<49 chars>")`) compiled
   under ASan before fixing — `AddressSanitizer: stack-buffer-overflow
   ... WRITE of size 47 ... in INFIX2RPN::INFIX2RPN`. No in-tree caller
   currently uses this constructor with a long `Op` (only the 2-arg
   constructor is used, in `src/Isearch.cxx`/`src/zsearch.cxx`), but
   it's a `public` constructor, so any future caller (or the same
   pattern reintroduced later) would corrupt memory. Fixed by
   delegating to the already-correct `SetDefaultOp(Op)` instead of
   duplicating (and this time getting wrong) its bounds check.
   `BUGFIX #2` in source; re-ran the same repro after the fix to
   confirm it's clean. Covered by `INFIX2RPN 3-arg constructor does not
   overflow on an oversized Op`.
2. **Default constructor left `TermsWithNoOps` uninitialized** — the
   other two constructors indirectly zero it via `Parse()`'s own
   `TermsWithNoOps = 0;`, but the no-arg constructor never calls
   `Parse()`, so `InputParsedOK()` (which reads `TermsWithNoOps`)
   returned garbage if called before any `Parse()`. Same recurring
   pattern as prior turns' uninitialized-primitive-member fixes (RESULT,
   NUMERICFLD, SRCH_DATE, `INDEX`). Fixed via a member-initializer list.
   `BUGFIX #1` in source. Covered by `INFIX2RPN default constructor
   starts with a deterministic parse state` (a plain assertion, not an
   ASan-catchable case — reading an uninitialized `INT` isn't something
   `-fsanitize=address,undefined` flags without MemorySanitizer, which
   this tree doesn't build with).

Not otherwise pursued: `Parse()`'s `")"`/`ProcessOp()`'s "pop until
left-paren" loops don't distinguish "found `(`" from "stack ran empty
first" before their final unconditional `Pop()` (mismatched/unbalanced
parens) — `STRSTACK::Pop()` already handles an empty stack safely
(returns `GDT_FALSE`, leaves `*Value` untouched, confirmed in
`src/strstack.cxx`'s own turn this batch), so this is silently-lenient
behavior on malformed queries, not a memory-safety bug; the class
already has unused `RegisterError()`/`GetErrorMessage()` infrastructure
and even a commented-out "two operands in a row" error path, suggesting
incomplete-but-intentional error handling rather than a regression, so
left alone rather than guessed at.

No `NULL`/`sprintf` usages in code (one `NULL` reference is inside a
commented-out dead line). Added class/method doc comments. Tests
(`tests/src/test_infix2rpn.cxx`) cover basic AND/OR/ANDNOT/NEAR
translation and their symbolic aliases, paren precedence, implicit-
default-operator insertion between adjacent terms, the 3-arg
constructor (valid and oversized `Op`), `SetDefaultOp()`'s own
too-long fallback, and both `BUGFIX`es above.

## src/glist.hxx

`class GLIST` is a generic intrusive doubly-linked list of untyped
`GATOM*` (`void*`) pointers, each cell tagged with a type code. Used by
`GSTACK` (`src/gstack.hxx`/`.cxx`, its own future turn).

1. **`Delete()` left a dangling `Prev` pointer on interior deletes,
   confirmed a real heap-use-after-free** — deleting a cell from the
   *interior* of the list (not head, not tail) did
   `c->Prev->Next = c->Next; c->Next = c->Prev;` — the second line sets
   a field on `c` itself, which is about to be `delete`d anyway,
   accomplishing nothing, instead of `c->Next->Prev = c->Prev;`, which
   is what actually needs updating so `c`'s successor no longer points
   back at the freed cell. Confirmed with a standalone repro (insert
   3 cells, delete the middle one, then `Prev()`/`Retrieve()` the last
   cell) compiled under ASan *before* fixing:
   `AddressSanitizer: heap-use-after-free ... READ of size 8 ... in
   GLIST::Retrieve`. Fixed to update the correct pointer; re-ran the
   same repro after the fix to confirm it's clean. `BUGFIX #1` in
   source. Covered by `GLIST Delete from an interior position relinks
   both neighbors` (the assertions there confirm the relink; the
   use-after-free itself is what `make tests-asan` verifies on a
   regression).
2. **`InsertBefore()` decoupled each atom from its own type tag** —
   "insert before c" is implemented by inserting a new cell *after* c
   and then swapping data between the two cells (so callers already
   holding a `GPOSITION*` to `c` keep pointing at the right logical
   position). The swap used `Update()`, which only ever sets `Atom`
   (never `Type`), so after the swap the new atom's `Type` stayed on
   the cell that ended up holding the *old* atom, and vice versa.
   Confirmed with a standalone repro: `InsertBefore` an atom tagged 99
   next to one tagged 42, and the two atoms came back with their type
   tags crossed. Fixed by swapping `Atom` and `Type` together directly
   on the two cells instead of going through `Update()`. `BUGFIX #2` in
   source; the 2-argument `InsertBefore(c, a)` overload (which this
   also fixes) is reachable in the tree today via `GSTACK::Push()`
   (`src/gstack.cxx`). Covered by `GLIST InsertBefore keeps each atom
   paired with its own type tag`.

**Not fixed (needs a header change):** `GLIST` has no destructor at
all, so any cells still linked when a `GLIST` is destroyed leak --
confirmed with the same standalone repro used for `BUGFIX #1`
(LeakSanitizer flagged the two cells that test intentionally left
un-`Delete()`d: `64 byte(s) leaked in 2 allocation(s)`). Unlike the
`BUGFIX`es above, there's no way to add automatic cleanup without
declaring `~GLIST()` in `glist.hxx`, which GENERAL step 4 reserves for
a human decision -- and unlike this session's other blocked files
(`mergeunit.hxx`/`tokengen.hxx`/`thesaurus.hxx`/`squery.hxx`, all
blocked over a *copy*-safety double-free/UAF risk), this is "just" a
leak with no corruption/crash risk on its own, and `Delete()`'s own doc
comment ("Caller has freed memory allocated within the atom in c")
already documents an expectation of manual, node-by-node cleanup
discipline -- so this turn fixed the two confirmed memory-*safety* bugs
above and documented the leak here rather than blocking the whole file
over it. Proposed fix for a future header-focused pass: a
`~GLIST()` that walks `Head` and `delete`s each remaining cell (atoms
are never owned, so nothing else to free).

Modernization: all code-level `NULL` uses converted to `nullptr`
(4 more, in doc-comment prose, correctly left as English "NULL"). No
`sprintf` calls present. Added class/method doc comments, including the
destructor/ownership caveat above. Tests (`tests/src/test_glist.cxx`)
cover empty-list state, forward/backward traversal after `InsertAfter`,
`Delete` from interior/head/tail/singleton positions, `InsertBefore`
(including into an empty list), and `Update`.

## doctype/colondoc.cxx

`class COLONDOC` parses "colon-tagged" (IAFA-like) records: lines of
the form `Tag: value`, where a value continues across following lines
until the next `Tag:` line. This turn found the most bugs of any file
so far — four related off-by-ones in `ParseFields()`'s value-boundary
math, one of which only surfaced once another was fixed.

1. **File read truncated the last byte of every record** —
   `RecEnd = ftell(fp) - 1;` in the "no explicit RecordEnd" fallback
   (the common case) meant `fread()` never read the file's actual last
   byte at all. Neither `sgmlnorm.cxx` nor `sgmltag.cxx` do this same
   `-1` — `sgmltag.cxx` even has a `//RecEnd -= 1;` left commented out
   at the equivalent spot, i.e. a prior author considered and rejected
   this exact adjustment there. Fixed by dropping the `- 1`. `BUGFIX
   #1` in source.
2. **Trailing-newline exclusion assumed a newline was always there**
   — `INT val_len = (p - *tags_ptr) - off - 1;` unconditionally
   excluded one byte "for the \n", correct for every interior field
   (the format guarantees exactly one `\n` before the next `Tag:`
   line) but wrong for the *last* field when the file doesn't end with
   `\n` — confirmed with a test file ending `...Jane Doe` (no trailing
   newline): the field came back as `"Jane Do"`, one byte short. Fixed
   by only excluding it when `p[-1] == '\n'` is actually true. `BUGFIX
   #1b` in source (found while writing the regression test for
   `BUGFIX #1`, since removing the `-1` in `BUGFIX #1` is what first
   made this file long enough for the bug to be reachable in a
   realistic-looking test case).
3. **Trailing-whitespace trim checked the wrong index, chopping the
   last real character off nearly every value** —
   `RecBuffer[val_len + val_start]` checks one byte *past* the value's
   actual last character (`val_start + val_len - 1`); since that
   position is almost always the delimiter (`\n` or the next tag) and
   almost always whitespace, this trimmed one real trailing character
   off of nearly every field. Confirmed with a standalone repro:
   `"Hello World"` came back as `"Hello Worl"`. `BUGFIX #2` in source.
4. **`SetFieldEnd()` stored one byte too many** —
   `fc.SetFieldEnd(val_start + val_len)` is one past the correct
   *inclusive* end index; every other doctype parser (`sgmlnorm.cxx`,
   `sgmltag.cxx`) computes `val_start + val_len - 1` here, and
   `src/index.cxx` derives a field's length as
   `GetFieldEnd() - GetFieldStart() + 1`, which only works for an
   inclusive end. This `+1` happened to numerically cancel out against
   `BUGFIX #2`'s `-1` in the common single-trim-iteration case, which
   is almost certainly why neither was noticed — fixing `BUGFIX #2`
   alone, without this one, would have turned "accidentally correct"
   stored field coordinates into genuinely wrong ones. `BUGFIX #4` in
   source; fixed together with `BUGFIX #2` for exactly this reason.
5. **Last-field fallback used the wrong length variable** —
   `p = &RecBuffer[RecLength]` used the buffer's allocated capacity,
   not `ActualLength` (bytes actually read) — harmless now that
   `BUGFIX #1` keeps them equal in the normal case, but would have
   extended the last field into unread/uninitialized-but-allocated
   bytes on a short `fread()`. Fixed to use `ActualLength`. `BUGFIX #3`
   in source.

All four numbered fixes (`#1`/`#1b`/`#2`/`#4`; `#3` is a smaller
consistency fix) are covered together by
`COLONDOC::ParseFields extracts full field values, including the last
byte of the file`, which deliberately uses a file with no trailing
newline and checks the *stored* `FC` coordinates (read back via
`GetFieldStart()`/`GetFieldEnd()`, the same way the real engine
retrieves field text) rather than an internal variable — the only way
to actually catch `BUGFIX #4` given how it interacted with `BUGFIX #2`.

Modernization: all code-level `NULL` uses converted to `nullptr`. No
`sprintf` calls present. Added class-level and `ParseFields()` doc
comments. Tests (`tests/doctype/test_colondoc.cxx`) also cover
`UnifiedName()` and leading-whitespace trimming after the `:`.

## doctype/mailfolder.cxx

`class MAILFOLDER` parses Unix mail folders (mbox-style): one message
per `From `/`Article ` boundary, RFC822-ish headers, then a
`Message-body` field for everything after the blank line separating
headers from the body. Its `ParseFields()` shares essentially the same
value-boundary math as `doctype/colondoc.cxx` (processed earlier this
batch) and has the same five off-by-ones, confirmed and fixed the same
way -- see that entry for the detailed reasoning behind each. It also
has two bugs of its own, one of them the most severe found this batch.

1. **`parse_tags()`: unbounded From/Article-line skip caused a real
   heap-buffer-overflow** — three leading loops (skip whitespace, skip
   to end of the From/Article line, skip its trailing newlines) advance
   the local pointer `b` forward but never decremented `len` to match,
   and had no bound of their own besides the buffer eventually
   containing a `'\n'` or non-`'\n'` byte -- not guaranteed, since this
   buffer comes from `fread()`, not line-buffered I/O. The `for`
   loop right after trusted the *original* `len` as how many bytes are
   safely readable starting from the now-*advanced* `b`, so it could
   run up to `len` bytes past the buffer's true end -- and that loop
   both reads (`b[i]`) and writes (`b[i] = '\0'`) through it, so this
   was a potential out-of-bounds write, not just a read. Confirmed a
   real heap-buffer-overflow with a standalone repro (a buffer
   containing only a valid `From ` line plus one short header, no
   body -- exactly the shape that leaves nothing after the header for
   the old code to stop at) compiled under ASan *before* fixing:
   `AddressSanitizer: heap-buffer-overflow ... READ of size 1 ... in
   MAILFOLDER::parse_tags`. Fixed by having all three loops stop at
   `len` too, decrementing it as `b` advances. `BUGFIX #1` in source;
   re-ran the repro after the fix to confirm clean. Covered by
   `MAILFOLDER::ParseFields handles a header-only message with no
   body` (`make tests-asan` is what actually verifies this one, same
   as the repro).
2. **`RecEnd = ftell(fp) - 1;`** truncated the last byte of every
   record read via the common fallback path. Same bug as, and fixed
   the same way as, `colondoc.cxx`'s `BUGFIX #1`. `BUGFIX #2` in
   source.
3. Unconditional **"leave off the `\n`"** subtraction, wrong for the
   last field when the file doesn't end with `'\n'`. Same bug as, and
   fixed the same way as (only exclude it when `p[-1] == '\n'`),
   `colondoc.cxx`'s `BUGFIX #1b`. `BUGFIX #2b` in source.
4. **Trailing-whitespace trim checked one byte past the value's real
   last character**, silently chopping a real trailing character off
   nearly every field. Same bug as, and fixed the same way as,
   `colondoc.cxx`'s `BUGFIX #2`. `BUGFIX #3` in source.
5. **`SetFieldEnd()` stored one byte too many**, numerically canceling
   out against `BUGFIX #3` in the common case (same interaction as
   `colondoc.cxx`'s `BUGFIX #2`/`#4` -- fixing one without the other
   would have turned accidentally-correct field coordinates into
   genuinely wrong ones). Same bug as, and fixed the same way as,
   `colondoc.cxx`'s `BUGFIX #4`. `BUGFIX #4` in source; fixed together
   with `BUGFIX #3` for that reason.
6. **Last-field fallback used `RecLength` instead of `ActualLength`**
   — harmless now, would extend into unread/uninitialized-but-allocated
   bytes on a short `fread()`. Same bug as, and fixed the same way as,
   `colondoc.cxx`'s `BUGFIX #3`. `BUGFIX #5` in source.
7. **`NameKey()`: `strcpy()` with overlapping source and destination**
   — both `strcpy(email, s + 1)` and `strcpy(s, e + 1)` shift part of
   `email` down over itself in place (e.g. `"Name <addr>"` ->
   `"addr"`), which overlaps whenever the matched delimiter isn't at
   position 0 -- undefined behavior for `strcpy()` specifically (unlike
   `memmove()`). Not a latent/theoretical concern: ASan's
   `strcpy-param-overlap` check aborted on exactly this input
   (`"Name <addr>"`) while writing the regression test for `BUGFIX #1`
   above, unprompted. Fixed by using `memmove()`, which is defined for
   overlapping ranges, at both call sites. `BUGFIX #6` in source.
   Covered by `MAILFOLDER::NameKey extracts the name part of "Name
   <addr>"` (again, `make tests-asan` is what actually verifies it).

**Incidental fix (not a `mailfolder.cxx` bug):** linking `src/marc.cxx`
into `TEST_ENGINE_SRCS` (needed for `RememberKey`, which
`src/marclib.cxx` — already linked — declares `extern` and expects some
translation unit to define) collided with `tests/src/test_marclib.cxx`,
which had its own stub `struct MemBlock *RememberKey = nullptr;`
because `marc.cxx` (the real definer) wasn't linked yet back when that
test was written. Removed the now-redundant stub; `marc.cxx`'s real
definition is used instead.

Modernization: all code-level `NULL` uses converted to `nullptr`. No
`sprintf` calls present. Added class-level and per-function doc
comments. Tests (`tests/doctype/test_mailfolder.cxx`) also cover
`IsMailFromLine()`, `IsNewsLine()`, `accept_tag()`, and a full
headers-plus-body `ParseFields()` case.

## src/gstack.hxx

`class GSTACK` is a LIFO stack of untyped `GATOM*` pointers, backed by
a `GLIST Stack;` member plus a `CurrentIndex` cursor. Used by 5
`doctype/*.cxx` files (`cipc.cxx`, `cipp.cxx`, `anzmeta.cxx`,
`anzlic.cxx`, `fgdc.cxx`, all still `pending`) to track nested-field
depth while parsing.

1. **`Top()`/`Pop()` crashed (null-pointer dereference) on an empty
   stack** — both computed `CurrentIndex = Stack.First()` (`nullptr`
   when empty) and then called `Stack.Retrieve(CurrentIndex)`, which
   dereferences its argument unconditionally
   (`GLIST::Retrieve(GPOSITION *c) { return c->Atom; }`). Confirmed a
   real crash with a standalone repro (`GSTACK().Top()`) before fixing:
   `AddressSanitizer: SEGV ... in GLIST::Retrieve`. Every call site
   found in the tree happens to check `GetSize() != 0` before reaching
   `Top()`/`Pop()` via its own surrounding logic, so this wasn't
   observed to crash in practice, but `GSTACK` itself shouldn't depend
   on every caller getting that right. Fixed both to check
   `CurrentIndex == nullptr` and return `nullptr` instead of
   dereferencing it. `BUGFIX #2` in source. Covered by `GSTACK::Top and
   Pop on an empty stack return nullptr instead of crashing`.
2. **Constructor left `CurrentIndex` uninitialized** — never actually
   reachable as a live bug (`Push()`/`Top()`/`Pop()` all unconditionally
   overwrite it before any read), but the same class of fix as this
   batch's other uninitialized-raw-pointer-member turns
   (`src/index.cxx`, `src/infix2rpn.cxx`). Fixed via a member-
   initializer list. `BUGFIX #1` in source.

**Not fixed (needs a header change):** `GSTACK` publicly inherits
`GLIST` (`class GSTACK : public GLIST`) but never uses that base
class — every method operates on the `Stack` *member* instead, so the
inherited `GLIST`'s own `Head`/`Tail`/`Length` sit there permanently
empty and unused on every `GSTACK` instance. Confirmed by reading every
method: none call an inherited (unqualified, implicit-`this`) `GLIST`
method. Not a currently-live bug either — grepping all 5 real callers
shows none call an inherited `GLIST` method (`IsEmpty()`, `First()`,
etc.) directly on a `GSTACK` instance, only `GSTACK`'s own
`Push`/`Top`/`Pop`/`GetSize` — but if one ever did, it would silently
operate on the wrong (always-empty) list instead of `Stack`. Removing
`: public GLIST` is a signature change (GENERAL step 4 header freeze),
so this turn documented it rather than fixing it, same rationale as
`src/glist.hxx`'s missing-destructor note from earlier this batch.

No `NULL`/`sprintf` usages present. Added class-level and per-method
doc comments, including the vestigial-inheritance note above. Tests
(`tests/src/test_gstack.cxx`) cover empty-stack state, the
`BUGFIX #2` regression, and LIFO Push/Top/Pop ordering.

## src/isearch.hxx

A pure convenience aggregator: declares nothing of its own, just
`#include`s the ~30 core engine headers a doctype parser or CGI
frontend typically needs (`STRING`, `RECORD`, `MDT`, `DOCTYPE`, the
query-evaluation classes, etc.). No bugs possible to find in a file
with no logic of its own; confirmed self-contained (compiles standalone
as the sole `#include` in a translation unit, given the right include
paths for both `src/` and `doctype/`). Added a file-level doc comment
and the processed marker.

Test (`tests/src/test_isearch.cxx`) is necessarily a smoke test rather
than behavioral coverage: it includes only `isearch.hxx` and touches a
few of the aggregated types (`STRING`, `STRLIST`, `RECORD`, `GSTACK`)
to confirm the full set of ~30 includes is mutually coherent (no
missing includes, no redefinition/ambiguity conflicts) when combined,
not just individually self-contained.

## src/idb.hxx

`class IDB` is the concrete, on-disk-filesystem database: owns
`MainIndex`/`MainMdt`/`MainDfdt`/`MainRegistry`/`DocTypeReg` and
coordinates them for indexing and search. 1832-line `.cxx`; reviewed
the constructors/`Initialize()`/destructor in full plus a representative,
not-exhaustive sample of the largest/most-central methods
(`SetWrongEndian`, `IsSystemFile`, `DfdtGetFileName`, `KeyLookup`,
`GetRecordDfdt`, the `GetFieldData` overloads, `Index()`, `KillAll()`,
`CleanupDb()`, `DeleteByKey`/`UndeleteByKey`, `MakeDbGilsRec`) --
same "thorough but not exhaustive" scope as `src/index.cxx` earlier
this batch.

1. **`MakeDbGilsRec()` leaked its date buffer** — `date` was a
   `malloc(9)`'d buffer used to hold `strftime()`'s `"YYYYMMDD"` output
   and then `Cat()`'d into the output buffer, but never `free()`'d.
   Reachable via this function's one live caller (`src/Iutil.cxx`).
   Also, if the (9-byte, essentially-never-fails) allocation *did*
   fail, `date` stayed `nullptr` and was passed unchecked to
   `buffer->Cat(date)`. Fixed by using a fixed-size stack array
   (`CHR date[9]`) instead of `malloc`, which needs neither a `free()`
   nor a null check. `BUGFIX #1` in source.

**Not fixed (needs a header change):** like `GSTACK`/`GLIST` earlier
this batch, `IDB` has several raw owning pointer members
(`MainIndex`/`MainMdt`/`MainDfdt`/`MainRegistry`/`DocTypeReg`) and a
destructor that `delete`s all of them, but no declared copy
constructor/`operator=` -- the same "double-free on copy" shape that's
triggered blocking for other files this session. Checked reachability
the same way as those: grepped the whole tree for any place `IDB` is
copied, sliced, or passed/returned by value. Found none -- every real
use is via pointer (`IDB*`/`PIDB`) or as a base class (`VIDB`,
`Isearch-cgi/*.cxx`'s `IDBC`), never by value. Since there's no
confirmed-reachable copy site, this turn documented the risk rather
than proposing a header change for it, consistent with how
`src/glist.hxx`'s missing destructor and `src/gstack.hxx`'s vestigial
inheritance were handled earlier this batch.

**Deferred: no live-`IDB` integration test this turn.** Every method
on `IDB` (and the free function `MakeDbGilsRec()`) needs a constructed
`IDB`, and `IDB::Initialize()` unconditionally does
`DocTypeReg = new DTREG(this);`. `DTREG`'s own `.cxx`
(`src/dtreg.cxx`) directly references (behind runtime `if` branches,
but still present as calls in one function body) essentially every
`doctype/*.cxx` parser -- confirmed via `dtreg.hxx`'s ~37 `#include`s,
one per doctype class. Actually linking a test binary that constructs
a live `IDB` would mean pulling the *entire*, still almost entirely
`pending`, `doctype/` tree into `TEST_ENGINE_SRCS` in one turn -- tried
it in a scratch build to see how bad it really was rather than just
assuming, and a single-invocation compile of all ~40 files together
didn't finish inside a 90-second budget. That's a disproportionate,
slow, one-file dependency jump compared to every other turn this batch
(the largest prior pull, for `src/index.cxx`, was ~10 files), so this
turn left it deferred rather than forcing it through. `idb.hxx` itself
was confirmed self-contained (compiles standalone as the sole
`#include`) and `idb.cxx` was confirmed to compile cleanly under
`-Wall -Wextra` (`g++ -c`) with the `BUGFIX #1` fix applied; the fix
itself was validated by manual trace (allocate-once/use-twice/never-
freed, confirmed via `grep` across the whole function) rather than a
sanitizer repro, the same standard applied to other found-but-
not-repro'd issues earlier this batch.

Modernization: all code-level `NULL` uses converted to `nullptr`
(2 sites: `time(nullptr)`, plus the `date` buffer above), the one live
`sprintf` converted to `snprintf` (a second, in a `/* ... */`-commented
out and never-compiled old `IsSystemFile()` implementation, was left
alone). Added class-level and per-function doc comments to `IDB`,
`Initialize()`, the destructor, `Index()`, `KillAll()`, `CleanupDb()`,
and `MakeDbGilsRec()`.

## src/vidb.hxx

`class VIDB` is a "virtual database": a view over a list of real `IDB`
databases (`c_dblist`), loaded from a `.vdb` file naming one
sub-database per line. Found the most externally-significant bug
category of this batch's non-index-parser files.

1. **Multiple methods indexed `c_dblist` with an unvalidated index —
   confirmed externally reachable via a real request path** —
   `KeyLookup()` parses a database number directly out of its `Key`
   argument (`"DBnum:Key"` syntax) and indexed `c_dblist[i]` with it,
   with no bounds check. Traced every real caller in the tree:
   `src/Iget.cxx` and `src/zpresent.cxx` both declare `VIDB *pdb` and
   pass a record key straight from their own command-line/protocol
   arguments through to `pdb->KeyLookup()` -- i.e. a remote client of
   either of those tools controls `Key`, and a key like `"999:x"`
   against a `VIDB` with fewer than 1000 sub-databases would have read
   `c_dblist[999]` out of bounds and then called a method through
   whatever garbage pointer was sitting there. Fixed by bounds-checking
   `i` against `c_dbcount` and returning (leaving `*ResultBuffer`
   untouched) when out of range. `BUGFIX #1` in source.

   The same function's no-prefix-key branch, plus `GetDfdt()` and
   `GetRecordDfdt()`, unconditionally used `c_dblist[0]` -- safe only
   because every *other* code path happens to guarantee `c_dbcount >
   0`, except one: a `.vdb` file that exists, is non-empty, but
   contains only comment/blank lines leaves `Initialize()` completing
   normally (it only `EXIT_ERROR`s on a *fully* empty raw file) with
   `c_dbcount == 0` and `c_dblist[0]` never written to -- so this
   dereferenced an uninitialized pointer in that specific case. Fixed
   with the same `c_dbcount <= 0` guard. `BUGFIX #1` (continued) in
   source.

2. **`GetDbNameByNumber()` and `Present()` had the same missing-bounds-
   check shape**, using a caller-supplied (`GetDbNameByNumber`) or
   `RESULT`-embedded (`Present`) index into `c_dblist` with nothing
   stopping an out-of-range value. Neither was confirmed externally
   reachable the way `BUGFIX #1` was -- `GetDbNameByNumber()`'s only
   callers (`Isearch-cgi/isrch_srch.cxx`) pass back a `DbNum` a search
   result was already stamped with internally, and `RESULT::GetDbNum()`
   defaults to `0` (safe whenever `c_dbcount > 0`, per `RESULT`'s own
   earlier turn this session) -- but both are `public` API on a class
   whose one confirmed-reachable sibling bug was serious, and the fix
   is free, so both got the same guard. `BUGFIX #2` in source.

**Not otherwise pursued:** `c_inconsistent_doctypes` is set to
`GDT_FALSE` once in `Initialize()` and never set `GDT_TRUE` anywhere,
making the `if(c_inconsistent_doctypes) return GDT_FALSE;` check in
`IsDbCompatible()` permanently dead -- reads like an incomplete
feature (detecting when a `.vdb`'s sub-databases have inconsistent
doctypes) rather than a regression, and implementing that detection
from scratch would be guessing at intended behavior, not fixing a
confirmed bug, so left alone and just noted here.

**No live-`VIDB` integration test this turn**, for the same structural
reason as `src/idb.hxx` just above (`VIDB::Initialize()` constructs
real `IDB` objects, which need the full `DTREG`/`doctype/` dependency
chain) -- see that entry for the full reasoning. `vidb.hxx` confirmed
self-contained is not applicable here (it depends on `idb.hxx`/
`dtreg.hxx` by design, as a `VIDB` fundamentally needs `IDB`), but
`vidb.cxx` was confirmed to compile clean under `-Wall -Wextra` via
direct `g++ -c`, and the existing (unaffected, `vidb.cxx` isn't linked)
suite was re-run to confirm no regression elsewhere.

Modernization: the 2 code-level `NULL` uses converted to `nullptr`. No
`sprintf` calls present. Added a class-level doc comment.

## Isearch-cgi/cgi-util.hxx

`class CGIAPP` parses raw CGI form input (`QUERY_STRING` for GET, the
POST body off `stdin`, picked via `REQUEST_METHOD`) into name/value
pairs. Every one of this turn's three bugs takes fully client-
controlled input straight into a fixed-size stack buffer with no
bounds check -- the most severe bug cluster found in this whole
cleanup effort, and (unlike most of this session's other memory-safety
finds) directly, trivially reachable by an ordinary HTTP request, not
just a crafted key or a rare edge case.

1. **POST: `Content-Length`-sized read into a 256-byte stack buffer**
   — `cin.getline(temp1, ContentLen+1, '&')` told `getline` the buffer
   was `ContentLen+1` bytes -- `ContentLen` parsed straight from the
   client-supplied `Content-Length` header -- when `temp1` is a fixed
   `CHR temp1[256]`. A POST body over 255 bytes with no `&` in the
   first 255 overflows the stack. Confirmed with a standalone repro
   under ASan before fixing (a 400-byte body, no `&`):
   `AddressSanitizer: stack-buffer-overflow ... in
   std::istream::getline ... in CGIAPP::GetInput`. Fixed by capping the
   read at `sizeof(temp1)`; when a field doesn't fit, `getline` sets
   failbit without consuming the delimiter, so the leftover bytes are
   explicitly discarded up to the next real `&` (`cin.ignore(...,
   '&')`) to keep the parser aligned with the stream instead of
   misreading them as the start of the next field. `BUGFIX #1` in
   source; re-ran the repro after the fix to confirm clean.
2. **GET: no bound at all on the query-string copy loops** — the two
   `while` loops copying a name/value segment out of `QUERY_STRING`
   into `temp1`/`temp2` (also fixed 256-byte buffers) had no length
   check whatsoever -- `QUERY_STRING` is the entire URL query, fully
   attacker-controlled with no length limit enforced before this code
   runs. Confirmed with a standalone repro under ASan before fixing (a
   400-byte field name): `AddressSanitizer: stack-buffer-overflow ...
   in CGIAPP::GetInput`. Unlike the POST case, this is plain in-memory
   array iteration (not a stream), so the fix is simpler: cap the
   *write* index while still advancing the *read* index through the
   whole field, so parsing stays correctly aligned with the `=`/`&`
   delimiters and only the copy is truncated. `BUGFIX #2` in source.
3. **GET: writing through a string-literal pointer for a request with
   no `QUERY_STRING` at all** — when `getenv("QUERY_STRING")` returns
   `nullptr` (no query string present), `query` used to be pointed at
   a string literal `""`. `plustospace()`/`unescape_url()` are called
   on it immediately after and both write through their argument
   unconditionally (`unescape_url()` always writes a `'\0'` terminator
   even for an already-empty string) -- undefined behavior that
   crashes on a typical modern OS (write to read-only `.rodata`).
   Confirmed with a standalone repro under ASan before fixing --
   triggered by simply requesting the CGI script with no query
   string at all (no attack payload needed):
   `AddressSanitizer: SEGV ... WRITE ... in unescape_url ... in
   CGIAPP::GetInput`. Fixed by pointing `query` at a local mutable
   buffer (`CHR EmptyQuery[1] = "";`) instead of a literal. `BUGFIX #3`
   in source.

All three are covered by dedicated regression tests in
`tests/Isearch-cgi/test_cgi-util.cxx`; `make tests-asan` is what
actually re-verifies each one, the same as the standalone repros used
to confirm them before fixing.

**Not fixed (needs a header change):** `escape_url(PCHR url, PCHR
out)` writes up to 3x `strlen(url)` bytes into `out` (every non-
alphanumeric, non-space byte becomes a 3-byte `%XX` escape) with no
size parameter for `out` at all -- a caller that sizes `out` to match
`url` would overflow. Grepped the whole tree for callers and found
none (dead code, declared and defined but never invoked), so this
wasn't confirmed live, and fixing the root cause needs an output-size
parameter (a signature change) -- documented here instead, matching
this session's convention for public-API landmines with no confirmed
caller to break yet (see `src/glist.hxx`, `src/gstack.hxx`,
`src/idb.hxx`, `src/vidb.hxx` earlier this batch).

Also noted, not acted on: `GetName(INT4 i)`/`GetValue(INT4 i)` have no
bounds check against the actual entry count, and there's no public
accessor for that count either -- but grepped every real caller in the
tree and found none using these two directly (only `GetValueByName()`,
which is internally bounds-checked, and `CGIAPP::Display()`'s own
`entry_count`-bounded loop), so left alone rather than guessed at.

Modernization: all code-level `NULL` uses converted to `nullptr`. No
`sprintf` calls present (`escape_url()` already used `snprintf`).
Added class-level and field-level doc comments, including the
`escape_url()`/`GetName()`/`GetValue()` caveats above.

## src/marc.hxx

`class MARC` parses one MARC bibliographic record (via
`marclib.cxx`'s `GetMARC()`) and formats it for display. Sole real
caller: `doctype/usmarc.cxx`'s `USMARC::Present()`, which always
constructs, uses, and destroys exactly one `MARC` object at a time --
load-bearing for `BUGFIX #1` below.

1. **`~MARC()` never freed `c_rec`** — the destructor's only trace of
   this was the original author's own comment, `// FREE THE c_rec!!`.
   `c_rec` (and every `MARC_FIELD`/`MARC_SUBFIELD` hung off it) is
   allocated via `AllocSafe(&RememberKey, ...)` in `GetMARC()`
   (`marclib.cxx`), a custom "Intuition Remember"-style pool allocator
   (`src/memcntl.cxx`, processed earlier this session) that tracks
   every allocation in one linked list per `RememberKey` and can free
   the whole list in one call via `FreeSafe(&RememberKey, nullptr, 1)`
   -- exactly the primitive needed here, confirmed by checking that
   every allocation `GetMARC()` makes (including nested fields/
   subfields) goes through that same chain. Fixed by calling it in the
   destructor. The catch: `RememberKey` is **one process-wide chain,
   not per-object** -- calling this frees every live `MARC` object's
   `c_rec`, not just the one being destroyed, which would be a bug if
   two `MARC` objects were ever alive at once. Confirmed they aren't:
   the only real caller in the tree constructs, uses, and deletes one
   `MARC` at a time, every time. `BUGFIX #1` in source; covered by
   `MARC does not leak its parsed record`, verified via
   `make tests-asan`'s LeakSanitizer (not a repro -- a leak isn't a
   crash to reproduce, just something to observe going away).
2. **Constructor left `c_format`/`c_maxlen` uninitialized on a parse
   failure** — both were set *after* the `GetMARC()` failure check's
   early `return;`, so a malformed record left them uninitialized on
   exactly the path where a caller -- with no way to ask "did
   construction succeed?" (`c_rec` isn't exposed, there's no
   `IsValid()`) -- is likely to still call `Print()`/
   `GetPrettyBuffer()` anyway, which read `c_maxlen` as a word-wrap
   width and index into a line buffer with it. Fixed by moving both
   into the constructor's member-initializer list, so they're always
   valid regardless of parse success. `BUGFIX #2` in source; covered by
   `MARC handles a malformed record without leaving format state
   uninitialized`.
3. **Word-wrap helpers scanned backward for a space with no lower
   bound — confirmed a real stack-buffer-underflow** —
   `outputline()`/`OutputString()` each have two copies of
   `for (c = &line[maxlen - 1]; *c != ' '; c--);` ("find a word break
   to wrap at"), with nothing stopping `c` from running past the start
   of the buffer if the text has no space within range (an unbroken
   run of `maxlen`+ characters -- e.g. a URL or identifier in a real
   MARC field, or any field value with no spaces). Confirmed with a
   standalone repro under ASan before fixing (a 299-byte space-less
   field): `AddressSanitizer: stack-buffer-overflow ... READ ...
   underflows this variable ... in OutputString`. Fixed by bounding
   each scan at its buffer's start, falling back to a hard break at the
   original position when no space is found in range (matching the
   original code's intent for the has-a-space case, since that path is
   unaffected). `BUGFIX #3` in source (all 4 occurrences); re-ran the
   repro after the fix, plus a normal multi-word case, to confirm both
   the fix and no regression to ordinary wrapping. Covered by `MARC::
   Print wraps a long space-less field instead of crashing`.

**Not otherwise pursued:** `class MARC`'s declaration is wrapped in
`extern "C" { ... }` in `marc.hxx`, which doesn't really make sense for
a C++ class with constructors/methods (C linkage can't represent
those) -- harmless in practice since compilers just do the sensible
C++ thing for the class's own members regardless, but a header change
either way, so left alone.

Modernization: all live code-level `NULL` uses converted to `nullptr`
(one more, inside an already-dead `/* ... */`-commented-out
`GetPrettyBuffer()` implementation, correctly left alone). No live
`sprintf` calls (already using `snprintf`). Added class-level and
per-function doc comments. Tests (`tests/src/test_marc.cxx`) build a
minimal well-formed MARC record by hand (same byte layout as
`tests/src/test_marclib.cxx`'s existing helper) and cover a normal
parse-and-print, the leak regression, a malformed-record case, and the
long-field word-wrap regression.

## doctype/medline.hxx

`class MEDLINE` parses MEDLINE-format records: lines of the form
`AB  -value` (a 2-4 letter tag, then a mandatory space/dash separator),
value continuing across following lines until the next tag. Its
`ParseFields()` has the exact same shape as `doctype/colondoc.cxx`'s
(third file this session with this pattern, after
`doctype/mailfolder.cxx`), and turned out to share the identical
five-bug family, plus two bugs unique to this file.

1. **File read truncated the last byte of every record** — same as
   `doctype/colondoc.cxx`'s `BUGFIX #1`: `RecEnd = ftell(fp) - 1;`
   dropped the file's actual last byte before `fread()` ever saw it.
   Fixed the same way, by dropping the `- 1`. `BUGFIX #1` in source.
2. **Trailing-newline exclusion assumed a newline was always there** —
   same as `doctype/colondoc.cxx`'s `BUGFIX #1b`: an unconditional
   `val_len = (...) - off - 1;` is wrong for the last field in a file
   with no trailing `\n`. Fixed by only excluding the byte when
   `p[-1] == '\n'` is actually true. `BUGFIX #1b` in source.
3. **Trailing-whitespace trim checked the wrong index** — same as
   `doctype/colondoc.cxx`'s `BUGFIX #2`: `RecBuffer[val_len +
   val_start]` checks one byte past the value's actual last character
   instead of `RecBuffer[val_start + val_len - 1]`, silently chopping a
   real trailing character off nearly every field. `BUGFIX #2` in
   source.
4. **`SetFieldEnd()` stored one byte too many** — same as
   `doctype/colondoc.cxx`'s `BUGFIX #4`: `fc.SetFieldEnd(val_start +
   val_len)` is one past the correct inclusive end index. As with
   `colondoc.cxx`, this `+1` happened to numerically cancel `BUGFIX
   #2`'s `-1` in the common case, which is why neither was
   independently noticed; fixed together with `BUGFIX #2` for the same
   reason documented there. `BUGFIX #4` in source.
5. **Last-field fallback used the wrong length variable** — same as
   `doctype/colondoc.cxx`'s `BUGFIX #3`: `p = &RecBuffer[RecLength]`
   used the buffer's allocated capacity rather than `ActualLength`
   (bytes actually read); harmless once `BUGFIX #1` keeps them equal,
   but wrong on a short `fread()`. `BUGFIX #5` in source.
6. **Unconditional debug `printf()` in `UnifiedName()`** — a stray
   `printf("Medline:UnifiedName called\n");` fired on every call,
   unconditionally, including the normal `Present()` path used to
   build the brief-element headline (three calls per `Present()`, for
   the `TI`/`SO`/`AU` tags). Since `Present()`'s whole purpose is to
   write into an HTTP response body via a CGI frontend (see
   `Isearch-cgi/cgi-util.hxx`'s bug cluster earlier in this file for
   the general class of problem), this would have interleaved debug
   spam directly into `stdout`/the response every time a record was
   presented. Removed. `BUGFIX #6` in source.
7. **Unsigned-length underflow caused a real heap-buffer-overflow in
   `parse_tags()`** — `for (i = 0; i < len - 4; i++)`, where `len` is
   `GPTYPE` (`typedef UINT4 GPTYPE`, unsigned). For any record shorter
   than 4 bytes, `len - 4` underflows to a huge value, turning both of
   `parse_tags()`'s scanning loops into reads far past the small
   allocated buffer. Confirmed with a standalone repro (a 2-byte
   record) before fixing — the first attempt at timing this repro
   conflated ~85s of compile time with the run itself and looked like
   a hang; separating compile and run into two independent steps
   showed the actual failure is immediate and deterministic, not a
   hang:
   ```
   AddressSanitizer: heap-buffer-overflow doctype/medline.cxx:528 in parse_tags
   READ of size 1 ... 1 bytes after 3-byte region
   ```
   Fixed with an explicit short-record guard before the vulnerable
   loops:
   ```cpp
   if (len < 4) {
     t[0] = nullptr;
     return t;
   }
   ```
   `BUGFIX #7` in source. This is the most severe bug in this file —
   unlike bugs #1-#5, which corrupt field boundaries, this one is a
   real out-of-bounds read reachable by simply indexing a MEDLINE-type
   database with a short or malformed record.

Bugs `#1`/`#1b`/`#2`/`#4`/`#5` are covered together by `MEDLINE::
ParseFields extracts full field values, including the last byte of the
file` (a file with no trailing newline, checking exact `FC`-derived
substrings, same approach as the `colondoc.cxx`/`mailfolder.cxx`
regression tests). `#6` is exercised (not asserted on, since
`TESTIDBOBJ` doesn't implement `GetFieldData()`) by `MEDLINE::Present
with the brief element set does not crash`. `#7` is covered by
`MEDLINE::ParseFields does not overflow on a record too short to hold
a tag`, which reproduces the original 2-byte-record crash as a
regression test and was verified against the real, unfixed code path
(via a temporarily-disabled fix) to confirm it reproduces the exact
same ASan report shown above before confirming the fix suppresses it.

Modernization: all `NULL` uses converted to `nullptr`. No live
`sprintf` calls. Added class-level and per-function doc comments.

## src/nfldmgr.cxx

`class NUMERICFLDMGR` manages a set of per-attribute `NUMERICLIST`
field tables, loaded from a database's `<dbName>.fdf` definition file.
It has **no callers anywhere in the current tree**, and `src/Makefile`'s
`OBJ` list (the real production link) doesn't include `nfldmgr.o`
either — this is dead code, though still processed per the standard
pipeline. It also depends on two features the original author left
commented out (`NUMERICLIST::LoadTable()` in `LoadFields()`, and
`NUMERICLIST::Find()` in `Find()`) — noted below rather than guessed
at.

1. **File didn't compile** — `if(fields[NumFields].GetCount==0)`
   references the non-static member function `GetCount` without
   calling it (missing `()`); this is ill-formed C++ (GCC: "invalid
   use of member function ... did you forget the '()'?") and the file
   has evidently never compiled since this line was written. Fixed by
   adding the call: `GetCount()`. `BUGFIX #1` in source. Note:
   `LoadTable()` (the line directly above) is itself commented out, so
   even after this fix `GetCount()` always returns 0 here and every
   field is skipped — a pre-existing incomplete feature, not something
   this turn attempts to finish.
2. **Constructor left `fields`/`MaxEntries` uninitialized, and a second
   `LoadFields()` call would leak the first call's array** — the
   constructor only set `NumFields = 0`. Fixed via a member-initializer
   list (`fields(nullptr), NumFields(0), MaxEntries(0)`). Separately,
   `LoadFields()` reassigns `fields = new NUMERICLIST[counter]`
   unconditionally, so calling it twice on the same `NUMERICFLDMGR`
   would leak whatever the first call allocated; fixed by freeing the
   previous `fields` (a `delete` on `nullptr` is a no-op, so this is
   safe on the very first call too) before reassigning. `BUGFIX #2` in
   source.
3. **`GetResult()` indexed `fields[-1]` with no bounds check** —
   `LocateFieldByAttribute()` documents that it "returns -1 if no field
   for this attribute" (including when no fields were ever loaded), but
   `GetResult()` passed that straight into `fields[i]` unconditionally.
   Not confirmed reachable through any current caller (there are none),
   but free to fix on the public API, same reasoning as
   `src/vidb.cxx`'s `BUGFIX #2`. Fixed with an early return when
   `i==-1`. `BUGFIX #3` in source.
4. **Unbounded `sscanf` into a fixed buffer** —
   `sscanf(Input,"%d %s",&Attribute,TypeString)` has no width limit on
   `TypeString[128]`; a `.fdf` line whose second token is longer than
   127 bytes overflows it. `Input` itself is capped at 255 bytes
   (`fgets(Input,256,fp)`), so the overflow is real for any line with a
   long enough second field. Fixed with an explicit field width:
   `"%d %127s"`. Also converted the two `FullName`/`FieldFile`
   `sprintf`s (unbounded on `dbName`, an `LoadFields()` parameter) to
   `snprintf` with explicit buffer sizes, and `NULL` to `nullptr`
   throughout. `BUGFIX #4` in source.
5. **Destructor's `if(NumFields>0)` guard leaked the `fields` array
   whenever zero fields were successfully loaded** — `LoadFields()`
   allocates `fields` (sized by the `.fdf` file's line count) *before*
   the loop that increments `NumFields`; if every line is skipped (e.g.
   all `TEXT`, or — per `BUGFIX #1`'s note — every field's `GetCount()`
   coming back 0, which is the normal case today) `NumFields` stays 0
   and the old destructor never freed `fields` at all. Confirmed with a
   real ASan leak report from exactly this path (a `.fdf` with one
   `NUMERIC` line) while writing this file's regression tests. Fixed by
   dropping the guard entirely — `delete [] fields;` unconditionally,
   relying on `delete` on `nullptr` being a no-op. `BUGFIX #5` in
   source; also applied to the equivalent guard in `LoadFields()`
   itself (see `BUGFIX #2`).

Covered by `tests/src/test_nfldmgr.cxx`: `LoadFields` returning 0 for a
missing file, `LoadFields` skipping `TEXT`/loading no `NUMERIC` fields
(pins the current incomplete-`LoadTable()` behavior *and* is the
`BUGFIX #5` leak regression, verified leak-free under `make
tests-asan`), `LoadFields` not overflowing on an oversized second token
(`BUGFIX #4`), `GetResult` not crashing on an unmatched attribute
(`BUGFIX #3`), and `Find` returning 0 when no field matches.

Modernization: all `NULL` uses converted to `nullptr`, all `sprintf`
converted to `snprintf`. Added class-level and per-function doc
comments, including a note on `Find()`'s incomplete
`fields[FieldIndex].Find(Key,Relation)` call (also left commented out
by the original author — `Find()` currently always returns 1 once
`Attribute` matches a loaded field, regardless of `Key`/`Relation`).

## src/nlatlon.cxx

Two free functions, `ParseLatToNum`/`ParseLonToNum`, that parse a
`"12.5N"`/`"98.2W"`-style term into a latitude/longitude double. Like
the previous two files this batch (`src/nfldmgr.cxx`,
`src/nlatlon.cxx` itself), neither has any caller anywhere in the
current tree and neither is in `src/Makefile`'s production `OBJ` list
— dead code, still processed per the standard pipeline.

1. **Leak on the invalid-character early return, in both functions** —
   each function `new[]`s a scratch accumulator buffer, then loops over
   the input string; any character outside the recognized set (digits,
   `.`, `-`, and `N`/`S` for latitude or `E`/`W` for longitude) returns
   immediately without freeing it. The success path *does* free it
   (`delete [] accum;` right before the final range check), so only the
   malformed-input path leaked. Confirmed with a real before/after ASan
   comparison: reverting the fix and re-running just this file's tests
   reproduced a clean leak report —
   ```
   Direct leak of 2 byte(s) in 1 object(s) allocated from:
       ... in ParseLatToNum(char*) src/nlatlon.cxx:83
   Direct leak of 2 byte(s) in 1 object(s) allocated from:
       ... in ParseLonToNum(char*) src/nlatlon.cxx:152
   ```
   — restoring the fix (add `delete [] accum;` immediately before each
   early `return`) made it disappear. `BUGFIX #1` (`ParseLatToNum`) and
   `BUGFIX #2` (`ParseLonToNum`) in source.
2. **`isdigit()` called on a plain, possibly-signed `char`** — undefined
   behavior per the C standard for any value not representable as
   `unsigned char` or equal to `EOF`; a byte with the high bit set
   (non-ASCII/extended input) triggers this. Not confirmed to
   misbehave on this platform's libc, but a standard, free hardening
   fix. Fixed by casting to `unsigned char` before the call, in both
   functions. `BUGFIX #3` in source.

Also corrected a stale doc comment: `ParseLonToNum`'s header comment
claimed "Errors are returned as -999," but the actual `LonERROR` macro
(`nlatlon.hxx`) is `-99.0`, matching `LatERROR` — the comment was
simply wrong, not a behavioral bug (a real error return has always
been `-99.0`).

Covered by `tests/src/test_nlatlon.cxx`: normal signed/hemisphere-
suffixed parses for both functions, out-of-range magnitudes, the
invalid-character leak regression (verified leak-free under `make
tests-asan`, `BUGFIX #1`/`#2`), and a high-bit-set byte not crashing
(`BUGFIX #3`).

No live `NULL`/`sprintf` usage to modernize. Added file-level and
per-function doc comments.

## src/stopword.cxx

`class STOPWORD` is a stop-word list backed by a flat fixed-record
memory block, mapped from/flushed to a file — unrelated to
`IDBOBJ::IsStopWord()` despite the similar name. Like the previous
three files this batch, it has no callers anywhere in the current tree
and isn't in `src/Makefile`'s production `OBJ` list either — dead code,
still processed per the standard pipeline. Both bugs below are in
`ImportFromTextFile()`'s per-line trimming logic and were each
confirmed with a real ASan report from this file's own regression
tests, not just reasoned about.

1. **Unsigned-length underflow causing an out-of-bounds read/write,
   and an infinite loop** — the trailing-trim loop,
   `while (!IsAlnum(WordBuffer[n=(strlen(WordBuffer)-1)])) { WordBuffer[n]
   = '\0'; }`, computes `strlen()-1` on an unsigned `size_t`. Once
   enough trailing non-alphanumeric characters were trimmed to make the
   line empty (a blank line, or a line of pure punctuation), the next
   iteration's `strlen()` is 0, so `strlen()-1` underflows and truncates
   to `n==-1` when assigned to the `INT n` — indexing `WordBuffer[-1]`
   out of bounds. Since that write is `'\0'`, which is never
   alphanumeric, the loop condition stays true forever: an infinite
   loop *and* a stack-buffer-underflow on every iteration. Confirmed
   with a real before/after comparison: reverting the fix and running
   just this file's blank-line regression test reproduced a clean ASan
   report (caught before it could hang, since the redzone poisoning
   aborts the process on the very first out-of-bounds access) —
   ```
   AddressSanitizer: stack-buffer-overflow ... src/stopword.cxx:97 in
   STOPWORD::ImportFromTextFile
   ... offset 127 ... underflows this variable [WordBuffer]
   ```
   restoring the fix made it disappear. Fixed by computing the length
   once and guarding the loop on it directly:
   ```cpp
   n = strlen(WordBuffer);
   while (n > 0 && !IsAlnum(WordBuffer[n-1])) {
     WordBuffer[--n] = '\0';
   }
   ```
   `BUGFIX #1` in source.
2. **Overlapping-buffer `strcpy()`, undefined behavior** — the
   leading-trim loop, `strcpy(WordBuffer, WordBuffer + 1)`, shifts the
   buffer's contents left by one byte on every leading non-alphanumeric
   character (e.g. a line like `"(hello)"`). `strcpy()`'s source and
   destination overlap for all but the last byte moved, which the C
   standard leaves undefined; this isn't a hypothetical concern here —
   this exact input (already covered by this turn's punctuation-
   stripping test, no special-casing needed) reproduced a real ASan
   report:
   ```
   AddressSanitizer: strcpy-param-overlap: memory ranges
   [...,...) and [...,...) overlap
   ... in STOPWORD::ImportFromTextFile src/stopword.cxx:112
   ```
   Fixed by switching to `memmove()`, which is explicitly safe for
   overlapping ranges: `memmove(WordBuffer, WordBuffer + 1,
   strlen(WordBuffer))`. `BUGFIX #2` in source.

Also removed a stray, misleading declaration from `stopword.hxx`:
`static int StopwordCompareWords(const void*, const void*);` at file
scope. Because it's declared `static`, every other translation unit
that includes the header gets its own private, internal-linkage
declaration that's never defined there (only `stopword.cxx` defines
its own copy) — harmless as long as nothing calls it, but it triggered
a `-Wunused-function` warning the moment this turn's test file became
the second includer of the header. Not a public-API change: a `static`
function has no external linkage, so no other translation unit could
ever have legitimately used this declaration regardless of whether the
header carried it. `stopword.cxx`'s own `StopwordCompareWords`
definition already precedes all of its uses in that same file, so
nothing needed the forward declaration in the first place.

Covered by `tests/src/test_stopword.cxx`: a nonexistent backing file
starting empty, `AddWords`/`IsStopWord` round-tripping, `AddWords`
skipping an already-present word, `ImportFromTextFile` not hanging on
a blank line or an all-punctuation line (`BUGFIX #1`, verified via the
before/after ASan comparison above), and stripping leading/trailing
punctuation from a real word (`BUGFIX #2`).

No live `NULL`/`sprintf` usage to modernize. Added file-level and
per-function/class doc comments.

## src/sw.hxx

A pure-data header: `const CHR *stoplist[]`, the English stop-word
list. Unlike this batch's previous four files, this one is **not** dead
code — `src/index.cxx` (already processed in an earlier turn) `#include`s
it directly and its `INDEX::IsStopWord()` binary-searches `stoplist[]`
by name (see the doc comment there, added when `index.cxx` was
processed), which is what actually filters stop words out of every
real index built through `IDB`.

1. **A stray accidental array entry broke the sortedness that the
   binary search depends on** — `"found", "",` inserted an extra empty
   string into the list, right after `"found"`. `INDEX::IsStopWord()`'s
   binary search assumes `stoplist[]` is fully sorted (case-
   insensitively, via `StrCaseCmp()`/`strcasecmp()`); an empty string
   sorts *before* every non-empty string, so placing it *after*
   `"found"` violates that invariant. A quick simulation of the exact
   algorithm against the array as it stood found no real word that
   currently fails to be located because of this — the corruption
   happened not to manifest as an observable defect in the *current*
   399-word list — but the invariant itself was still genuinely broken,
   the `"400 words"` comment no longer matched the real count (400
   total entries, only 399 real words), and the entry is obviously
   unintentional. Fixed by removing the stray `""`; the comment was
   corrected to `399 words`. `BUGFIX #1` in source.

Also documented (not changed): `stoplist` is a plain, non-`static`,
non-`inline` array *definition* sitting in a header, so it has external
linkage — `#include`-ing `sw.hxx` from a second `.cxx` file linked into
the same binary would be a one-definition-rule violation (a duplicate-
symbol link error under the `-fno-common` default most current
compilers use). Currently only `index.cxx` includes it; two other files
(`numsearch.cxx`, `geosearch.cxx`) have a `//#include "sw.hxx"` left
commented out, presumably for exactly this reason. Changing this data
definition's linkage (`static`/`inline`, or moving it to a `.cxx`)
would be a real header-shape change and was left alone rather than
guessed at, per GENERAL step 4; the constraint is called out explicitly
in a new file-level comment instead, so the next person who considers
`#include`-ing this header elsewhere doesn't rediscover it via a link
error.

`tests/src/test_sw.cxx` re-parses `src/sw.hxx`'s own source text from
disk (rather than `#include`-ing it a second time, for exactly the
linkage reason above) and directly checks: no empty entries and full
case-insensitive sortedness (together the `BUGFIX #1` regression
test — confirmed to fail against the unfixed data via a real
before/after run, then pass once restored), plus a handful of expected
words present. `INDEX::IsStopWord()` itself continues to be exercised
behaviorally by the pre-existing `tests/src/test_index.cxx`.

No live `NULL`/`sprintf` usage to modernize (pure data). Added a
file-level doc comment; no functions in this file to comment
individually.

## doctype/anzlic.hxx

1. **strcmp() logic error in ParseFields()** — line 372 (now fixed to line 375 after cleanup marker added)
   checked `if (strcmp(*tags_ptr,"/custom"))` without negation. `strcmp()` returns non-zero when
   strings do NOT match, so this condition was true for every non-"/custom" tag and false only for
   the "/custom" end tag itself — inverting the intended logic. The fix is to use `!strcmp()` or
   equivalently `(strcmp(...) == 0)`. This caused incorrect handling of custom tags in ANZLIC
   document parsing. See `BUGFIX #1` in source.

2. **Missing null-pointer check in find_end_tag()** — the function accepted `tag` (second
   parameter) without verifying it was not a null pointer before dereferencing it. While the
   function already checked `t` and `*t`, a null `tag` pointer would cause undefined behavior when
   passed to `strlen()` and subsequent operations. Added safety check at the start of the function.
   See `BUGFIX #2` in source.

### Modernization

- Replaced all `NULL` with `nullptr` throughout both `.hxx` and `.cxx` (14 replacements in `.cxx`).
- Added file-level and method-level doc comments to clarify SGML parsing helper functions.
- Fixed missing parent class include (`sgmlnorm.hxx`) in `anzlic.hxx` to ensure header
  self-containment (discovered during test compilation).

### Tests

Created `tests/doctype/test_anzlic.cxx` with 18 test cases covering:
- Header constant definitions (MAXNESTINGLEN, ANZLIC_ACCEPT_EMPTY_TAGS)
- File extension constants (standard, short, and uppercase variants) — 14 sub-tests
- AMD_Element class operations (set/get tag, start, end positions) — 3 sub-tests
- ANZLIC type definitions and string buffer operations — 2 sub-tests

All tests pass under plain compilation and AddressSanitizer/UndefinedBehaviorSanitizer.
No memory safety issues detected.


## doctype/anzmeta.hxx

1. **strcmp() logic error in ParseFields()** — line 887 (now fixed after cleanup marker added)
   checked `if (strcmp(*tags_ptr,"/custom"))` without negation. `strcmp()` returns non-zero when
   strings do NOT match, so this condition was true for every non-"/custom" tag and false only for
   the "/custom" end tag itself — inverting the intended logic. This caused incorrect handling of
   custom tags in ANZMETA document parsing, paralleling the same bug found in doctype/anzlic.cxx
   (BUGFIX #1 there). The fix is to use `!strcmp()`. See `BUGFIX #1` in source.

### Modernization

- Replaced all `NULL` with `nullptr` throughout .cxx file (29 replacements total across strtok,
  pointer comparisons, and casts).
- Added file-level doc comment to mark cleanup completion.
- Fixed missing parent class include (`sgmlnorm.hxx`) in `anzmeta.hxx` to ensure header
  self-containment (discovered during test compilation).

### Tests

Created `tests/doctype/test_anzmeta.cxx` with 12 test cases covering:
- Header constant definitions (ANZ_ACCEPT_EMPTY_TAGS, MAXNESTINGLEN, BRIEF_MAGIC) — 3 sub-tests
- File extension constants (standard, short, and uppercase variants) — 18 sub-tests
- ZMD_Element class operations (set/get tag, start, end positions, array operations) — 4 sub-tests
- ANZMETA type definitions and string buffer operations — 2 sub-tests

All tests pass under plain compilation and AddressSanitizer/UndefinedBehaviorSanitizer.
No memory safety issues detected.

## doctype/doc_conf.hxx

A pure-macro configuration header ("Local Configurations for BSn
doctypes") — no functions or classes. `doctype/mailfolder.cxx` is its
only current includer in the tree, and only actually relies on the
`BSN_EXTENSIONS`/`BRIEF_MAGIC` macros at the bottom; every other
doctype file that references `USE_UNIFIED_NAMES` or a per-doctype
override defines its own local fallback rather than including this
header.

1. **No include guard at all** — every other `.hxx` in this tree uses
   the standard `#ifndef X_HXX`/`#define X_HXX`/`#endif` pattern; this
   file had none. Harmless today only because `mailfolder.cxx` (the
   sole current includer) includes it exactly once — a latent
   double-inclusion hazard for any future includer, and inconsistent
   with the rest of the tree. Fixed by wrapping the whole file in
   `#ifndef DOC_CONF_HXX`/`#define DOC_CONF_HXX`/`#endif`. `BUGFIX #1`
   in source.
2. **`USE_UNIFIED_NAMES` defined twice, identically** — once under a
   "Bibliographic Formats" comment, then again under a second,
   redundant "General" comment further down, both `#define
   USE_UNIFIED_NAMES 1`. A same-value macro redefinition is legal C++
   (no compile error), so this was never an observable defect, but it's
   an obvious copy-paste duplicate. Removed the second definition.
   `BUGFIX #2` in source.

Also documented, not changed: `BRIEF_MAGIC` is only defined when
`BSN_EXTENSIONS < 1`; if `BSN_EXTENSIONS` were ever set to `1`,
`BRIEF_MAGIC` would never get defined at all. Not unique to this file —
every `doctype/*.hxx` with its own local `BSN_EXTENSIONS` fallback
(`anzlic.hxx`, `cipc.hxx`, `cipp.hxx`, `fgdc.hxx`, `html.hxx`, and
others) has this exact same shape, and `BSN_EXTENSIONS` is never
actually set to `1` anywhere in the current tree — a tree-wide
incomplete-BSn-mode design choice, not a defect specific to this file.

`tests/doctype/test_doc_conf.cxx` `#include`s `doc_conf.hxx` twice in
the same translation unit — the direct regression test for `BUGFIX #1`
— then checks every documented macro's default value, including the
three per-doctype `*_UNIFIED_NAMES` overrides correctly falling back to
the single (post-`BUGFIX #2`) `USE_UNIFIED_NAMES` definition.

No live `NULL`/`sprintf` to modernize (pure macros, no code). Added a
file-level doc comment; no functions to comment individually.

## doctype/bibtex.cxx

`class BIBTEX` splits a file into `"}"`-terminated BibTeX entries
(`ParseRecords()`) and extracts each entry's `title = "..."` value as
its sole indexed field (`ParseFields()`). Found and fixed five real
bugs — three leaks (one severe: the whole file, every call), a
functional bug that added a bogus field to every title-less record, and
a field-boundary bug that included the delimiting quote characters in
the indexed text.

1. **`ParseRecords()` leaked the entire file buffer on every successful
   call** — `RecBuffer` (sized to the whole file) was allocated near
   the top of the function and freed on every early-return error path,
   but the function's normal/success path (after splitting the file
   into records) just fell off the end without ever freeing it. Since
   `ParseRecords()` runs once per file during indexing, this leaked a
   whole file's worth of memory per file indexed — the most severe leak
   in this file. Confirmed leak-free after the fix via `make
   tests-asan`. Fixed by adding `delete [] RecBuffer;` at the end of
   the function. `BUGFIX #1` in source.
2. **`ParseFields()` leaked a `STRING::NewCString()` buffer on every
   call** — `file = fn.NewCString();` was allocated purely to pass to
   two `perror(file)` diagnostic calls, and never freed on any path
   (including success). `fopen(fn, "rb")` two lines above it already
   relies on `STRING`'s non-allocating `operator const char*()`
   conversion, so the allocation wasn't even necessary — fixed by
   deleting the `file` variable entirely and calling `perror(fn)`
   directly, the same pattern already used for `fopen`. `BUGFIX #2` in
   source.
3. **Two early-return paths inside the title-parsing state machine
   leaked both `RecBuffer` and the heap-allocated `DFT`** — "Cannot
   find quote mark after title." and "couldn't find ending quote."
   both `return`ed without freeing `RecBuffer` or `delete`ing `pdft`
   (allocated via `new DFT()` earlier in the function). Fixed by adding
   `delete pdft; delete [] RecBuffer;` before each of the two
   `return`s. `BUGFIX #3` in source.
4. **A malformed/absent title added a bogus zero-length "title" field
   to every record** — `val_start` only stays at its `0` initializer
   when "title" is never found anywhere in the record at all (both
   malformed-title failure paths above already `return` before
   reaching the field-adding code, and a legitimately parsed title's
   quote position is always `>= 5`, never `0`). The field-adding block
   used to run unconditionally regardless, so every title-less BibTeX
   record got a spurious "title" DF entry with `FieldStart=0,
   FieldEnd=0` — corrupting title-based search/display for any record
   without one. Fixed by only adding the field when `val_start != 0`.
   `BUGFIX #4` in source.
5. **The stored title field included its own delimiting quote
   characters** — `val_start`/`val_end` are the positions of the
   opening and closing `"` characters themselves (that's what the
   quote-searching loops set them to), but `fc.SetFieldStart(val_start);
   fc.SetFieldEnd(val_end);` used them directly, so a real callback
   reading the field back via its `FC` coordinates got
   `"A Great Title"` — quote marks included — instead of `A Great
   Title`. Confirmed via a regression test asserting the exact
   extracted substring. Fixed by excluding both quotes:
   `fc.SetFieldStart(val_start + 1); fc.SetFieldEnd(val_end - 1);`.
   `BUGFIX #5` in source. Noted, not pursued: a literal `title = ""`
   (empty title) is a pre-existing, unhandled degenerate case either
   way — `val_end` would equal `val_start+1`, producing an inverted
   (not just empty) `[start, end]` range — but this is vanishingly rare
   input not worth guessing a convention for.

Also fixed (compile warnings, not correctness bugs, surfaced because
this was this file's first turn through `-Wall -Wextra`): a handful of
signed/unsigned comparison warnings (`ParseRecords()`'s `int lastBrace`
compared against `GPTYPE i`, cast at the comparison site;
`ParseFields()`'s loop counter retyped from `int` to `GPTYPE` to match
`ActualLength`, both variables always non-negative in practice), and a
`-Wdangling-else` warning in the nested `if` cascade that searches for
`"title"`, resolved with explicit braces without changing which `if`
the `else` binds to.

`tests/doctype/test_bibtex.cxx` covers: `ParseRecords()` splitting a
two-entry file into two records with correct boundaries, including the
last-record-extended-to-EOF behavior (`BUGFIX #1`'s regression
coverage, verified leak-free under `make tests-asan`); `ParseFields()`
extracting a real title's exact text (`BUGFIX #5`); adding no field at
all for a title-less record (`BUGFIX #4`); and not crashing or leaking
on both malformed-title shapes (missing opening quote, missing closing
quote — `BUGFIX #3`, via `DocTypeAddRecord()` and `TESTIDBOBJ` from the
same pattern used by `test_colondoc.cxx`/`test_medline.cxx`).
`Present()` itself isn't exercised directly — its `"F"` path delegates
to `RESULT::GetRecordData()` (crashes on a default-constructed
`RESULT`, for reasons unrelated to `BIBTEX`), and its non-`"F"` path's
first check always short-circuits via `IDBOBJ`'s own
`DfdtGetTotalEntries()` default (`TESTIDBOBJ` doesn't override it);
building a real, exercisable `RESULT`/`IDBOBJ` pair was judged out of
scope for this file's own turn.

No live `NULL`/`sprintf` to modernize. Added class-level and
per-function doc comments.

## doctype/cipc.cxx

`class CIPC` (`: public SGMLNORM`) is the NASA/CIP Collection metadata
DOCTYPE. This turn found five real bugs, including the most severe
find of this batch: a case-mismatch that made an entire parsing branch
permanently unreachable, and a confirmed null-pointer-dereference
crash (verified with a real before/after SEGV repro, not just reasoned
about).

1. **`ParseDate()`'s `<StartDate>`/`<EndDate>` interval parsing was
   unreachable dead code** — `Hold.UpperCase()` runs unconditionally
   before the `<StartDate>` search, and is never reset to mixed case
   before it; searching an all-uppercase string for the mixed-case
   needle `"<StartDate>"` (and `"<EndDate>"`) can never match via
   `strstr()`. Every call that reached this point silently fell
   through to the final `else` branch and returned `DATE_ERROR` for
   both `*fStart` and `*fEnd` — even for a perfectly well-formed
   `<StartDate>2020</StartDate><EndDate>2021</EndDate>` input. This
   means interval-date parsing has never worked. Confirmed via a
   regression test asserting an actual successful parse (not just "no
   crash"). Fixed by uppercasing the search needles instead
   (`"<STARTDATE>"`/`"</STARTDATE"`/`"<ENDDATE>"`/`"</ENDDATE"`,
   matching the sibling `<CALDATE>` branch's already-correct
   convention; `strlen()` calls using the mixed-case literal are
   unaffected since the length is identical either way). Bundled with
   a second, smaller fix in the same branch: unlike the `<CALDATE>`
   branch above and the `<EndDate>` branch below (both of which
   `return` immediately on a missing closing tag), the `<StartDate>`
   branch used to fall through into `Hold.EraseAfter(End-1)` with
   `End==0` (a `STRINGINDEX` underflow to `SIZE_MAX` — harmless only
   because `EraseAfter()` itself already bounds-checks and no-ops) and
   then kept going, searching for `<EndDate>` despite already having
   flagged the record malformed, leaving `*fEnd` unset on that path.
   `BUGFIX #1` in source.
2. **Same two bugs, duplicated in `ParseDateRange()`** — a
   near-duplicate function with the identical `Hold.UpperCase()`-then-
   mixed-case-search shape and the identical missing-`return`.
   `BUGFIX #2` in source.
3. **Unguarded `Nested.Top()` — a real, confirmed null-pointer-
   dereference crash** — `ParseFields()`'s closing-tag handling called
   `Nested.Top()` and immediately dereferenced the result
   (`pTmp->get_tag()`) with no `Nested.GetSize() != 0` guard, unlike
   its two sibling call sites in the very same function. A closing tag
   with nothing open on the stack (e.g. a document whose first real
   tag is an unmatched `</foo>`) makes `GSTACK::Top()` return `nullptr`
   (per its own already-fixed, already-safe behavior — see
   `docs/BUG_CATALOG.md#srcgstackhxx`), and this call dereferenced it.
   That earlier fix's comment claimed every real caller in the tree,
   including this file, "only reach[es] Top()/Pop() after confirming
   GetSize() != 0 first" — true for two of this function's three call
   sites, but not this one; this finding corrects that claim. Confirmed
   with a real before/after SEGV repro:
   ```
   AddressSanitizer: SEGV ... READ memory access
       #0 STRING::Equals(STRING const&) const src/string.cxx:492
       #2 CIPC::ParseFields(RECORD*) doctype/cipc.cxx:982
   ```
   reverting the fix and running just this file's stray-closing-tag
   regression test reproduced it exactly; restoring the fix cleared it.
   Fixed by wrapping the match-and-pop logic in the same
   `Nested.GetSize() != 0` guard its siblings already use — a stray
   closing tag with nothing to match is simply ignored. `BUGFIX #3` in
   source.
4. **Leaked `CIPC_Element` on overlapping (non-LIFO) tags** — a
   `CIPC_Element` is only `Pop()`'d and `delete`'d when a closing tag
   matches whatever's currently on *top* of `Nested`. Input like
   `<A><B></A></B>` pairs `A` with a real `</A>` and `B` with a real
   `</B>` (`find_end_tag()` matches both, so both get `Nested.Push()`'d),
   but they close in the wrong order relative to each other: when
   `</A>` is processed, `B` is on top, not `A`, so nothing pops and
   `A`'s element is stuck on `Nested` for the rest of the function —
   and since `Nested` is a local `GSTACK` with no destructor-side
   cleanup of the opaque element pointers it holds, that leaks. Fixed
   by draining and `delete`ing whatever's left on `Nested` before
   `ParseFields()` returns. Confirmed leak-free under `make
   tests-asan` with this exact overlapping-tag input. `BUGFIX #4` in
   source.
5. **`LoadFieldTable()` could crash on an empty FIELDTYPE file** —
   `IsFile()` only checks that the configured `-o fieldtype=<filename>`
   file exists, not that it has content. The original `do`-`while`
   loop unconditionally ran `Field_and_Type = pBuf;` once before ever
   checking `pBuf`; if the file is empty (or has no non-newline
   content), `strtok()` returns `nullptr` on the very first call, and
   `STRING::operator=(const CHR*)` calls `strlen()` on it
   unconditionally — a null-pointer-dereference crash. Fixed by
   converting to a `while (pBuf)` loop that checks before every
   iteration, including the first. `BUGFIX #5` in source.

Also fixed while bringing this file to a clean `-Wall -Wextra` build
for the first time (its first turn through this pipeline): removed an
unused `DOUBLE Left;` in `ParseGPoly()`, an unused `INT n;` in
`Present()`, and a genuinely dead block in `ParseFields()` (computed
`Nested.Top()` into `pTmp` and a `LastEnd` comparison, then never used
either) whose removal surfaced a follow-on `LastEnd` unused-variable
warning, so `LastEnd` was removed too since that dead block was its
only reader; cast `int lastBrace`-style comparisons were not needed
here, but a nested `if (RecBuffer[i+3]=='l')`-shape dangling-else
risk was avoided by construction. `NULL` converted to `nullptr` at
every live call site (comment-only mentions left alone). Also
documented, not changed: `store_attributes()` is declared as a CIPC
member in the header but never defined in this file — `ParseFields()`
always calls `SGMLNORM::store_attributes()` explicitly instead, so the
declaration is inert (never ODR-used).

`fgdc.cxx` was added to `TEST_ENGINE_DOCTYPE_SRCS` alongside
`cipc.cxx` purely to satisfy `GetNumericValue()` at link time —
`cipc.cxx` (like `cipp.cxx`) has its own copy of that function
commented out, relying on `fgdc.cxx`'s live definition instead;
`fgdc.cxx` itself is not otherwise exercised or processed by this
turn. (`fgdc.cxx`'s own `ParseFields()`, seen only in passing while
confirming it compiles, appears to share several of these same bug
shapes — worth a close look whenever its own turn comes up.)

`tests/doctype/test_cipc.cxx` covers: `ParseDate`/`ParseDateRange`
successfully parsing a well-formed interval (the direct `BUGFIX #1`/
`#2` regression, since before the fix this could never succeed at
all) and correctly erroring out on a missing closing tag; `ParseFields`
not crashing on a stray unmatched closing tag (`BUGFIX #3`, confirmed
via the real before/after SEGV repro above) and not leaking on
overlapping tags (`BUGFIX #4`); and `LoadFieldTable` not crashing on
an empty FIELDTYPE file (`BUGFIX #5`) while still loading real entries
correctly.

