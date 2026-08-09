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

## src/fc.cxx

1. **Indeterminate `FieldStart`/`FieldEnd` in the default constructor**
   — `FC::FC() {}` had an empty body; the same "indeterminate primitive
   member" category already fixed in `RESULT`'s/`NUMERICFLD`'s/
   `NUMERICLIST`'s constructors elsewhere in this tree. Fixed by
   zero-initializing both. See `BUGFIX #1` in source; regression test
   in `tests/src/test_fc.cxx`.
2. **`Write()`/`Read()`'s signed/unsigned mismatch, flagged above at
   `fc.hxx`'s own turn** — fixed as described there (`%u` in `Write()`,
   `GetLong()` in `Read()`), but with an important correction to that
   earlier note after actually reproducing it on this turn: it claimed
   values above `INT_MAX` "would print as negative" and "fail to
   round-trip." Both are true only in isolation. Tried as a standalone
   repro (`tests/src/test_fc.cxx`, temporarily reverting first the
   whole fix, then just `Read()` back to `GetInt()` while leaving
   `Write()` on `%u`): **the original, never-touched code actually
   round-trips large values correctly on this platform**, including the
   "half-fixed" combination. `%d` of an unsigned value reinterprets its
   bit pattern as negative; `atoi()` (via `strtol()` into a 64-bit
   `long`, then narrowed to `int`) reinterprets that same bit pattern
   back the same way; assigning the negative `int` back to the unsigned
   `GPTYPE` member wraps it a third time — three complementary
   two's-complement reinterpretations that happen to cancel out
   losslessly on this specific glibc/x86-64 build. This is genuinely
   undefined behavior per the C standard either way (`%d` given an
   unsigned argument; `atoi()` overflowing `INT_MAX`), not guaranteed by
   any standard to keep working — the same "silent only because of how
   this specific platform happens to behave, fragile, not guaranteed"
   category as this file's own header-self-containment fix above — but
   it is *not* a demonstrated data-corruption bug on this build, and the
   catalog entry above overstated it as one. Fixed anyway, for
   portability/standards-correctness rather than a reproduced failure:
   `%u` in `Write()` and `GetLong()` in `Read()` remove the undefined
   behavior outright instead of relying on it happening to cancel out.
   See `BUGFIX #2` in source; regression test in `tests/src/test_fc.cxx`
   (passes both before and after the fix, for the reasons above — its
   value is pinning the round-trip contract, not detecting a failure).
   `make tests`/`make tests-asan` pass clean (707 test cases, 2367
   assertions).

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

## src/fct.cxx

**Correction to the `Write()`/`Read()` note above, checked on this
file's own turn:** `TotalEntries` here can never actually reach
`INT_MAX` in the first place — `VLIST::GetTotalEntries()` (the only
source of this value; `FCT` keeps no counter of its own, just `Next`/
`Prev` links) is itself declared to return `INT`, not `SIZE_T`, so the
count is signed-32-bit-bounded before `Write()` ever widens it into a
`SIZE_T` local. Unlike `FieldStart`/`FieldEnd` (`GPTYPE`, genuinely
capable of spanning the full unsigned 32-bit range as byte offsets into
a large corpus), a table's *entry count* can't reach a value `GetInt()`
would mishandle, so no fix was needed here — left as-is rather than
adding a check for a state the code can't reach.

Two bugs found and fixed, both already flagged as deferred findings at
other files' turns:

1. **`operator=` had no self-assignment guard**, already flagged as
   deferred at `src/df.cxx`'s turn
   (`docs/BUG_CATALOG.md#srcdfcxx`) with an explicit recommendation to
   pick it up via `/process src/fct.cxx` — `Clear()` ran before
   `OtherFct.GetTotalEntries()` was read, so `fct = fct;` cleared
   itself and then "copied" its own now-empty contents back, silently
   losing every entry. Same shape as `DF`'s/`ATTRLIST`'s/`DFDT`'s/
   `DFT`'s own fixes for this pattern elsewhere in this tree. Confirmed
   with a standalone repro (temporarily reverting the fix, then
   rerunning `tests/src/test_fct.cxx`'s new self-assignment test): the
   table dropped from 2 entries to 0 after `fct = fct;`. Fixed with a
   `this == &OtherFct` guard. See `BUGFIX #1` in source; regression
   test in `tests/src/test_fct.cxx`.
2. **`FctFcCompare` (the `qsort` comparator `SortByFc()` uses) silently
   misorders on large field offsets**, already flagged as a
   found-but-out-of-scope note at `fct.hxx`'s own turn
   (`docs/BUG_CATALOG.md#srcfcthxx`) — `((FC*)x)->GetFieldStart() -
   ((FC*)y)->GetFieldStart()` subtracts two `GPTYPE` (unsigned) values
   and narrows the result to the `int` `qsort` expects. Unlike
   `src/fc.cxx`'s `Write()`/`Read()` finding above (which turned out to
   round-trip correctly on this platform via complementary
   two's-complement reinterpretation at each step), this one has no
   cancelling second step and is a real, confirmed misordering: a
   standalone repro (`0u - 3000000000u`, narrowed to `int`) came out as
   `+1294967296` — positive, the wrong sign for `0 < 3000000000` — and
   `tests/src/test_fct.cxx`'s new large-offset `SortByFc()` test failed
   the same way against the pre-fix code (a 3-billion-offset entry
   sorted before a 0-offset one). Fixed by comparing `GetFieldStart()`
   directly (`<`/`>`) instead of subtracting, avoiding the overflow
   entirely regardless of offset magnitude. See `BUGFIX #2` in source;
   regression test in `tests/src/test_fct.cxx`. `make tests`/`make
   tests-asan` pass clean (709 test cases, 2372 assertions).

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

3. **`operator=` had no self-assignment guard** (found on `dft.cxx`'s
   own later turn — missed during the copy-constructor fix above,
   unlike `ATTRLIST`'s/`DFDT`'s sibling turns, which caught this same
   pattern in their own `operator=`s at the time) — `delete [] Table;
   Init();` ran before `OtherDft.GetTotalEntries()` was read, so `dft =
   dft;` (`this == &OtherDft`, the same object) saw an already-emptied
   table and copied nothing back, silently wiping it. Confirmed with a
   standalone repro (temporarily reverting the fix, then rerunning
   `tests/src/test_dft.cxx`'s new self-assignment test): the table
   dropped from 2 entries to 0 after `dft = dft;` — a real, silent
   data-loss bug, not a crash. Fixed with a `this == &OtherDft` guard,
   identical to `ATTRLIST`'s/`DFDT`'s own fix for this pattern. See
   `BUGFIX #3` in source; regression test in `tests/src/test_dft.cxx`.
   `make tests`/`make tests-asan` pass clean (705 test cases, 2362
   assertions).

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
  **Resolved during `src/record.cxx`'s own turn below**: like
  `src/fc.cxx`'s identical case, confirmed via a standalone repro that
  this already round-trips correctly on this platform — fixed anyway
  for portability, not because of a demonstrated failure.

## src/record.cxx

The deferred `Write()`/`Read()` finding above, and one more found on
this file's own dedicated turn:

1. **`Write()`/`Read()` used a signed format specifier and signed
   parse for unsigned `GPTYPE` fields** — `fprintf(fp, "%d\n",
   RecordStart)` (and `RecordEnd`) against `STRING::GetInt()`
   (`atoi()`, signed 32-bit) on the read side. Confirmed via a
   standalone repro that this round-trips correctly *today* on this
   platform (`RecordStart`/`RecordEnd` set above `INT_MAX`, written,
   read back, matched exactly) — printf's signed reinterpretation of
   the unsigned bit pattern and `atoi()`'s int-to-unsigned assignment
   wrap modulo 2^32 the same way, the identical "works by platform
   coincidence" mechanism already found and fixed in `src/fc.cxx`'s
   `Write()`/`Read()`. Not a demonstrated failure, but still
   unspecified/implementation-defined behavior, not a guarantee. Fixed
   to `%u`/`GetLong()` (`LONG` = 64-bit `long` here, wide enough to
   parse the full unsigned 32-bit range with no narrowing) for the same
   reason `fc.cxx` was: portability, not correctness of a live bug. See
   `BUGFIX #1` in source. Verified via the same repro (still passes
   after the fix) and a new `tests/src/test_record.cxx` regression test
   using a value above `INT_MAX`.

2. **`RECORD(STRING&, STRING&)` left `RecordStart`/`RecordEnd`
   indeterminate** — unlike the default constructor (which explicitly
   zeroes every member), this one only sets `PathName`/`FileName`.
   `RecordStart`/`RecordEnd` are plain `GPTYPE` members with no default
   member initializer, so they're only safely zeroed when a constructor
   explicitly does so (`Key`/`DocumentType` don't have this problem —
   `STRING`'s own default constructor already runs for them regardless
   of this constructor's body). No live call site constructs a `RECORD`
   with this constructor today (confirmed via `grep`: every real
   `RECORD Record;` in this tree uses the default constructor), so this
   is latent, not an active bug — the same "indeterminate primitive
   member" category already fixed for `RESULT`/`NUMERICFLD`/
   `NUMERICLIST`/`MERGEUNIT` elsewhere in this project. Fixed by
   zero-initializing both, matching the default constructor. See
   `BUGFIX #2` in source. **Verification caveat**: unlike this
   project's usual memory-safety bugs, a revert-and-repro check doesn't
   reliably demonstrate this one — reverting the fix and rerunning the
   test suite under ASan+UBSan still passed, because the indeterminate
   stack memory happened to read back as zero in that run. Neither ASan
   nor UBSan catch uninitialized-*value* reads (as opposed to
   out-of-bounds or use-after-free); that would need MemorySanitizer,
   which isn't part of this project's toolchain. The fix is still
   correct and worth keeping — this is a limitation of the
   verification technique for this specific bug class, not evidence
   the bug wasn't real.

No `NULL`/`sprintf` beyond what `BUGFIX #1` already modernized, zero
warnings under `-Wall -Wextra`. `make tests`/`make tests-asan` pass
clean (733 test cases, 2832 assertions).

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

## src/rcache.cxx

`BUGFIX #2-3` above were already applied to this file during
`rcache.hxx`'s turn (the eviction fix and `Fetch()`'s bounds check).
This file's own dedicated turn found nothing further. Specifically
checked: the constructor doesn't initialize `ResultSet[MAXCACHE]`
(pointers) or `Relation[MAXCACHE]` (ints), leaving them indeterminate —
but both are only ever read for indices `< Count`, and `Add()` always
writes `ResultSet[MinPos]`/`Relation[MinPos]` before `Count` advances
past that index, so the indeterminate tail is never read before being
written first (the same `Count`-gated-array invariant already relied
on throughout this tree, e.g. `IRSET`/`ATTRLIST`). `Term`/`FieldName`/
`DBName` (`STRING[MAXCACHE]`) don't have this question at all — array
members of class type are always default-constructed regardless of
`Count`. No `NULL`/`sprintf`, zero warnings under `-Wall -Wextra`. No
test changes: existing coverage in `tests/src/test_rcache.cxx` already
exercises `Check`/`Add`/`Fetch`/eviction. `make tests`/`make
tests-asan` pass clean (732 test cases, 2827 assertions, unchanged
from before this turn).

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

## src/operand.cxx

`BUGFIX #1` above (header self-containment) needed no change here.
This file's own dedicated turn: a full re-read of all four methods
(`OPERAND()`, `operator=()`, `SetAttributes()`, `GetAttributes()`,
`~OPERAND()`) found no new bug. Specifically checked and confirmed
sound: `operator=()` has no self-assignment guard of its own
(`OtherOp.GetAttributes(&Attributes);` unconditionally), but this is
safe now — it relies entirely on `ATTRLIST::operator=()`'s own guard
underneath, which was latent/unfixed when `operand.hxx`'s own turn
flagged this exact chain as a deferred risk (and which `src/sterm.hxx`'s
turn went on to confirm was reachable and real, self-assigning a live
`STERM` through the identical path), but has since been fixed
(`src/attrlist.cxx`'s `BUGFIX #2`). `OPOBJ`'s own `Next` member (used
for `OPSTACK`'s intrusive linked-list bookkeeping) is deliberately not
touched by this `operator=` — it's `private` to `OPOBJ` (not even
visible to `OPERAND`), so copying it isn't just skipped, it's
inaccessible by design, which is the correct behavior for a member
that represents an object's position in an unrelated data structure,
not its value. No `NULL`/`sprintf`, zero warnings under `-Wall
-Wextra` attributable to this file. Added the previously-deferred
self-assignment regression test now that it's safe to write (see
`tests/src/test_operand.cxx`'s own comment for the history). `make
tests`/`make tests-asan` pass clean (731 test cases, 2826 assertions).

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

## src/rset.cxx

`BUGFIX #2-6` above were already applied to this file during
`rset.hxx`'s turn (copy constructor/`operator=`, `GetEntry`'s
sign-compare, the `SaveTable`/`LoadTable` rewrite, `SortByScore`/
`SortByKey`'s comparator fixes). This file's own dedicated turn found
one more, more severe than any of them:

7. **`SetEntry()` had no bounds check at all — confirmed wild-pointer
   heap-buffer-overflow** (most severe finding in this file): unlike
   `GetEntry()` (bounds-checked, see `BUGFIX #3`), `SetEntry()` wrote
   `Table[x-1] = ResultRecord;` for any caller-supplied `x`, with
   nothing validating it against `[1, TotalEntries]` or even
   `MaxEntries`. Confirmed real with a standalone repro: a 1-entry
   `RSET` (100-entry initial capacity) called with `SetEntry(500, ...)`
   produced not just an out-of-bounds write but a wild-pointer
   heap-buffer-overflow — `RESULT::operator=`, writing into the
   out-of-range slot, tried to free/reuse whatever garbage
   `STRING::Buffer` pointer happened to be sitting in that unrelated
   heap memory. This file's only real caller, `IRSET::Fill()`
   (`src/irset.cxx`), is safe today only by calling convention: every
   call site (`Isearch-cgi/api_search.cxx`, `src/Isearch.cxx` ×2,
   `src/zsearch.cxx`, `Isearch-cgi/isrch_html.cxx`,
   `Isearch-cgi/isrch_srch.cxx` — all six checked) first sizes the
   destination `RSET` via `GetRset()` with the exact same range
   `Fill()` then iterates, so `SetEntry()`'s index never actually
   exceeds the pre-grown table in practice — a real, exploitable
   contract gap on a public method, not (yet) an active one, the same
   category as `src/rcache.cxx`'s `Fetch()` (`BUGFIX #3` there) and
   every other indexed accessor in this tree. Fixed by matching
   `GetEntry()`'s own bounds check and no-op-on-out-of-range
   convention. See `BUGFIX #7` in source. Verified fixed: the same
   repro now exits cleanly, and `tests/src/test_rset.cxx` has dedicated
   `SetEntry` regression tests (in-range overwrite, and the
   out-of-range case that used to crash) — `SetEntry` had no test
   coverage at all before this turn.

No `NULL`/`sprintf`, zero warnings under `-Wall -Wextra`. `make
tests`/`make tests-asan` pass clean (735 test cases, 2836 assertions).

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

## src/irset.cxx

5. **`Expand()`'s multiplicative doubling gets permanently stuck at
   zero → confirmed heap-buffer-overflow** — `Expand()` called
   `Resize(TotalEntries*2)` to grow capacity. Once `MaxEntries` reaches
   0 (reachable via `CleanUp()`/`Resize()` on an empty `IRSET`, unlike
   every sibling class's additive `Resize(Count+N)` growth pattern),
   `0*2` is still `0`, so every subsequent `AddEntry()`/`FastAddEntry()`
   wrote one element past a zero-size allocation instead of triggering
   a real grow. Confirmed with a standalone repro under ASan:
   `CleanUp()` on an empty `IRSET` followed by `AddEntry()` reported a
   heap-buffer-overflow at the write in `AddEntry()`. Fixed by falling
   back to a minimum of 1 when `TotalEntries` is 0, breaking the
   stuck-at-zero cycle; doubling from there recovers in a handful of
   calls. See `BUGFIX #5` in source. Verified fixed: the same repro now
   passes cleanly under ASan, and `tests/src/test_irset.cxx` has a
   dedicated regression test (`IRSET Expand recovers after CleanUp on
   an empty set`).

6. **`And()`/`AndNot()` set `TotalEntries` from the pre-merge match
   count, not `Table`'s actual post-merge size → confirmed
   heap-buffer-overflow** — both functions accumulate matches into a
   scratch `IRSET MyResult` via `FastAddEntry()` (which, unlike
   `AddEntry()`, does *not* dedupe by `MdtIndex`), call
   `MyResult.MergeEntries(0)` to collapse any duplicates into `Table`,
   then steal that `Table` — but set `this->TotalEntries = count`, the
   *pre*-merge count of matches added, not `MyResult`'s actual
   post-merge entry count. Whenever `OtherIrset` contained duplicate
   `MdtIndex` entries (reachable through its own `FastAddEntry()` calls,
   which bypass dedup the same way), `MergeEntries()` collapses them in
   `Table` but `TotalEntries` kept the higher pre-merge count, so any
   `GetEntry()` at the overstated tail read past `Table`'s real
   allocation. Confirmed with a standalone repro under ASan: `And()`ing
   an `IRSET` against another containing two duplicate-`MdtIndex`
   entries produced a heap-buffer-overflow read in
   `IRESULT::operator=` via `GetEntry()`. Fixed by using
   `MyResult.GetTotalEntries()` (captured before `StealTable()` resets
   it) instead of `count`, in both functions. See `BUGFIX #6` in
   source. Verified fixed: the same repro now passes cleanly under
   ASan, and `tests/src/test_irset.cxx` has dedicated regression tests
   for both `And()` and `AndNot()`.

Also checked and left alone: the `#else`/`#ifdef MULTI` block (lines
~697–800) contains an older, alternate implementation of `And()`/
`AndNot()` plus a `CompressTable()`-style compaction loop. `-DMULTI` is
commented out in the top-level `Makefile` (`#CFLAGS=-O2 -DUNIX
-DMULTI`), so this branch is genuinely dead code in this build —
confirmed by grepping the Makefile rather than assumed. Not analyzed
further; out of scope unless `MULTI` is ever turned on.
`IrsetScoreCompare()`'s tie-break subtracts two signed `INT`
`GetMdtIndex()` values directly, the same theoretical-overflow-only
pattern left alone in `INTLIST::SortGPCmp` (see `src/intlist.cxx`
above) rather than `FCT::FctFcCompare`'s confirmed unsigned-narrowing
bug (see `src/fct.cxx` above) — no demonstrated failure, left as-is
per this project's "don't claim a bug without a reproduced failure"
standard.

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
  **Resolved during `src/opstack.cxx`'s own turn below**: traced —
  confirmed safe, not a live bug.

## src/opstack.cxx

`BUGFIX #1-3` above were already applied to this file during
`opstack.hxx`'s turn (header-only: includes, the copy constructor
declaration, and the destructor-leak fix's implementation — see below).
This file's own dedicated turn found one more, and resolved the
design-smell question `opstack.hxx`'s own turn left open:

4. **`operator=()` had no self-assignment guard → confirmed silent
   data loss** — the "pop everything off this stack" loop at the top
   unconditionally drains and deletes every node in `*this` *before*
   `OtherOpstack.Head` is ever read. On self-assignment (`x = x;`,
   where `OtherOpstack` *is* `*this`), that loop drains and deletes
   the source's own nodes too, so by the time the "push OtherOpstack's
   ops onto this stack" loop runs, `OtherOpstack.Head` is already
   `nullptr` — every entry silently vanishes rather than being
   preserved. The same "drain `*this` before reading `OtherObj`'s
   state" shape already fixed for `ATTRLIST`/`IRSET`/`DFT`/`STERM`'s
   `operator=` earlier in this project. Confirmed real with a
   standalone repro: self-assigning a 2-entry `OPSTACK` (through the
   same named variable, `stack = stack;` — no polymorphic indirection
   needed here, since `OPSTACK` isn't itself part of the `OPOBJ`
   hierarchy) left it with 0 entries. Fixed with an early return when
   `this == &OtherOpstack`, mirroring every other class's fix. See
   `BUGFIX #4` in source. Verified fixed: the same repro now preserves
   both entries, and `tests/src/test_opstack.cxx` has a dedicated
   self-assignment regression test, passing under `make tests-asan`.

Also resolved (traced, not fixed — nothing to fix): `opstack.hxx`'s own
"found but out of scope" note above questioned whether
`operator>>(PIRSET&)`'s C-style `(PIRSET)OpobjPtr` downcast could ever
see a non-`IRSET` top-of-stack. Traced `INDEX::Search()`
(`src/index.cxx`), this overload's only real caller
(`TempStack >> NewIrset;`): everything ever pushed onto `TempStack` is
either (a) an operand already gated by `GetOperandType() ==
TypeRset` — and `IRSET::GetOperandType()` is the *only* override
anywhere in the tree that returns `TypeRset` (`TERMOBJ` returns
`TypeTerm`, `OPERATOR` returns `0`), so that check alone guarantees the
object really is an `IRSET` — or (b) a freshly `new IRSET(Parent)`
result from the `TypeTerm` branch. No path pushes a raw `OPERATOR`/
`TERMOBJ` node onto `TempStack`. Confirmed safe at this call site;
left the cast as-is (no behavior to change) rather than adding a
`dynamic_cast`/runtime check for a scenario that's now confirmed
unreachable, not just unlikely.

No `NULL`/`sprintf`, zero warnings under `-Wall -Wextra`. `make
tests`/`make tests-asan` pass clean (732 test cases, 2827 assertions).

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

## src/termobj.cxx

`BUGFIX #1-2` above were already applied to this file during
`termobj.hxx`'s turn, including adding the processed marker — but this
file's own `docs/PROCESSING_STATUS.md` row was never synced to `done`.
This turn is that sync: `TERMOBJ()`/`~TERMOBJ()` are both empty bodies
(the class adds no state of its own beyond `OPERAND`), so there was
nothing left to re-review beyond confirming this remains true. No
`NULL`/`sprintf`, zero warnings under `-Wall -Wextra`. No test changes:
existing coverage in `tests/src/test_termobj.cxx` already exercises
`GetOperandType`/`GetOpType`/polymorphic assignment. `make tests`/`make
tests-asan` pass clean (740 test cases, 2842 assertions, unchanged from
before this turn).

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

## src/memcntl.cxx

`src/memcntl.hxx`'s turn already found, fixed, and tested `BUGFIX #1-3`
directly in this `.cxx` (see above), including adding the
`ISEARCH2-CLEANUP: processed 2026-08-06` marker to this file itself —
`docs/PROCESSING_STATUS.md`'s row for this file just never got synced
to `done` at the time (this file's `Order` was assigned later than
`memcntl.hxx`'s, so the row didn't exist yet when that turn ran). This
turn is that sync, plus the full dedicated re-read: no further bugs
found. `AllocSafe`'s `size` parameter could in principle go negative
only through `GetMARC()`'s `copy=1` branch in `src/marclib.cxx`
(`lrecl+1`, before that function's own `BUGFIX #4` length validation
runs) — but `copy=1` has no live caller anywhere in this tree (the only
call site, `MARC::MARC(STRING&)`, always passes `copy=0`), and even if
reached, `new (std::nothrow)` already returns `nullptr` gracefully
(checked by the caller) for a bad array length on this platform/
compiler — the same accepted contract `BUGFIX #2` above already
documents, not a new gap.

One correction to the record above: `BUGFIX #3`'s note that "the only
call site in this tree always passes `flag=0`" is now stale —
`src/marc.cxx`'s destructor (added by that file's own later turn) calls
`FreeSafe(&RememberKey, nullptr, 1)`, exercising the `flag=1` path for
real. Doesn't change anything about the fix itself (already correct
regardless of which path is live), just noted here since a stale
"unreachable" claim shouldn't stand uncorrected once it no longer holds.

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

## src/operator.cxx

`BUGFIX #1-2` above were already applied to this file during
`operator.hxx`'s turn (this file already carried `.hxx`'s own
`ISEARCH2-CLEANUP` marker, dated the same day — its
`docs/PROCESSING_STATUS.md` row just hadn't been synced to `done` yet,
same shape as `src/memcntl.cxx`/`src/mergeunit.cxx`'s stale rows). This
turn's own full re-read of all five methods (`OPERATOR()`,
`SetOperatorType()`, `Duplicate()`, `operator=()`, `~OPERATOR()`) found
nothing further. `operator=()`'s missing self-assignment guard is
confirmed harmless (unlike `src/operand.cxx`'s equivalent, which needed
checking against `ATTRLIST`'s underlying fix): `OperatorType =
OtherOp.GetOperatorType();` on self-assignment just reads and
reassigns the same plain `INT`, a no-op with no owned resource to
corrupt — matching this file's own already-cataloged "no copy
constructor either, but it's safe" finding above. No `NULL`/`sprintf`,
zero warnings under `-Wall -Wextra`. No test changes: existing coverage
in `tests/src/test_operator.cxx` already exercises every method
including `operator=` and `Duplicate()`. `make tests`/`make
tests-asan` pass clean (731 test cases, 2826 assertions, unchanged from
before this turn).

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

Also confirmed at the time (not fixed here — see the cross-reference
under `src/operand.hxx` above): self-assigning an `STERM` through its
`OPOBJ&` interface silently discarded its `Attributes` while correctly
preserving its `Term`. `STERM::operator=` calls `OPERAND::operator=`,
which copies `Attributes` via `OtherOp.GetAttributes(&Attributes)` —
on self-assignment that's `ATTRLIST::operator=` self-assigning, and
unlike `STRING::operator=` (which is why `Term` survived), it had no
self-assignment guard at the time. **Resolved during `src/sterm.cxx`'s
own turn below**: `ATTRLIST::operator=` was fixed in the meantime
(`BUGFIX #2`, `docs/BUG_CATALOG.md#srcattrlisthxx`), and `sterm.cxx`'s
turn confirmed via a standalone repro that self-assignment now
preserves both `Term` and `Attributes`.

## src/sterm.cxx

`BUGFIX #1-2` above were already applied to this file during
`sterm.hxx`'s turn (self-containment, hidden virtual `operator=`),
including adding the processed marker — but this file's own
`docs/PROCESSING_STATUS.md` row was never synced to `done`, same shape
as several other stale rows this batch. This turn's own full re-read
of all five methods (`STERM()`, `Duplicate()`, `operator=()`,
`SetTerm()`/`GetTerm()`, `~STERM()`) found nothing new — closes out the
deferred self-assignment finding above instead: confirmed via a
standalone repro that self-assigning an `STERM` (through its `OPOBJ&`
interface) now preserves both `Term` and `Attributes`, now that
`ATTRLIST::operator=` has its own guard. Removed the "deliberately no
self-assignment test" note from `tests/src/test_sterm.cxx` and added
that test, matching the same resolution already applied to
`src/operand.cxx`'s turn. No `NULL`/`sprintf`, zero warnings under
`-Wall -Wextra`. `make tests`/`make tests-asan` pass clean (740 test
cases, 2842 assertions).

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

## src/hash.cxx

`hash.cxx`'s own turn, per the note above — `BUGFIX #1`-`#4` were
already applied here during `hash.hxx`'s turn; this file's own pass
found one more.

1. **`AddEntry` leaked the `strdup()`'d value on a duplicate key, and
   on table-overflow insertion failure** — `r.Block=strdup(Value)` ran
   unconditionally, before checking whether the key already exists,
   and `Insert(r)`'s return value was discarded entirely. Two distinct
   leaks followed: every call with an already-present key skipped
   `Insert()` altogether (the `Check(r.Key)` guard), so the just-`strdup`'d
   copy was never referenced by anything and never freed — a guaranteed
   leak on every duplicate, not an edge case; and every call that *did*
   reach `Insert()` but hit table overflow (`Insert()` returns `2`
   without storing `r`) leaked the same way. Confirmed with a
   standalone repro under AddressSanitizer's LeakSanitizer
   (`tests/src/test_hash.cxx`, run against the pre-fix code with the
   fix temporarily reverted): two `AddEntry()` calls with the same key
   reported a 6-byte direct leak from `strdup()` at the exact call
   site, confirmed reachable and detected (unlike `src/fpt.cxx`'s
   `FILE*`-handle leak from an earlier turn — LeakSanitizer doesn't see
   those, but a plain `strdup()`'d buffer with no other references is
   exactly what it's built to catch). Fixed by only `strdup()`-ing
   after `Check()` confirms the key is new, and freeing `r.Block` if
   `Insert()` still fails (table overflow). See `BUGFIX #5` in source;
   regression test in `tests/src/test_hash.cxx`. `make tests`/`make
   tests-asan` pass clean (713 test cases, 2379 assertions).

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

## src/marclib.cxx

`src/marclib.hxx`'s turn already found and fixed `BUGFIX #1-3` directly
in this `.cxx` (see above); this is this file's own dedicated turn,
closing it out with one more confirmed defect found on a full re-read.

4. **`GetMARC()` trusted the caller's `lrecl` with no floor, reading
   past a too-short record in two places → confirmed
   heap-buffer-overflow** — `GetMARC()` immediately treats `record` as
   a full `MARC_LEADER_OVER` (24 bytes) and reads its `BaseAddr` field
   (offset 12-16) with no check that `lrecl` is even that large, then
   starts a directory-scan loop (`for (; isdigit(dir->tag[0]); dir++)`)
   with no check that `dir` stays within `record + lrecl` either —
   relying entirely on a well-formed record's own terminator bytes
   being present to naturally stop the scan. `MARC::MARC(STRING&)` in
   `src/marc.cxx` (Order 143, still pending) passes a document's raw
   length straight through with no minimum-size check of its own, so
   any truncated or malformed record being indexed as MARC reaches this
   directly. Confirmed real with two standalone repros under ASan: a
   10-byte record crashed inside `GetNum()` reading the leader's
   `BaseAddr`; a 24-byte (leader-only, no directory) record crashed one
   line later at the directory scan's first byte. Fixed by rejecting
   anything shorter than a full leader up front (matching `SetField()`'s
   existing "return `nullptr` on bad input" convention from `BUGFIX #2`
   above), and bounding the directory-scan loop's `dir` pointer against
   `record + lrecl` before every dereference so a truncated or
   unterminated directory can't walk past it either. See `BUGFIX #4` in
   source. Verified fixed: both repros now safely print an error and
   return `nullptr` (for the leader case) or stop the scan cleanly with
   `nfields == 0` (for the directory case) instead of reading out of
   bounds; the pre-existing "well-formed record" test in
   `tests/src/test_marclib.cxx` still passes unchanged, confirming
   nothing legitimate regressed.

Also checked and left alone: `ReadMARC()` computes `read(file, buffer+5,
lrecl-5L)`, which underflows to a huge `size_t` if a record claims a
length under 5 bytes -- looks like the same class of bug as `BUGFIX #4`
at first glance, but a standalone repro (a record whose length field
reads `00000`, backed by a large real file) did **not** reproduce a
memory-safety failure: glibc's `read()` rejects any count exceeding
`SSIZE_MAX` with `EINVAL` before touching the buffer, and any `lrecl`
under 5 underflows to a `size_t` far above that ceiling. Also, neither
`ReadMARC()` nor `SeekMARC()` has any caller anywhere in this tree
(confirmed via `grep` — only `GetMARC()`, called with `copy=0` from
`marc.cxx`, is actually used). Left unfixed per this project's "don't
claim a bug without a reproduced failure" standard — noted here rather
than silently dropped in case a future caller (or a different libc)
changes either fact.

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

## src/md5.cxx

`src/md5.hxx`'s turn already found and fixed `BUGFIX #1-2` directly in
this `.cxx` (see above, both confirmed with standalone repros — the
`uint32` width fix and the `sizeof(ctx)` scrub fix). This is this
file's own dedicated turn: a full re-read of `MD5Init`/`MD5Update`/
`MD5Final`/`MD5Transform`/`byteReverse` found no further bugs — every
buffer write is bounded by MD5's own well-known, fixed-size padding
arithmetic (all indices provably stay within `ctx->in`'s 64 bytes and
`ctx->buf`'s 4 words once `uint32` is the real 4-byte type), and no
`NULL`/`sprintf` modernization was needed. No new `BUGFIX` entry;
closes out this file the same way `src/filemap.cxx` and
`src/intlist.cxx` closed out their sibling headers' turns above.

## src/md5sum.cxx

Standalone `md5sum`-style CLI reference implementation (generate/check
MD5 digests). Genuinely dead code in this tree: not referenced by the
top-level `Makefile` at all (no build target links it), no other file
calls any of its functions, and even its own `main()` is `#if 0`'d out
inside the file itself — it has apparently never been wired into a
real binary since it was added. Read fully anyway per this file's own
turn; found two hygiene issues, no confirmable memory-safety bugs.

1. **`print_digest()` forward-declared with a different arity than its
   definition** — `void print_digest();` (zero parameters) vs. the
   actual `void print_digest(unsigned char *p)` a few dozen lines
   later. In C++ (unlike C's "unspecified parameters" convention for
   `()`), these are two genuinely different signatures, not the same
   function pre- and post-definition. Currently harmless only because
   every call site is inside `main()`'s `#if 0` block, and even if that
   were re-enabled it would still resolve correctly (the real
   definition supplies the one-argument overload actually called) — so
   this was never an active bug, just a misleading declaration. Fixed
   by correcting the forward declaration to match. See `BUGFIX #1` in
   source.

2. **`-Wsign-compare` warning in `do_check()`** — `strlen(filename) >
   flen` compares `size_t` against `int`; `flen` has to stay `int`
   since it feeds `"%-*s"`'s `int` width argument right after. Fixed
   by casting the comparison side, changing no behavior (`filename`'s
   length is always small and non-negative here). See `BUGFIX #2` in
   source.

Verified via a new `tests/src/test_md5sum.cxx` (added to
`TEST_ENGINE_SRCS` in the `Makefile`, since this file wasn't compiled
by `make tests` before now): `hex_digit()`, `mdfile()` (checked against
a real RFC 1321 vector), `get_md5_line()` (well-formed line, malformed
line, EOF), and `do_check()` (matching and mismatching checksums) all
pass under both plain and ASan+UBSan builds. No further bugs found on a
close read of `get_md5_line()`'s buffer handling — the 256-byte
`filename`/`file` output is bounded by an internal `i > 255` check
before the `strcpy`, and every `fgets()`-sourced pointer walk is capped
by `buf`'s own null terminator (caught via `hex_digit()`/explicit
character checks before any read past it), so nothing here reads or
writes out of bounds despite this code having no evidence of ever
having been exercised before.

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

## src/reclist.cxx

`BUGFIX #2` above (copy constructor and `operator=`, both already
self-assignment-safe) was already applied to this file during
`reclist.hxx`'s turn. This file's own dedicated turn (Order 164) found
nothing further: `AddEntry()`/`Expand()` use the additive
`Resize(TotalEntries+1000)` growth pattern (not the anomalous
multiplicative-doubling shape that caused a confirmed stuck-at-zero
overflow in `src/irset.cxx`'s `Expand()`), so no equivalent risk here.
`GetEntry()` leaves its output parameter untouched on an out-of-range
index — matches `IRSET::GetEntry()`'s identical, already-accepted
convention (see `tests/src/test_irset.cxx`), not a bug. `RECLIST` is
still dormant (confirmed via `grep`: its only two references, in
`src/Iindex.cxx` and `src/idb.hxx`, remain commented out). No
`NULL`/`sprintf`, zero warnings under `-Wall -Wextra`. No test changes:
existing coverage in `tests/src/test_reclist.cxx` is already thorough
(construction, add/get round-trip including out-of-range, `Expand`/
`CleanUp`, copy constructor, `operator=` including self-assignment,
independent destruction). `make tests`/`make tests-asan` pass clean
(732 test cases, 2827 assertions, unchanged from before this turn).

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

## src/vlist.cxx

`BUGFIX #1` above (copy constructor/`operator=`) was already applied to
this file during `vlist.hxx`'s turn, but the processed marker was never
added here and `docs/PROCESSING_STATUS.md`'s row was never synced —
this turn is both. Full re-read of every member function confirmed
`BUGFIX #1` present and correct, and resolved the one item that entry
explicitly deferred to this turn:

1. **The `~VLIST()` cascade-delete hazard (documented, not hardened)** —
   `vlist.hxx`'s "Found but out of scope" note flagged that `~VLIST()`
   unconditionally cascades `delete Next` through the rest of the
   circle, which is only safe when every attached node is
   heap-allocated and exactly one entry point into a circle is ever
   deliberately destroyed; it explicitly left the choice between
   hardening the destructor and just documenting the contract to this
   file's own turn. Chose documentation over hardening: there's no
   reliable way to distinguish a heap- from a stack-allocated `VLIST*`
   at runtime without extra per-node bookkeeping that no current caller
   needs, and both live subclasses (`FCT`, `STRLIST`) already honor the
   contract (their anchors are stack-/member-allocated but torn down
   through exactly one entry point; every node attached via
   `AddNode()`/`AddEntry()` is heap-allocated). Added a comment directly
   above `~VLIST()` spelling out the contract explicitly, cross-
   referencing this entry and the original one in `vlist.hxx`.
2. Modernization: pointer-typed `0` literals (`return 0;` in
   `GetNodePtr()`, `Prev`/`Next` assignments and comparisons in
   `Clear()`/`EraseAfter()`/`~VLIST()`) converted to `nullptr`, matching
   the same treatment `doctype/gopher.cxx`'s `(FILE *)0` got earlier in
   this project. No `NULL`/`sprintf` usages. `INT x = 0;` in
   `GetTotalEntries()` is a genuine integer counter, left alone.

No test changes: `tests/src/test_vlist.cxx` (written during `vlist.hxx`'s
turn) already covers `BUGFIX #1` — including the exact stack-allocated-
multi-node-circle crash that motivated item 1 above, called out in that
file's own header comment as the reason every test builds heap-allocated
nodes under a single-entry-point anchor. `make tests`/`make tests-asan`
pass clean (745 test cases, 2850 assertions).

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

4. **`LoadTable()` crashes on an empty or truncated `.dfd` file** (found
   on `dfdt.cxx`'s own later turn, after the modernization pass above
   already renamed these calls to `strtok(nullptr, ...)` but didn't
   check their results) — every one of the six `strtok()` calls in this
   function can return `nullptr`; an empty file makes even the very
   first call (reading the entry count) return `nullptr`, and
   `atoi(nullptr)` is undefined behavior — the same established pattern
   as `doctype/cipc.cxx`'s/`doctype/cipp.cxx`'s own `BUGFIX #5`, just
   reached via `DFDT`'s multi-level (entry → attribute) loop instead of
   a flat one. Confirmed with a standalone repro under
   AddressSanitizer + UndefinedBehaviorSanitizer
   (`tests/src/test_dfdt.cxx`, run against the pre-fix code with the
   fix temporarily reverted): a zero-byte file crashed with a real
   SIGSEGV inside glibc's `strtol` (`atoi`'s implementation), and UBSan
   independently flagged "null pointer passed as argument 1, which is
   declared to never be null" at the exact call site. Fixed by checking
   every `strtok()` result before use and stopping the parse at
   whichever token is missing — for a truncated attribute list this
   discards that one incomplete `DFD` rather than adding a
   half-populated entry, but keeps every entry already fully parsed
   before the truncation point. See `BUGFIX #4` in source; regression
   tests in `tests/src/test_dfdt.cxx` covering an empty file, a file
   truncated right after the entry count, and a file truncated partway
   through a later entry's attribute list. `make tests`/`make
   tests-asan` pass clean (704 test cases, 2359 assertions).
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

## src/mdt.cxx

`src/mdt.hxx`'s turn already found and fixed `BUGFIX #2-5` directly in
this `.cxx` (see above — the two unsigned-subtraction comparators,
`sprintf`→`snprintf`, the `-Wclass-memaccess` `memset`, and the
`-Wsign-compare` `Dump()` loop variable); `BUGFIX #1` was header-only
(deleted copy constructor/`operator=`). This is this file's own
dedicated turn, closing it out with one more defensive fix found on a
full re-read.

6. **Constructor's second `fopen()` reopen went unchecked** — when the
   on-disk `.mdt` file doesn't exist yet, the constructor creates it
   (`fopen(Fn, "w+b")`), closes it, and reopens it `"r+b"` for normal
   read/write use — but unlike every other `fopen()` in this same
   constructor (both siblings either fall through to a further
   fallback or `perror()`+`exit(1)`), this reopen's result was never
   checked. A failure here (the file we just created becoming
   unreadable before the reopen — another process racing to
   delete/replace it, or a permissions/umask edge case) would leave
   `MdtFp` null with `ReadOnly` still `GDT_FALSE`, so every subsequent
   `fseek`/`fread`/`fwrite`/`fileno` call — and the destructor's
   unconditional `fclose(MdtFp)` — would be a null-`FILE*` dereference.
   **Defensive fix, not a demonstrated crash**: the window (the file we
   just successfully created becoming unreadable before we reopen it,
   moments later, with no intervening code of our own) is narrow
   enough that it couldn't be forced without mocking `fopen()` or an
   actual concurrent process racing this exact file, so no standalone
   repro was attempted — unlike this project's usual "confirmed real"
   standard. Fixed anyway since the fix is free and exactly matches
   the `perror()`+`exit(1)` convention already used one branch below
   for the analogous `"rb"`-fallback failure. See `BUGFIX #6` in
   source. No new regression test (nothing to assert against without
   the mocking this repro would require) — the existing
   `tests/src/test_mdt.cxx` suite continues to pass unchanged, plain
   and ASan+UBSan.

Also checked and found sound: the constructor's `TotalEntries =
GetFileSize(Fp) / sizeof(GPREC)` would, in principle, wrap to a huge
unsigned value if `GetFileSize()` ever returned its `(off_t)-1` error
sentinel (it calls `fstat()` on an fd we just successfully `fopen()`'d
moments earlier) — but this requires `fstat()` to fail on a
freshly-opened, still-valid file descriptor, which isn't something a
standalone repro can force without mocking the syscall either. Left
unfixed, noted here rather than silently dropped, per the same
"don't claim a bug without a reproduced failure" standard. Every other
function in this file (`RemoveDeleted()`'s compaction indices,
`Resize()`'s additive growth, `AddEntry()`/`SetEntry()`/`GetEntry()`'s
bounds checks) was re-read in full and found correct.

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

## src/fpt.cxx

No new fixes on this file's own turn (`BUGFIX #1`/`#2` above, along
with the file-level doc comment, were already applied to this file
during `fpt.hxx`'s turn — this file itself was just never marked
processed or flipped to `done`). Independently re-reviewed for
anything new: one real issue found, investigated at length, and
ultimately left unfixed rather than shipping an unproven change —
recorded in full below since the investigation itself is the useful
part.

### Found but deferred, not fixed

- **`ffopen()`'s eviction can silently leak a `FILE*`/fd when every
  cached slot is still logically open** — when the table is at
  `MaximumEntries` and a new file needs a slot, the purge loop picks
  the slot with the highest priority number (least recently touched)
  purely by that number, then only physically `fclose()`s it if
  `GetClosed()` is true; otherwise the slot's old `FilePointer` is
  silently overwritten by the new entry's, and nobody ever closes the
  old one. Confirmed real with a standalone repro (a 2-entry `FPT`,
  two files opened and never `ffclose()`'d, a third file forcing
  eviction) verified via a precise check (`readlink("/proc/self/fd/N")`
  before and after, confirming the *same* underlying file stays open
  under the evicted entry's original fd — plain `fcntl(fd, F_GETFD)`
  isn't precise enough here, since a reused fd number for the
  replacement file gives a false negative). AddressSanitizer's
  LeakSanitizer does *not* catch this class of bug — glibc keeps every
  open `FILE*`'s backing allocation reachable via its own internal
  open-file list (for `atexit` flushing), so an unreachable-heap-only
  leak detector never flags a merely-unclosed file descriptor.
  **Attempted fix, reverted:** preferring an already-`GetClosed()`
  entry for eviction over one that isn't seemed like the obvious
  guard — implemented it, then tried to construct a scenario where it
  would change the outcome. Every attempt failed to find one: working
  through `HighPriority()`/`LowPriority()`'s bookkeeping shows closing
  an entry always promotes it to be the single most-evictable (highest
  priority number) entry in the table at that moment, and no later
  operation can let an untouched *open* entry surpass an already-closed
  one's number without another, more-recent close first taking its
  place — i.e. the existing purely-priority-based selection already
  behaves as if it preferred closed entries, as an emergent property of
  the ranking algorithm, in every case found. Since the attempted fix
  couldn't be shown to change any reachable outcome, keeping it would
  mean shipping unproven "defensive" code rather than a confirmed fix —
  reverted rather than kept on spec. The one case that *does* leak
  (every competing entry still logically open, none closed at all) has
  no safe automatic fix either way: forcibly closing a handle the
  caller still thinks is open just trades a leak for a potential
  use-after-close somewhere else, the same "no single obviously-correct
  meaning" reasoning already used to justify `FPT`'s non-copyable
  choice at its own turn. Left as a documented, confirmed limitation
  rather than a guessed-at fix.

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

3. **Added 2026-08-09, while processing `Isearch-cgi/isrch_srch.cxx`
   (a `pending` file, not part of this section's original turn):
   `Search()`/`AndSearch()` dereferenced `RsetPtrs[i]` immediately after
   `c_dblist[i]->MainIndex->Search()`/`AndSearch()` with no null check —
   confirmed SIGSEGV via the real `isrch_srch` CGI binary.**
   `INDEX::Search()` (`src/index.cxx`) returns `nullptr` when the
   `SQUERY` it's given tokenizes to zero real operands (its final
   `TempStack >> NewIrset;` explicitly sets `NewIrset` to `nullptr` via
   `OPSTACK::Pop()`'s empty-stack branch) — and `INDEX::AndSearch()`
   just delegates to `INDEX::Search()`, inheriting the same behavior.
   `RsetPtrs[i]->StoreDbNum(i);` ran unconditionally right after,
   crashing on the null result. This is one level deeper than
   `BUGFIX #1`'s own "no databases at all" null-return case (`RsetPtr =
   nullptr; if (c_dbcount <= 0) return RsetPtr;`), which callers were
   already expected to check for — this one hides *inside* a
   successful, `c_dbcount > 0` call. Confirmed live: a CGI request whose
   query has no real search terms (e.g. a "next page" request that's
   missing its query field entirely) reached this exact crash through
   `Isearch-cgi/isrch_srch.cxx`. Fixed in both `Search()` and
   `AndSearch()` by substituting an empty result set instead of a null
   one — `if (!RsetPtrs[i]) RsetPtrs[i] = new IRSET(c_dblist[i]);` —
   mirroring `INDEX::Search()`'s own existing fallback for its
   per-operand null case a few lines earlier in that same file
   (`if (!NewIrset) NewIrset = new IRSET(Parent);`). `BUGFIX #3` in
   source (both `Search()` and `AndSearch()`). Re-verified no new
   warnings via the same before/after `-Wall -Wextra` diff technique
   (85 warnings, unchanged); `make tests`/`make tests-asan`/`make
   smoke-test` all still pass.

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

`class ANZLIC` (`: public SGMLNORM`) is the ANZLIC-profile metadata
DOCTYPE — near-identical architecture to `doctype/anzmeta.cxx` (same
SGML-tag scanner, same `Nested`-stack approach to `<custom>` field
handling), and it shares that file's lineage closely enough that its own
`BUGFIX #1` note already cross-references this file. Reprocessed after
its original turn's tests turned out not to actually link against this
file at all (see the `Makefile` note below) — a fresh read surfaced four
more real bugs beyond the two already fixed.

1. **`strcmp()` logic error in `ParseFields()`'s closing-tag handling** —
   `if (strcmp(*tags_ptr,"/custom"))` was missing the negation: `strcmp()`
   returns non-zero when strings do *not* match, so this ran the
   Nested-popping logic for every closing tag *except* "/custom", and ran
   `InCustom=GDT_FALSE` only for "/custom" itself — both backwards. Same
   defect class as `doctype/anzmeta.cxx`'s `BUGFIX #1`. Fixed by using
   `!strcmp()`. `BUGFIX #1` in source.
2. **Missing null-pointer guard in `find_end_tag()`** — `tag` (the second
   parameter) was dereferenced without checking it for null first. In
   practice every real call site passes `*tags_ptr` for both `t[0]` and
   `tag`, and `*t`'s nullness is already checked just above, so this
   specific path was never reachable as a live bug — but a defensive
   guard either way. `BUGFIX #2` in source.
3. **`Nested.Top()` called with no `GetSize()!=0` guard, then immediately
   dereferenced** — reached whenever a "/custom" closing tag is seen.
   Not just a malformed-input edge case: opening a `<custom>` field sets
   `InCustom=GDT_TRUE` *before* the `!InCustom` check that would
   otherwise `Nested.Push()` it (custom content is deliberately excluded
   from indexing), so `Nested` is still empty by the time `</custom>` is
   reached even for a simple, well-formed `<custom>text</custom>` field —
   every real use of this field crashed, not just malformed records.
   Same bug, same fix, as `doctype/anzmeta.cxx`'s `BUGFIX #2`. Confirmed
   with a standalone repro (a record containing only `</custom>`) before
   fixing: `AddressSanitizer: SEGV ... in ANZLIC::ParseFields,
   doctype/anzlic.cxx:387` (a null-pointer dereference via
   `STRING::Equals` on a null `this`). Fixed by guarding with
   `Nested.GetSize() != 0`, the same pattern already used for the second
   `Nested.Top()` call a few lines below. `BUGFIX #3` in source.
4. **`pTag` (`new AMD_Element()`) leaked on two paths** — allocated
   unconditionally per tag, but only ever freed via `Nested.Push()` plus
   a later `Pop()`/`delete`; both the "unclassified tag" `continue` and
   the `InCustom` (inside a `<custom>` field) case skipped that push,
   leaking one `AMD_Element` per skipped or custom-nested tag. Same bug,
   same fix, as `doctype/anzmeta.cxx`'s `BUGFIX #3`: moved the allocation
   from the top of the tag-pair branch to immediately before its first
   real use. `BUGFIX #4` in source.
5. **`LoadFieldTable()` could crash on an empty (but existing) FIELDTYPE
   file** — a `do`-`while` ran `Field_and_Type = pBuf;` (via
   `STRING::operator=(const CHR*)`, which calls `strlen()` unconditionally)
   once *before* ever checking whether `pBuf` (`strtok()`'s result) was
   null. An empty file makes `strtok()` return `nullptr` on its very
   first call. Same bug as, and fixed the same way as (converting the
   `do`-`while` to a `while`), `doctype/cipc.cxx`'s `BUGFIX #5` and
   `doctype/dif.cxx`'s `BUGFIX #5` — already fixed elsewhere in this same
   batch, but the identical pattern here was missed. `BUGFIX #5` in
   source.
6. **`ParseDateRange()` used `Hold.Search(...)` with no `>0` guard before
   acting on the result** — unlike every sibling `ParseDateRange()` in
   this codebase (`dif.cxx`, `cipc.cxx`, `cipp.cxx`, `anzmeta.cxx` all
   check it). `Search()` returns `0` when a tag isn't found, so
   `Start += 9; Hold.EraseBefore(Start);` ran on a computed offset of 9
   into an unrelated buffer instead of erroring out. `STRING`'s
   `EraseBefore`/`EraseAfter` are bounds-checked so this never crashed,
   but it could silently produce a plausible-looking wrong date instead
   of a clear error value: confirmed with a 10-character buffer
   containing no `BEGDATE` tag at all, whose last two (digit) characters
   survived the bogus erase and parsed as a normal-looking number
   (`fStart` came back `90.0` instead of the intended `-1.0` error
   sentinel). Fixed by only proceeding when the opening tag was actually
   found. `BUGFIX #6` in source.

Modernization: all `NULL` uses converted to `nullptr` throughout (14
call sites in `.cxx`). Restored the missing `sgmlnorm.hxx` include in
`anzlic.hxx` for header self-containment. Removed several now-genuinely-
dead locals surfaced by bringing this file to a clean `-Wall -Wextra`
build for the first time: `pCurrentTag` and `LastEnd` in `ParseFields()`
(the latter written every iteration, but its only read lived in an inert
`if (Nested.GetSize() != 0) { pTmp = Nested.Top(); }` block that computed
a value and threw it away — removed both together), `cx` in the
`/custom`-handling branch, and `found`/`tmp[160]` in `ParseDateRange()`.

Also fixed, this turn: `doctype/anzlic.cxx` was missing from
`TEST_ENGINE_DOCTYPE_SRCS` in the top-level `Makefile`. That meant this
file's original turn could never actually have linked a test against a
real `ANZLIC` method — which is exactly why `BUGFIX #3`–`#6` went
undetected then: nothing had ever called the real `ParseFields()`,
`LoadFieldTable()`, or `ParseDateRange()` under test, plain or ASan.
Added it. `tests/doctype/test_anzlic.cxx` now covers: `ParseFields` not
crashing on a stray unmatched `/custom` closing tag (`BUGFIX #3`'s
standalone repro) and on a well-formed `<custom>text</custom>` field
(the realistic case, same root cause); a `<custom>` field followed by a
real field not crashing or hanging (`BUGFIX #1`); `LoadFieldTable` not
crashing on an empty FIELDTYPE file (`BUGFIX #5`) while still loading
real entries; `ParseDateRange` defaulting to the `-1.0` error sentinel
rather than a garbage value when `BEGDATE` is missing (`BUGFIX #6`) and
parsing a well-formed range correctly; plus the pre-existing
header-constant and `AMD_Element` accessor checks. All pass under plain
compilation and AddressSanitizer/UndefinedBehaviorSanitizer, leak
detector clean.

A pre-existing compiler warning (`find_end_tag()`'s `const PCHR` return
type — `PCHR` is already `char*`, so `const PCHR` means `char* const`,
not the presumably-intended `const char*`) was left alone: fixing the
spelling would change the declared return type, which is a public header
signature change per GENERAL step 4, not something to do without asking.


## doctype/anzmeta.hxx

`class ANZMETA` (`: public SGMLNORM`) is the ANZLIC/FGDC-derived metadata
DOCTYPE — a small SGML-tag scanner (`parse_tags()`/`find_end_tag()`,
shared with `anzlic.cxx`/`cipc.cxx`/`cipp.cxx`) feeding `ParseFields()`,
which also tracks a stack of currently-open fields (`Nested`) to build
compound names for anything nested inside a `<custom>` field. Reprocessed
after its original turn's tests turned out not to actually link against
this file at all (see the `Makefile` note below) — a fresh read surfaced
two more real bugs beyond the one already fixed.

1. **`strcmp()` logic error in `ParseFields()`'s closing-tag handling** —
   `if (strcmp(*tags_ptr,"/custom"))` was missing the negation: `strcmp()`
   returns non-zero when strings do *not* match, so this ran the
   Nested-popping logic for every closing tag *except* "/custom", and ran
   `InCustom=GDT_FALSE` only for "/custom" itself — both backwards. Same
   defect class as `doctype/anzlic.cxx`'s `BUGFIX #1`. Fixed by using
   `!strcmp()`. `BUGFIX #1` in source.
2. **`Nested.Top()` called with no `GetSize()!=0` guard, then immediately
   dereferenced** — reached whenever a "/custom" closing tag is seen.
   Not just a malformed-input edge case: opening a `<custom>` field sets
   `InCustom=GDT_TRUE` *before* the `!InCustom` check that would
   otherwise `Nested.Push()` it (custom content is deliberately excluded
   from indexing), so `Nested` is still empty by the time `</custom>` is
   reached even for a simple, well-formed `<custom>text</custom>` field —
   every real use of this field crashed, not just malformed records.
   Confirmed with a standalone repro (a record containing only
   `</custom>`) before fixing: `AddressSanitizer: SEGV ... in
   ANZMETA::ParseFields, doctype/anzmeta.cxx:902` (a null-pointer
   dereference via `STRING::Equals` on a null `this`). Fixed by guarding
   with `Nested.GetSize() != 0`, the same pattern already used for the
   second `Nested.Top()` call a few lines below. `BUGFIX #2` in source.
3. **`pTag` (`new ZMD_Element()`) leaked on two paths** — allocated
   unconditionally per tag, but only ever freed via `Nested.Push()` plus
   a later `Pop()`/`delete`; both the "unclassified tag" `continue` and
   the `InCustom` (inside a `<custom>` field) case skipped that push,
   leaking one `ZMD_Element` — plus its two `STRING` members — per
   skipped or custom-nested tag. Confirmed via a before/after
   AddressSanitizer leak-detector comparison on the `BUGFIX #2`
   regression tests (a `<custom>` field is exactly the `InCustom=true`
   leak case): `AddressSanitizer: 156 byte(s) leaked in 6 allocation(s)`
   before the fix, clean after. Fixed by moving the allocation from the
   top of the tag-pair branch to immediately before its first real use,
   inside the `!InCustom` block — a skipped tag now never allocates one
   at all. `BUGFIX #3` in source.

Modernization: all `NULL` uses converted to `nullptr` throughout (29
call sites: `strtok`, pointer comparisons, casts). Restored the missing
`sgmlnorm.hxx` include in `anzmeta.hxx` for header self-containment
(`ANZMETA : public SGMLNORM` used the base class without including its
definition). Removed two now-genuinely-dead locals surfaced by bringing
this file to a clean `-Wall -Wextra` build for the first time: `Left` in
`ParseExtent()` (declared, never used) and `LastEnd` (written every
iteration, but its only read lived in an inert
`if (Nested.GetSize() != 0) { pTmp = Nested.Top(); }` block that computed
a value and threw it away — removed both together).

Also fixed, this turn: `doctype/anzmeta.cxx` was missing from
`TEST_ENGINE_DOCTYPE_SRCS` in the top-level `Makefile`. That meant this
file's original turn could never actually have linked a test against a
real `ANZMETA` method — which is exactly why `BUGFIX #2` and `#3` went
undetected then: nothing had ever called the real `ParseFields()` under
test, plain or ASan. Added it. `tests/doctype/test_anzmeta.cxx` now
covers: `ParseFields` not crashing on a stray unmatched `/custom` closing
tag (`BUGFIX #2`'s standalone repro) and on a well-formed
`<custom>text</custom>` field (the realistic case, same root cause); a
`<custom>` field followed by a real field not crashing or hanging
(`BUGFIX #1`); plus the pre-existing header-constant and `ZMD_Element`
accessor checks. All pass under plain compilation and
AddressSanitizer/UndefinedBehaviorSanitizer, leak detector clean.

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

## doctype/cipp.cxx

`class CIPP` (`: public SGMLNORM`) is the NASA/CIP Product metadata
DOCTYPE — `doctype/cipc.cxx`'s sibling (product-level instead of
collection-level metadata), and essentially a byte-for-byte duplicate
of it apart from class/variable naming (`CIPC`→`CIPP`,
`CIPC_Element`→`CIP_Element`). Every bug found in `cipc.cxx` this
batch turned out to be duplicated here too, with the same fixes; see
`docs/BUG_CATALOG.md#doctypecipccxx` for the full reasoning behind
each one, cross-referenced below rather than repeated in full.

1. **`ParseDate()`'s `<StartDate>`/`<EndDate>` interval parsing was
   unreachable dead code**, plus the same missing-`return` on a
   missing `</StartDate>` — identical shape to `cipc.cxx`'s
   `BUGFIX #1`, fixed the same way (uppercase the search needles;
   `strlen()` on the mixed-case literal is unaffected). One difference
   from `cipc.cxx`: this file's `ParseDate()` has its `<CALDATE>` block
   entirely commented out, so `Hold.UpperCase()` served *no purpose at
   all* before this fix — it existed only to accidentally break the
   very next search. `BUGFIX #1` in source.
2. **Same two bugs, duplicated in `ParseDateRange()`** — same as
   `cipc.cxx`'s `BUGFIX #2` (this function's own `<CALDATE>` block is
   active, unlike `ParseDate()`'s, and was already using the correct
   uppercase needle — only the `<StartDate>`/`<EndDate>` searches were
   broken). `BUGFIX #2` in source.
3. **Unguarded `Nested.Top()` — the same null-pointer-dereference
   shape as `cipc.cxx`'s `BUGFIX #3`**, in the identical spot in
   `ParseFields()`'s closing-tag handling. Not re-confirmed with a
   separate standalone SEGV repro here — `cipc.cxx`'s repro (see that
   entry) exercises the identical code shape byte-for-byte; this file's
   regression test (the same `</foo>`-as-first-tag input) passes clean
   under `make tests-asan` after the fix. `BUGFIX #3` in source.
4. **Leaked `CIP_Element` on overlapping (non-LIFO) tags** — same as
   `cipc.cxx`'s `BUGFIX #4`, same `<A><B></A></B>` repro, same fix
   (drain `Nested` before `ParseFields()` returns). Confirmed
   leak-free under `make tests-asan`. `BUGFIX #4` in source.
5. **`LoadFieldTable()` could crash on an empty FIELDTYPE file** — same
   as `cipc.cxx`'s `BUGFIX #5`, same fix (`do`-`while` → `while`,
   checking `pBuf` before the first iteration too). `BUGFIX #5` in
   source.

Also fixed while bringing this file to a clean `-Wall -Wextra` build
for the first time: the identical `DOUBLE Left;` (in `ParseGPoly()`)
and `INT n;` (in `Present()`) unused-variable warnings, the identical
dead `Nested.Top()`/`LastEnd`-comparison block in `ParseFields()`
(removed, along with the now-unused `LastEnd` variable itself), and
the identical unused `RecordSyntax` parameter in `Present()`. `NULL`
converted to `nullptr` at every live call site. Also documented, not
changed: `store_attributes()` is declared as a `CIPP` member in the
header but never defined in this file, exactly like `cipc.cxx`'s
`store_attributes()`.

`tests/doctype/test_cipp.cxx` mirrors `test_cipc.cxx`'s coverage:
`ParseDate`/`ParseDateRange` successfully parsing a well-formed
interval (`ParseDate()` here uses `ParseIsoDate()` rather than
`GetFloat()`, so the regression test asserts the parse didn't error
out rather than an exact value) and correctly erroring on a missing
closing tag; `ParseFields` not crashing on a stray unmatched closing
tag and not leaking on overlapping tags; and `LoadFieldTable` not
crashing on an empty FIELDTYPE file while still loading real entries
correctly.

## doctype/dif.cxx

`class DIF` (`: public COLONDOC`) is the GCMD/DIF (Directory
Interchange Format) DOCTYPE — a genuinely different architecture from
this batch's other files: a hand-written character-at-a-time scanner
(`sgetc()`/`sungetc()`/`tell()`) feeding a small recursive-descent
parser (`start()`/`atom()`/`field()`/`group()`/...). Found and fixed a
confirmed heap-buffer-overflow (the most severe bug in this batch,
verified with a real before/after ASan repro) plus five smaller bugs.

1. **`sgetc()` had no bounds check at all, and `group()` could call it
   again after the scanner had already run off the end of the
   buffer** — `sgetc()` unconditionally did `RecBuffer[pos++]`, relying
   entirely on every caller stopping as soon as it saw the `'\0'`
   terminator. Most scanner methods correctly pair one `sgetc()`-until-
   terminator loop with exactly one trailing `sungetc()`, but `group()`
   unconditionally calls `nextToken()` (which calls `sgetc()`) once
   more immediately after `groupbody()` returns — including when
   `groupbody()` (via nested `atomtail()`/`textML()` calls) already ran
   the scanner all the way to EOF looking for an `"End_Group"` that was
   never there (a `"Group:"` left unclosed before the file ends).
   `nextToken()`'s own EOF-detection path also doesn't call `sungetc()`
   to back off afterward, so `pos` was already one past the terminator
   by the time this extra call happened, and it read further still —
   with nothing to stop it, since `sgetc()` had no bound of its own.
   Confirmed with a real before/after ASan comparison: reverting the
   fix and running just this file's unclosed-group regression test
   reproduced a clean report —
   ```
   AddressSanitizer: heap-buffer-overflow doctype/dif.cxx:856 in DIF::sgetc()
   ```
   restoring the fix cleared it. Fixed by adding a new member,
   `RecBufferLen` (set once in `ParseFields()` right after `RecBuffer`
   is allocated and NUL-terminated), and clamping `pos` to it at the
   top of `sgetc()` before every read: a single `sgetc()`/`sungetc()`
   pair at EOF still behaves exactly as before (advance to one past the
   terminator, then back to it), but any further unmatched `sgetc()`
   call re-clamps instead of advancing past that point, so it always
   safely re-reads the terminator no matter how many extra calls
   happen. `BUGFIX #1` in source.
2. **Constructor left `RecBuffer`/`pos`/`state`/`status`/`toktype`/the
   new `RecBufferLen` all uninitialized** — every one is set at the top
   of `ParseFields()` before use and nothing else in the class touches
   them first (`~DIF()` is empty), so this was never reachable as a
   live bug, but it's the same class of fix as every other
   "constructor leaves members uninitialized" turn this session.
   `BUGFIX #2` in source.
3. **`ParseDateSingle()` printed an uninitialized variable** —
   `cout << "Parse Single Date:" << fVal << endl;` ran unconditionally,
   before `fVal` was ever assigned on any path below it (confirmed a
   real `-Wuninitialized` by the compiler, not just a style nit). A
   properly `#ifdef DEBUG`-guarded print of the same intent sits right
   below it. Removed. `BUGFIX #3` in source.
4. **`start()`'s `fieldType`/`groupType` cases had a spurious
   `-Wimplicit-fallthrough`** — each ended with two complementary `if`s
   (`toktype != eofType` → do work and `break`; `toktype == eofType` →
   `break`) that always `break` either way, so no path actually falls
   through to the next `case` — but the compiler can't prove two
   conditions are complementary, and the code visually reads as if
   falling into `groupType`/`eofType` were intentional. Collapsed each
   to a single `if`-then-unconditional-`break`, identical behavior,
   removing the ambiguity for both the compiler and future readers.
   `BUGFIX #4` in source.
5. **`LoadFieldTable()` could crash on an empty FIELDTYPE file** — same
   shape as, and fixed the same way as, `doctype/cipc.cxx`'s
   `BUGFIX #5` (a `do`-`while` that ran `Field_and_Type = pBuf;`
   unconditionally before ever checking `pBuf`, crashing via
   `STRING::operator=(const CHR*)`'s unconditional `strlen()` when
   `strtok()` returns `nullptr` on its very first call). `BUGFIX #5` in
   source.
6. **`TempFile` (a `new CHR[256]` in the `USE_DIFMORPH`-gated branch of
   `Present()`) was never freed** — a leak on every record presented
   through that path. Not compiled by default in this build (`
   USE_DIFMORPH` is never defined anywhere in the tree), but fixed
   anyway since it was free to fix while already touching these exact
   lines for `sprintf`→`snprintf` modernization. `BUGFIX #6` in source.

Also documented, not changed: `ParseDateRange()`'s
`Hold.EraseAfter(End-1)` after `Hold.Search("\n")` has the same
"unguarded `End-1` underflow" shape seen in `cipc.cxx`/`cipp.cxx`, but
it's likewise harmless in practice — `STRING::EraseAfter()` already
bounds-checks and no-ops when handed an out-of-range index (confirmed
when `cipc.cxx` was processed).

Modernization: `NULL` converted to `nullptr` at every live call site.
`sprintf` converted to `snprintf` throughout, including inside the
`AGGREGATIO`/`USE_DIFMORPH`-gated branches that aren't compiled by
default in this build (real, compilable C++ that could be enabled by
a different build configuration, unlike genuinely commented-out code).
Removed several always-unused local variables surfaced by bringing
this file to a clean `-Wall -Wextra` build for the first time:
`headline` (declared five times across `Present()`'s branches, never
read in any of them), `Status2`/`pDictFile`/`pFormattedData`/
`tmpbuff`. Added class-level and per-function doc comments.

`tests/doctype/test_dif.cxx` covers: `ParseFields` extracting a simple
field and not crashing on an empty record; not overflowing on an
unclosed `Group:` (`BUGFIX #1`'s regression, verified via the
before/after ASan comparison above) and correctly parsing a
well-formed one; `LoadFieldTable` not crashing on an empty FIELDTYPE
file (`BUGFIX #5`) while still loading real entries; and
`ParseDate`/`ParseDateRange` covering their documented value shapes
(bare number, "present", "unknown", a malformed value, and a real
`START_DATE`/`STOP_DATE` range with the STOP_DATE-absent fallback).

## doctype/emacsinfo.cxx

`class EMACSINFO` splits a file into `"File:"`-prefixed records and
indexes each record's `"File:"`/`"Node:"` line values (up to the next
comma) as fields — the same author (Erik Scott) and era as
`doctype/bibtex.cxx`, and it turned out to share two of that file's
exact bug shapes.

1. **`ParseRecords()` leaked the entire file buffer on every
   successful call** — same shape as, and fixed the same way as,
   `doctype/bibtex.cxx`'s `BUGFIX #1` (`RecBuffer` freed on every
   error path but not the function's normal return). `BUGFIX #1` in
   source.
2. **`ParseFields()` leaked a `STRING::NewCString()` buffer on every
   call** — same shape as, and fixed the same way as,
   `doctype/bibtex.cxx`'s `BUGFIX #2` (the `file` variable existed only
   for `perror()` calls that can use `STRING`'s existing non-allocating
   `operator const char*()` instead, same as `fopen()` two lines
   above). `BUGFIX #2` in source.
3. **Stored `"File:"`/`"Node:"` field values included a trailing
   comma** — the scan loop (`for (val_end = val_start;
   (RecBuffer[val_end]!=',') && (val_end < ActualLength);
   val_end++)`) stops with `val_end` pointing *at* the comma delimiter
   (or at `ActualLength` if no comma is found), but
   `fc.SetFieldEnd(val_end)` takes an inclusive end index, so the
   delimiter (or, with no comma, one byte past the buffer's last real
   character) was included in the stored field. Confirmed via a
   regression test asserting the exact extracted substring for both
   fields. Fixed by using `val_end - 1` for both the `"File:"` and
   `"Node:"` fields. `BUGFIX #3` in source.
4. **Wrong class/function names in diagnostic messages** — every
   `cout`/`perror` message inside `ParseFields()` said
   `"EMACSINFO::ParseRecords()"` (eight occurrences, evidently
   copy-pasted from the real `ParseRecords()` above it and never
   updated), and `Present()`'s "can't find first newline" message said
   `"FTP::Present()"` (copy-pasted from `doctype/ftp.cxx`). Not a
   behavioral bug, but actively misleading for anyone debugging from
   these log lines — corrected all nine. Also corrected a stale comment
   above `Present()` that claimed element set `"F"` excludes the
   record's first line; the code's `"F"` branch is actually an
   intentional no-op (shows the whole record, headline included) —
   only `"B"` (brief) trims down to just the first line. `BUGFIX #4` in
   source.

Modernization: `(char *)0`/`(char*)0` null-pointer comparisons (the
functional equivalent of `NULL`, just not spelled with the macro)
converted to `nullptr` at all three sites. No live `sprintf` to
modernize. Removed an always-unused `GPTYPE i` in `ParseRecords()` and
fixed two `val_end`/`ActualLength` signed/unsigned comparison warnings
(cast the unsigned side, matching `val_end`'s existing `INT` type)
while bringing this file to a clean `-Wall -Wextra` build for the
first time. Added class-level and per-function doc comments.

`tests/doctype/test_emacsinfo.cxx` covers: `ParseRecords()` splitting
a two-record file with correct boundaries (`BUGFIX #1`'s regression,
verified leak-free under `make tests-asan`); `ParseFields()`
extracting exact `"File:"`/`"Node:"` values with no trailing comma
(`BUGFIX #3`) and adding no fields when neither marker is present.
`Present()` isn't exercised directly — it calls
`ResultRecord.GetRecordData()` unconditionally before even looking at
`ElementSet`, and that call crashes on a default-constructed `RESULT`
for reasons unrelated to any `EMACSINFO`-specific logic, the same
issue noted for `doctype/bibtex.cxx`'s `Present()`.

## doctype/eos_guide.cxx

`class EOS_GUIDE` is an HTML DOCTYPE that only ever looks inside
`<HEAD>...</HEAD>`, indexing the `<TITLE>` text and every `<META
NAME="..." CONTENT="...">` as its own field. A different architecture
from this batch's other files — a character-at-a-time `fgetc()`
tokenizer rather than an in-memory buffer scan — and it turned up two
functional bugs severe enough that the class's core feature (title
extraction) never worked at all, plus one that silently corrupted an
XML output field.

1. **`(char)fgetc(fp)` collided the real byte `0xFF` with `EOF`** —
   `fgetc()` returns either a byte value (0-255) or the sentinel `EOF`
   (typically `-1`) as an `int`; the original code narrowed that
   result to `char` before ever comparing it to `EOF`. On a platform
   where `char` is signed (the common case), the real byte `0xFF`
   narrows to `-1` too — the same value as `EOF` — so parsing stopped
   dead the instant that byte appeared anywhere in the file, including
   in ordinary (non-tag) text content like a title, not just malformed
   input. This is plausible, not just theoretical, for a class whose
   own `Present()` elsewhere explicitly claims Latin-1/ISO-8859-1
   support. Fixed by keeping the read result as an `int` throughout,
   only narrowing to `char` when storing an already-confirmed-non-EOF
   byte into the token buffer. Also fixed in the same spot: `toupper()`
   given that same value is undefined behavior for anything not
   representable as `unsigned char` (same class of bug as
   `src/nlatlon.cxx`'s `BUGFIX #3` from earlier this session) — cast
   explicitly. `BUGFIX #1` in source.
2. **Title extraction could never succeed for any input, and once a
   `</TITLE>` was seen, `<META>` tags and everything else after it in
   the `<HEAD>` were silently discarded too** — `titlePosition` (which
   `<TITLE>`'s handling sets to the offset just past the opening tag,
   for `</TITLE>`'s handling to later read back) was unconditionally
   reset to `0` at the top of *every* loop iteration, including the
   very iteration that processes `</TITLE>` itself — since a tag and
   its matching close are necessarily read in different iterations
   (the title text between them is skipped character-by-character, not
   accumulated anywhere), this reset destroyed the value before
   `</TITLE>`'s handling ever got to read it. The guard immediately
   below, `if (titlePosition == 0) break;`, was therefore *always*
   true — and since that `break` sits directly in the main parsing
   loop (not inside a `switch`), it didn't just skip adding the title
   field, it exited the *entire* loop, silently ending the parse the
   moment any `</TITLE>` was seen. Confirmed with a real before/after
   test run: reverting just this fix made 3 of this file's 4
   regression tests fail, including the unrelated META-extraction one
   — direct evidence of the cascading effect, not just reasoned about.
   Fixed by initializing `titlePosition` once, outside the loop,
   instead of resetting it every iteration. `BUGFIX #2` in source.
3. **The `"R"` (XML result) element set corrupted its `docid`
   attribute** — `StringBufferPtr->Cat(ndb);` passed the raw `INT ndb`
   to `STRING::Cat()`, which has no `INT` overload; it silently
   resolved to `Cat(const UCHR Character)` via an implicit narrowing
   conversion, appending one garbled single byte instead of the
   intended decimal database-number string. The sibling `"B"`/
   `ISEARCH_XML` branch a little further down does this correctly,
   via the `ndb_string` buffer already built by `sprintf()` a few
   lines above the bug — that's what the fix uses too. `BUGFIX #3` in
   source.

Modernization: both `sprintf(ndb_string, ...)` calls converted to
`snprintf`. No live `NULL` usage. Added class-level and per-function
doc comments.

`tests/doctype/test_eos_guide.cxx` covers: extracting the `TITLE`
field (`BUGFIX #2`'s direct regression, confirmed via the before/after
test run described above) and `META` `NAME`/`CONTENT` fields correctly
by exact substring; not stopping early on a `0xFF` byte inside the
title text (`BUGFIX #1`); and adding no `TITLE` field when the
document has none.

## doctype/fgdc.cxx

`class FGDC` (`: public SGMLNORM`) is the USGS/FGDC metadata DOCTYPE —
the third sibling in the family with `doctype/cipc.cxx` and
`doctype/cipp.cxx` (predicted worth a close look when `cipc.cxx` was
processed; that prediction held), sharing the same `ParseFields()`/
`LoadFieldTable()`/`ParseDate()`/`ParseDateRange()`/`parse_tags()`/
`find_end_tag()` structure and most of the same bugs, with
`CIPC_Element`/`CIP_Element` replaced by `MD_Element`. Much larger than
its two siblings (~2100 lines vs. ~1500), but the SGML-tag-scanning
core is structurally identical; see `docs/BUG_CATALOG.md#doctypecipccxx`
for the full reasoning behind each shared bug, cross-referenced below
rather than repeated in full. One structural difference from both
siblings: this file holds the live, canonical `GetNumericValue()` that
`cipc.cxx`/`cipp.cxx` both call via their own commented-out copies +
`extern` declarations — which is why `fgdc.cxx` was already linked into
`TEST_ENGINE_DOCTYPE_SRCS` well before this, its own turn, came up.

1. **`ParseDate()`'s `<BEGDATE>`/`<ENDDATE>` interval-parsing branch had
   the same missing-`return` bug as `cipc.cxx`'s `BUGFIX #1`** — unlike
   `cipc.cxx`, this file's needles (`<BEGDATE>`, `<ENDDATE>`) are
   already spelled uppercase in the source, so there's no accompanying
   case-mismatch half here; interval parsing itself already worked.
   But the `<BEGDATE>`-found/`</BEGDATE>`-missing branch still fell
   through into `Hold.EraseAfter(End-1)` with `End==0` (the same
   harmless-only-because-`EraseAfter()`-bounds-checks underflow) and
   kept going, searching for `<ENDDATE>` despite already having flagged
   the record malformed, leaving `*fEnd` unset on that path. Fixed the
   same way as `cipc.cxx`'s `BUGFIX #1`: added `*fEnd = *fStart;
   return;` immediately after setting `*fStart = DATE_ERROR;`.
   `BUGFIX #1` in source.
2. **Same bug, duplicated in `ParseDateRange()`** — identical shape to
   `cipc.cxx`'s `BUGFIX #2`; this is `ParseDateRange`, a near-duplicate
   of `ParseDate()`. `BUGFIX #2` in source.
3. **Unguarded `Nested.Top()` — the same null-pointer-dereference shape
   as `cipc.cxx`'s `BUGFIX #3`**, in the identical spot in
   `ParseFields()`'s closing-tag handling (`pTmp = (PMD_Element)
   Nested.Top(); if (Tag == pTmp->get_tag())` with no
   `Nested.GetSize() != 0` guard, unlike the sibling call site just
   below it in the same function). Not re-confirmed with a separate
   standalone SEGV repro here — `cipc.cxx`'s repro (see that entry)
   exercises the identical code shape byte-for-byte; this file's
   regression test (the same `</foo>`-as-first-tag input) passes clean
   under `make tests-asan` after the fix. `BUGFIX #3` in source.
4. **Leaked `MD_Element` on overlapping (non-LIFO) tags** — same as
   `cipc.cxx`'s `BUGFIX #4`, same `<A><B></A></B>` repro, same fix
   (drain `Nested` before `ParseFields()` returns). Confirmed
   leak-free under `make tests-asan`. `BUGFIX #4` in source.
5. **`LoadFieldTable()` could crash on an empty FIELDTYPE file** — same
   as `cipc.cxx`'s `BUGFIX #5`, same fix (`do`-`while` → `while`,
   checking `pBuf` before the first iteration too). `BUGFIX #5` in
   source.

Also fixed while bringing this file to a clean `-Wall -Wextra` build
for the first time: the identical `DOUBLE Left;` unused-variable
warning (in `ParseGPoly()`), and the identical dead
`Nested.Top()`/`LastEnd`-comparison block in `ParseFields()` (removed,
along with the now-unused `LastEnd` variable itself, which that dead
block was the only reader of). `NULL` converted to `nullptr` at every
live call site (comment-only mentions left alone). Confirmed via a
standalone compile before any fixes were applied that these were the
*only* two warnings in the entire ~2100-line file — the ~750-line
`Present()` and the `parse_tags()`/`find_end_tag()` pair (diffed
against `cipc.cxx`'s and confirmed structurally identical apart from
`NULL`/`nullptr`) were already warning-clean.

`tests/doctype/test_fgdc.cxx` mirrors `test_cipc.cxx`'s coverage,
adjusted for the `BEGDATE`/`ENDDATE` tag spelling: `ParseDate`/
`ParseDateRange` successfully parsing a well-formed interval and
correctly erroring on a missing closing tag (`BUGFIX #1`/`#2`);
`ParseFields` not crashing on a stray unmatched closing tag (`BUGFIX
#3`) and not leaking on overlapping tags (`BUGFIX #4`); and
`LoadFieldTable` not crashing on an empty FIELDTYPE file (`BUGFIX #5`)
while still loading real entries correctly.

## doctype/fgdcsite.cxx

`class FGDCSITE` (`: public SGMLTAG`) is a locator/directory record for
an FGDC clearinghouse node — site metadata (hostname, port, database
name, contact info, bounding coordinates), not the geospatial metadata
itself (that's `doctype/fgdc.cxx`). A much smaller, differently-shaped
file than its `doctype/fgdc*.cxx` neighbors (record splitting and field
parsing are entirely inherited from `SGMLTAG`; `FGDCSITE` only
customizes `LoadFieldTable()`, `UsefulSearchField()`, and `Present()`).
Two real bugs found, one of them a confirmed-dead feature: SGML output
had silently never worked at all.

1. **`Present()`'s SGML-variant lookup was permanently unreachable —
   requesting a record's SGML rendering silently used the wrong
   file** — in each of `Present()`'s four filename-extension-guessing
   fallback tiers (plain, short, uppercase, short-uppercase), the
   `RecordSyntax` check meant to recognize `SgmlRecordSyntax` was
   instead an exact, verbatim duplicate of the `HtmlRecordSyntax` check
   directly above it: `if (RecordSyntax.Equals(HtmlRecordSyntax)) ...
   else if (RecordSyntax.Equals(HtmlRecordSyntax)) ...`. Since the
   second condition is identical to the first, it can only ever be
   reached when the first already matched — meaning it's dead, and any
   genuinely-SGML request fell through past both `Html`/duplicate
   branches and the `Sutrs` branch to the final `else`, which just uses
   the record's filename unmodified rather than trying an
   `FGDC_SGML_EXTENSION`-suffixed sibling file. Corroborating evidence:
   `FGDC_SGML_EXTENSION`/`SHORT_FGDC_SGML_EXTENSION`/their two
   uppercase siblings are all defined in `fgdcsite.hxx` but were,
   before this fix, referenced *nowhere* in `fgdcsite.cxx` — dead
   constants defined for a feature that could never run. Fixed by
   changing all four duplicated conditions to check `SgmlRecordSyntax`
   (declared in `src/defs.hxx`, alongside `HtmlRecordSyntax`/
   `SutrsRecordSyntax` already used correctly here) and `Cat()` the
   matching SGML extension constant instead of the HTML one. Confirmed
   via a real regression test: writes a `.sgml` sibling file next to a
   fake record path with an unrelated extension, calls `Present()` with
   `RecordSyntax="SGML"`, and asserts the *real* `.sgml` file's content
   comes back — this would have silently failed (returned the wrong,
   nonexistent-file "Requested file not found" message) before the fix.
   `BUGFIX #1` in source.
2. **`LoadFieldTable()` could crash on an empty FIELDTYPE file** — same
   bug as, and fixed the same way as, `doctype/cipc.cxx`'s `BUGFIX #5`
   (`do`-`while` → `while`, checking `pBuf` before the first iteration
   too, not just between iterations). `BUGFIX #2` in source.

Modernization: the one live `NULL` (in the `LoadFieldTable()` fix
above) converted to `nullptr`. No `sprintf` usage. Added class-level
and per-function doc comments. `doctype/fgdcsite.cxx` added to
`TEST_ENGINE_DOCTYPE_SRCS` (`doctype/sgmltag.cxx`, its base class, was
already linked in from an earlier turn).

`tests/doctype/test_fgdcsite.cxx` covers: the header's extension
`#define`s; `LoadFieldTable` not crashing on an empty FIELDTYPE file
(`BUGFIX #2`) while still loading real entries correctly; `Present()`'s
`"B"` element set falling back to a placeholder when
`IDBOBJ::GetFieldData()` finds nothing (safe against a
default-constructed `RESULT` since the default `GetFieldData()`
override never touches it, unlike `GetRecordData()` — see the other
`doctype/` tests' notes on that crash); and `Present()` actually
finding and returning a real `.sgml` sibling file's contents via
`SgmlRecordSyntax` (`BUGFIX #1`'s direct regression). Not tested:
`UsefulSearchField()` is `private` (same as its `cipc.cxx`/`cipp.cxx`/
`fgdc.cxx` equivalents, none of which are tested directly either), so
it isn't reachable from an external test without a friend/subclass
workaround, which wasn't judged worth adding for a straightforward
linear name-search function.

## doctype/filename.cxx

`class FILENAME` (`: public DOCTYPE`) indexes a file by its *filename*
rather than its content: since Iindex can only index text it can get a
file pointer to, `ParseRecords()` writes the record's own filename out
to a sibling `"<name>.fn"` file and indexes that instead, so a search
matches records whose filename contains the query term. A small,
self-contained file — one real bug (a leak on every successful
"return-the-real-file" read) plus some dead-code cleanup.

1. **`Present()` leaked the whole file buffer on every non-`"B"`
   element-set call** — `RecBuffer` (allocated to hold the original
   file's contents once the `".fn"` suffix is stripped back off) was
   assigned into `*StringBufferPtr` via `STRING::operator=(const
   CHR*)`, which copies the bytes into the `STRING`'s own internal
   buffer rather than taking ownership of `RecBuffer` — so `RecBuffer`
   itself was never freed. Same bug shape as `doctype/bibtex.cxx`'s
   `BUGFIX #1` and `doctype/emacsinfo.cxx`'s `BUGFIX #1`. Fixed by
   adding `delete [] RecBuffer;` right after the assignment. Confirmed
   leak-free under `make tests-asan`. `BUGFIX #1` in source.

Also fixed while bringing this file to a clean `-Wall -Wextra` build
for the first time: removed `ParseRecords()`'s three unused `GPTYPE
Start/Position/Pos;` locals and a `static int gdb_tester;`, none of
which were ever read or written anywhere in the function. `NULL`
converted to `nullptr` at the one live call site (a "NULL-terminate"
comment elsewhere is English prose about NUL-termination, not the
macro, and was left alone). Also documented, not changed: `Present()`'s
`hackedFN.SearchReverse(".fn")` returns `0` if `".fn"` is somehow
absent, and the following `EraseAfter(dotFN - 1)` would underflow
(`STRINGINDEX` is `size_t`) to `SIZE_MAX` — but `EraseAfter()` itself
bounds-checks its argument against `Length` and no-ops rather than
crashing, and every `RESULT` of this `DOCTYPE` was indexed via
`ParseRecords()` above, which always appends `".fn"` before calling
`Db->DocTypeAddRecord()` — so this is provably unreachable in practice,
not a live bug. `doctype/filename.cxx` added to
`TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_filename.cxx` covers: `ParseRecords()` actually
writing the `".fn"` sibling file and calling
`Db->DocTypeAddRecord()` with a filename ending in `".fn"`;
`Present()`'s `"B"` element set returning the indexed filename text
(exercised directly, unlike most other `doctype/` tests' `"B"`/`"F"`
paths — `RESULT::GetRecordData()` only crashes when `fopen()` fails on
an empty/invalid path, and pointing a real `SetPathName()`/
`SetFileName()`/`SetRecordStart()`/`SetRecordEnd()`-populated `RESULT`
at a real file avoids that path entirely); `Present()` with a
non-`"B"` element set returning the *original* file's real contents
(`BUGFIX #1`'s direct regression, confirmed leak-free under `make
tests-asan`); and `Present()` not crashing when the target file is
missing.

## doctype/filmline.cxx

`class FILMLINE` (`: public MEDLINE`) is the Filmline v1.x document
type — record splitting and field parsing are entirely inherited from
`MEDLINE`; this file only customizes `UnifiedName()` (mapping
Filmline's own two-letter field codes onto the shared Medline
field-parser hooks) and `Present()` (a brief-headline composer). One
real bug, and it's severe: a 100%-reliable, always-reachable
null-pointer-dereference crash.

1. **`Present()`'s `BRIEF_MAGIC` ("B") branch crashed whenever the
   primary title field was empty** — when `UnifiedName("TI")`'s
   (`"title_true"`) lookup came back empty, the code fell back to
   `Tag = UnifiedName("TO");` — but `"TO"` is not one of
   `UnifiedName()`'s recognized field codes at all (its table has no
   such entry; only `"TI"`/`"TE"`/`"TU"`/`"SH"` among the title-ish
   codes), so this call always returned `nullptr`. The very next line,
   `Tag = <that nullptr>;`, invokes `STRING::operator=(const CHR*)`,
   which calls `strlen()` on its argument unconditionally — a
   null-pointer-dereference crash, with no guard anywhere in between.
   Confirmed with a real repro: a plain `FILMLINE` object backed by a
   default (no-op) `TESTIDBOBJ` — whose `GetFieldData()` never finds
   anything, so the primary `"TI"` lookup is unconditionally empty —
   segfaults on every single call to `Present(result, "B", &out)`, no
   contrived edge case needed:
   ```
   FILMLINE::Present element set B does not crash when the title field is empty
   tests/doctype/test_filmline.cxx:60: FAILED:
   due to a fatal error condition:
     SIGSEGV - Segmentation violation signal
   ```
   confirmed via a before-fix standalone run of just this test case
   (the SIGSEGV terminated the whole test binary, not just this one
   `TEST_CASE`, cutting the surviving suite from ~550 down to 61 test
   cases). Fixed by capturing `UnifiedName("TO")`'s result in a local
   `const CHR*` first and only assigning it to `Tag` (and calling
   `DOCTYPE::Present()` with it) when it's non-null — the comment
   right above this code already says "Should not really happen,"
   confirming a silent no-fallback skip (leaving `Title` empty, same
   as if the whole `if` block weren't there) was always the intended
   behavior for the case where no better title can be found, not a
   guaranteed crash. `BUGFIX #1` in source.

Also documented, not changed: `UnifiedName()`'s field-code table is
commented `/* Sorted List! */` but is not actually in alphabetical
order (e.g. `"LA"` appears before `"KW"`, and `"SS"` is out of its
alphabetical position among the other `"S*"` codes) — harmless, since
the lookup is a linear scan (`strcmp` against every entry), not a
binary search, so correctness never depended on the ordering; noted
in a doc comment rather than resorting the table (a resort of ~50
hand-transcribed two-letter codes risked introducing a real
transcription error for a purely cosmetic fix). `NULL` converted to
`nullptr` at the one live call site (a `/* NO NULL please! */` comment
elsewhere refers to not adding a null-terminator sentinel to the table
array, not the macro, and was left alone). No `sprintf` usage. Added
class-level and per-function doc comments. `doctype/filmline.cxx`
added to `TEST_ENGINE_DOCTYPE_SRCS` (`doctype/medline.cxx`, its base
class, was already linked in from an earlier turn).

`tests/doctype/test_filmline.cxx` covers: `UnifiedName()` mapping known
Filmline field codes to their unified names and returning `nullptr` for
an unrecognized code; and `Present()`'s `"B"` element set not crashing
when the title field is empty (`BUGFIX #1`'s direct regression,
confirmed via the real before/after SIGSEGV repro described above).


## doctype/bibtex.hxx

No bugs found. File is clean and self-contained. Added cleanup marker and modernization.

## doctype/cipc.hxx

No bugs in header. Missing parent class include (sgmlnorm.hxx) was fixed for self-containment.

## doctype/cipc.cxx

1. **ParseDate() uppercases buffer before searching for mixed-case tags (BUGFIX #1)** — line 346
   called `Hold.UpperCase()` before searching for `<StartDate>` (with mixed case), making the branch
   unreachable. Fixed by searching in the original (non-uppercased) buffer. Verified in tests.

2. **ParseDateRange() has same case-mismatch bug (BUGFIX #2)** — similar to BUGFIX #1 but in
   ParseDateRange(). The `<StartDate>` search couldn't match after uppercasing. Fixed same way.

3. **ParseFields() Nested.Top() called without bounds check (BUGFIX #3)** — a closing tag with
   nothing on the Nested stack (dereferenced without checking Nested.GetSize()!=0) caused null
   pointer dereference. Added guard before dereferencing.

4. **ParseFields() memory leak on overlapping (non-LIFO) tags (BUGFIX #4)** — when tags close
   out of order (`<A><B></A></B>`), an element remains on Nested and is never delete'd. Documented
   as a known leak; verified under ASan.

5. **LoadFieldTable() crashes on empty FIELDTYPE file (BUGFIX #5)** — IsFile() only checks
   existence; an empty file makes strtok() return nullptr on first call, then the do-while
   unconditionally assigned `Field_and_Type = pBuf` (null pointer), and STRING's assignment
   calls strlen() on it. Added nullptr check after strtok().

### Modernization

- Replaced 40+ instances of `NULL` with `nullptr` throughout .cxx file
- Added file-level doc comment

## doctype/cipp.hxx

No bugs in header. Missing parent class include (sgmlnorm.hxx) was fixed for self-containment.

## doctype/cipp.cxx

1. **ParseDate() case-mismatch bug identical to cipc.cxx BUGFIX #1** — `<StartDate>` search
   on uppercased buffer unreachable. Fixed same way.

2. **ParseDateRange() case-mismatch bug identical to cipc.cxx BUGFIX #2** — same fix applied.

3. **ParseFields() null pointer dereference on unmatched closing tag (BUGFIX #3)** — identical
   to cipc.cxx BUGFIX #3, same fix: guard Nested.Top() with GetSize()!=0 check.

4. **ParseFields() memory leak on overlapping tags (BUGFIX #4)** — identical to cipc.cxx BUGFIX #4,
   documented but inherent to tag-matching logic.

5. **LoadFieldTable() crashes on empty FIELDTYPE file (BUGFIX #5)** — identical to cipc.cxx BUGFIX #5,
   same nullptr check added after strtok().

### Modernization

- Replaced 40+ instances of `NULL` with `nullptr` throughout .cxx file
- Added file-level doc comment

## doctype/dif.hxx

No bugs found. File is clean and includes what it needs (defs.hxx, doctype.hxx, colondoc.hxx).
Added cleanup marker only.

## doctype/dif.cxx

1. **ParseFields() heap-buffer-overflow on unclosed Group tag (BUGFIX #1)** — sgetc() had no
   bounds check; group() unconditionally calls nextToken() after groupbody() returns, even
   if groupbody() already ran the scanner off the end of the buffer looking for a missing
   "End_Group". Confirmed via heap-buffer-overflow report under ASan before fixing. Added
   bounds check to sgetc().

2. **LoadFieldTable() crashes on empty FIELDTYPE file (BUGFIX #5)** — identical to cipc/cipp
   BUGFIX #5: empty file makes strtok() return nullptr on first call, and the do-while
   unconditionally assigned it to Field_and_Type (calling strlen() on nullptr).
   Added nullptr check after strtok().

### Modernization

- Replaced 10+ instances of `NULL` with `nullptr` throughout .cxx file
- Added file-level doc comment

### Tests

Comprehensive test suite created for all 4 files (test_bibtex.cxx, test_cipc.cxx,
test_cipp.cxx, test_dif.cxx) with:
- Header constant verification
- Function/class definition validation  
- Real bug reproduction tests (BUGFIX #N test cases)
- Memory leak verification under ASan
- Edge case handling (empty files, malformed tags, buffer overflows)

All tests pass cleanly under AddressSanitizer/UndefinedBehaviorSanitizer.

## doctype/firstline.cxx

`class FIRSTLINE` (`: public DOCTYPE`) indexes a text file's first line
(up to the first `\r`/`\n`, or the whole file if it has neither) as a
single `"Headline"` field. A small, self-contained file — two bugs, one
in each of the header and the implementation.

1. **`firstline.hxx`'s include guard was missing its `#define`** — the
   file opens `#ifndef FIRSTLINE_HXX` but, unlike every other header in
   this tree (`#ifndef X_HXX` / `#define X_HXX`), never actually
   defines `FIRSTLINE_HXX`. A second `#include "firstline.hxx"` within
   the same translation unit would re-process the whole file and
   redefine `class FIRSTLINE`. Purely additive (no declared signature
   changed), so not subject to the GENERAL step 4 header freeze. Fixed
   by adding the missing `#define`. `BUGFIX #1` in source.
2. **`ParseFields()`'s Headline field end was off by one** — `FC`'s end
   is inclusive of the last byte in the field (confirmed via
   `src/index.cxx`'s own `fLen = fc.GetFieldEnd() -
   fc.GetFieldStart() + 1;`), but `val_len` is a *count* of characters
   read (valid indices `0..val_len-1`), not the inclusive end index
   itself. `fc.SetFieldEnd(val_len)` therefore always pointed one byte
   too far: for a line ending in `\r`/`\n`, the indexed field included
   that trailing delimiter; for a file with no delimiter at all, it
   pointed one byte past EOF. Fixed by changing to `SetFieldEnd(val_len
   - 1)`. Confirmed via an exact-substring regression test (not just
   "doesn't crash"). `BUGFIX #2` in source.

Modernization: no live `NULL`/`sprintf` usage found. Added class-level
and per-function doc comments. `doctype/firstline.cxx` added to
`TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_firstline.cxx` covers: extracting exactly the first
line, excluding the newline (`BUGFIX #2`'s direct regression, for a
file with a `\n`); extracting the whole file when there's no newline at
all (`BUGFIX #2`'s other half); and adding no `Headline` field for an
empty file. Field names are asserted in uppercase (`"HEADLINE"`) per
the established `DF::SetFieldName()`-uppercases-internally gotcha.

## doctype/ftp.cxx

`class FTP` (`: public DOCTYPE`) splits a record on its first `\n`:
`Present()`'s `"B"` element set returns just the first line (the
headline); `"F"` returns everything after it (the body). Record
splitting/field parsing are inherited unchanged from `DOCTYPE` — only
`Present()` is customized. One bug, plus a stray copy-paste in the
file's own header comment.

1. **`Present()`'s `"B"` element set included the trailing newline in
   the "headline"** — `STRING::Search()` is 1-based and returns the
   `'\n'` character's own position; `EraseAfter(N)` keeps `N`
   characters inclusive. So `myBuff.EraseAfter(firstNL)` kept the
   newline itself as part of the returned headline, unlike its `"F"`
   sibling branch (`EraseBefore(firstNL+1)`), which correctly skips
   past it. Confirmed via a real before/after regression test:
   ```
   REQUIRE( out == "Headline text" )
   with expansion:
     Headline text

   ==
     "Headline text"
   ```
   (the `\n` after "Headline text" on the actual side is the giveaway).
   Fixed by changing to `EraseAfter(firstNL-1)`, keeping everything up
   to but not including the newline. `BUGFIX #1` in source.

Also fixed: `ftp.cxx`'s own file-header comment described the class as
`"index files based on their filename"` — a verbatim copy-paste from
`doctype/filename.cxx`'s header comment (processed earlier this batch),
not a description of what `FTP` actually does. Corrected to match
`ftp.hxx`'s own (correct) description, `"first line is headline, rest
is real body"`. No live `NULL`/`sprintf` usage. `doctype/ftp.cxx` added
to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_ftp.cxx` covers: `"B"` returning exactly the first
line with no trailing newline (`BUGFIX #1`'s direct regression); `"F"`
returning everything after the first line; and not crashing when the
file has no newline at all.

## doctype/gils.cxx

`class GILS` (`: public SGMLNORM`) is a GILS (Government Information
Locator Service) SGML DOCTYPE. Record splitting and field parsing are
entirely inherited from `SGMLNORM`; `GILS` only customizes `Present()`:
`ElementSet` `"B"` returns just the `"title"` field, and anything else
reads a pre-rendered static output file straight off disk (its
extension chosen by `RecordSyntax`) rather than composing output from
parsed fields. One real bug (resource leaks), plus dead-code cleanup
that turned out to be harmless rather than a second bug.

1. **`Present()` leaked the open file handle on two early-return
   paths** — after `fopen()` succeeds, both `if (fseek(...) != 0)
   return;` checks (bounding the file to find its length, then
   rewinding) returned without ever calling `fclose(fp)` — only the
   success path further down did. Same leaked-resource-on-early-return
   shape as `doctype/bibtex.cxx`'s `BUGFIX #3`. Fixed by adding
   `fclose(fp);` immediately before each of those two returns. Not
   forced via a real repro (would need a non-seekable stream, e.g. a
   FIFO, to make `fseek()` actually fail on a freshly `fopen()`'d
   handle — impractical to construct for a plain "present this file"
   code path, and `LeakSanitizer` doesn't track file descriptors the
   way it tracks heap memory, so `make tests-asan` couldn't confirm it
   either way even if reproduced). `BUGFIX #1` in source.

Also investigated and found NOT to be a functional bug, despite the
shape looking identical to `doctype/fgdcsite.cxx`'s `BUGFIX #1`: the
extension-guessing `if`/`else if` chain checked `HtmlRecordSyntax` and
`SgmlRecordSyntax` twice each in a row (`if (...Html...) ... else if
(...Html...) ... else if (...Sutrs...) ... else if (...Sgml...) else
if (...Sgml...) else ...`). Unlike `fgdcsite.cxx` (where the
duplicated condition made the *only* path to a real syntax check
unreachable, silently falling through to the wrong extension),
`gils.cxx`'s first (non-duplicate) `Html`/`Sgml` branches already
correctly handle those two syntaxes — the second copies are 100% dead,
unreachable `else if`s with no observable effect, and `XmlRecordSyntax`
was never explicitly checked at all (there's no such branch, duplicate
or otherwise) yet already produces the correct `GILS_XML_EXTENSION`
via the unconditional final `else`. No test can distinguish "before"
from "after" here — every `RecordSyntax` value already produced the
correct extension either way. Removed the two dead branches as
cleanup, not cataloged as a numbered `BUGFIX`.

No live `NULL`/`sprintf` usage. Added class-level and per-function doc
comments. `doctype/gils.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_gils.cxx` covers: the header's extension
`#define`s; `Present()`'s `"B"` element set returning the title field
(and returning empty when there is none); `Present()` correctly
finding the HTML/SGML/XML variant of a record via
`HtmlRecordSyntax`/`SgmlRecordSyntax`/`XmlRecordSyntax` (the SGML and
XML cases doubling as the dead-code-removal regression, confirming the
first live `Sgml` branch and the catch-all `else`-based `Xml` handling
both still work correctly with the duplicate branches gone); and
`Present()` reporting a missing file gracefully instead of crashing.

## doctype/gilsxml.cxx

`class GILSXML` (`: public SGMLTAG`) is a GILS DOCTYPE for XML-tagged
records. Record splitting and field parsing are entirely inherited
from `SGMLTAG`; `GILSXML` only customizes `Present()`, which dispatches
by `ElementSet` (`"B"`/`"G"`/`"S"`/`"F"`) and then by `RecordSyntax`
(HTML/SGML/SUTRS) to one of nine `Present_<SYNTAX>_<SET>()` helpers —
`"G"` is the brief primitive element set, `"S"` is the full record with
its `<CENTROID>` section stripped, and `"F"` is the full record
unabridged. Two real bugs, both confirmed via real before/after tests
(one via an actual revert-and-rerun, matching the memory-safety-bug
repro discipline applied to a logic bug instead).

1. **`Present_HTML_S()` duplicated its own header into the body,
   HTML-escaped** — after building the `<!DOCTYPE HTML>...<H1>Title</H1>`
   header into `Hold` and transferring it into `ESN_F` via
   `ESN_F.Cat(Hold)`, the function reused the *same* `Hold` variable as
   the accumulator for the file's non-`<CENTROID>` lines
   (`Hold.Cat(ptr)` in a loop) — without ever resetting it. `Hold`
   still held the stale header text, so the loop appended the real
   body onto the end of it, and the subsequent `Hold.Replace("<",
   "&lt;")` / `Replace(">", "&gt;")` (meant to escape the file content
   for safe display inside `<pre>`) escaped the leftover header text
   right along with it — producing a second, garbled, HTML-escaped copy
   of the header inside the visible body. Sibling functions avoid this
   two different ways: `Present_HTML_F()` uses `Hold.ReadFile(FileName)`
   (which replaces `Hold`'s content outright, per `STRING::ReadFile()`'s
   `if (Buffer) delete [] Buffer;` reallocation) instead of `Cat()` in a
   loop; `Present_SGML_S()` uses a `Hold` that was never assigned
   anything beforehand, so accumulating into it via `Cat()` was always
   safe there. Confirmed via a real before/after test: temporarily
   reverting just the `Hold = "";` reset and rerunning the regression
   test reproduced the exact escaped duplicate,
   ```
   REQUIRE( out.Search("&lt;H1&gt;") == 0 )
   with expansion:
     351 (0x15f) == 0
   ```
   restoring the fix cleared it. Fixed by resetting `Hold = "";`
   immediately before it's reused as the body accumulator. `BUGFIX #1`
   in source.
2. **`Present_SUTRS_S()` delegated to the wrong sibling, including the
   `<CENTROID>` section its own element set is supposed to omit** — it
   called `Present_SGML_F()` (the *full* record) instead of
   `Present_SGML_S()` (the *short* record, centroid stripped) — a
   one-line copy-paste from `Present_SUTRS_F()` immediately below it,
   whose identical delegation is correct there (since `"F"` is
   *supposed* to include everything). `Present()`'s own comment
   explains the intent: `"S"` stands for "short"/"summary" and exists
   specifically "to send full records without the centroid" — so for
   SUTRS `RecordSyntax`, the `"S"` element set silently returned
   identical output to `"F"`, unlike its `HTML`/`SGML` siblings (both
   of which correctly strip the centroid for `"S"`). Fixed by changing
   the delegation target to `Present_SGML_S()`. Confirmed via a
   regression test asserting the centroid marker is absent. `BUGFIX #2`
   in source.

Also fixed while bringing this file to a clean `-Wall -Wextra` build
for the first time: an unused `Field` parameter in
`UsefulSearchField()` (unnamed in the `.cxx`, matching
`doctype/cipc.cxx`'s established convention for a required-but-unused
override parameter); an unused `GDT_BOOLEAN Status;` in the main
`Present()` dispatcher, plus four further genuinely-unused `STRING`
locals in that same function (`ESN_G`, `FieldValue`, `FieldType`,
`Hold`) that GCC didn't flag (`-Wunused-variable` doesn't fire for
class-typed locals with non-trivial constructors) but were equally
dead — each `Present_*_*()` helper already declares its own local of
the same name. `NULL` converted to `nullptr` at both live call sites
(`strtok(NULL, ...)` in `Present_HTML_S()` and `Present_SGML_S()`).
Added class-level and per-function doc comments.
`doctype/gilsxml.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_gilsxml.cxx` covers: `Present()`'s `"B"` element
set returning the title when present and falling back to the filename
when absent; `Present_HTML_S()` not duplicating the header into the
body (`BUGFIX #1`'s direct regression, confirmed via the real
before/after revert-and-rerun described above) and correctly stripping
the `<CENTROID>` section; `Present_SUTRS_S()` stripping the centroid
like its `HTML`/`SGML` siblings (`BUGFIX #2`'s direct regression); and
`Present_SUTRS_F()` still returning the full record with the centroid
included, confirming the `BUGFIX #2` fix didn't disturb the
already-correct `"F"` path.

## doctype/gopher.cxx

`class GOPHER` (`: public DOCTYPE`) presents gopher-style records:
`ElementSet` `"F"` returns the raw record data; anything else (`"B"`)
looks for a `".cap/<filename>"` sidecar next to the record and, if
found, returns the value of its `"Name="` line, otherwise falls back
to the record's own filename. One bug — a resource leak, but an
always-reachable one rather than an error-path-only one.

1. **The `.cap` sidecar's file handle was never closed** — `nameFile =
   fopen(pathName, "rb");` was only ever matched by an `fclose()` on
   nothing — there's no `fclose(nameFile)` anywhere in the
   successfully-opened branch, not even after the `while
   (linebuff.FGet(nameFile, 1024))` loop finishes. Unlike
   `doctype/gils.cxx`'s `BUGFIX #1` (which only leaked on a rare
   `fseek()`-failure error path), this leaks on the *success* path —
   every single `"B"` present of a record that actually has a `.cap`
   file. Fixed by adding `fclose(nameFile);` right after the loop.
   `BUGFIX #1` in source.

Modernization: `(FILE *)0` converted to `nullptr`. No `sprintf` usage.
Added class-level and per-function doc comments.
`doctype/gopher.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_gopher.cxx` covers: `"B"` emitting the `.cap`
file's `Name=` value when one exists (`BUGFIX #1`'s code path,
though the leak itself isn't independently observable via a `REQUIRE`
since `LeakSanitizer` doesn't track file descriptors); `"B"` falling
back to the record's filename when there is no `.cap` file; and `"F"`
returning the raw record data.

## doctype/html.cxx

`class HTML` (`: public SGMLNORM`) is a WWW HTML DOCTYPE. Tag scanning
itself (`parse_tags()`/`find_end_tag()`/`store_attributes()`) is
entirely inherited from `SGMLNORM`, already reviewed on its own turn —
this file only adds HTML-specific tag filtering (`IgnoreHTMLTag()`, or
`IsHTMLFieldTag()`'s allowlist if `STRICT_HTML` is defined) and a
hand-rolled fallback for HTML's common "minimized tag" idioms
(`<DD>`/`<DT>`/`<LI>`/`<TL>` used without a matching close). One real
bug — the same error-path resource leak shape already found in
`doctype/gils.cxx` and `doctype/gopher.cxx` this batch.

1. **`ParseFields()` leaked the open file handle on an `fseek()`
   failure** — after a successful `fopen()`, `if (-1 == fseek(fp,
   (long)RecStart, SEEK_SET)) goto error;` jumped to a shared `error:`
   label whose body is just `cout << ...; return;` — correct for the
   *other* jump to that same label (where `fp == nullptr`, so there's
   nothing to close) but wrong for this one, where `fp` is a real, open
   handle. Same leaked-resource-on-early-return shape as
   `doctype/gils.cxx`'s `BUGFIX #1` and `doctype/gopher.cxx`'s
   `BUGFIX #1`, and the same "the shared label can't be fixed once for
   both call sites" wrinkle already seen in `doctype/gils.cxx`. Fixed
   by adding `fclose(fp);` at this specific call site, right before the
   `goto error;`. Not forced via a real repro (would need a
   non-seekable stream on an already-`fopen()`'d handle, and
   `LeakSanitizer` doesn't track file descriptors regardless). `BUGFIX
   #1` in source.

Also fixed while bringing this file to a clean `-Wall -Wextra` build
(checked under both the default configuration and `-DSTRICT_HTML=1`,
since `IsHTMLFieldTag()`'s `#if STRICT_HTML`-gated body is genuinely
compilable, real code, not permanently dead like `ParseRecords()`'s
`#if 1`/`#else` — see below): an unused `mdType` parameter in
`GetMetadata()` (unnamed in the `.cxx`, matching the established
convention for a required-but-unused parameter). `NULL` converted to
`nullptr` at all 14 live call sites, including inside the
`STRICT_HTML`-gated code and the several `static const char* const
tags[] = {...}` sentinel-terminated local arrays in the DD/DT/LI/TL
fallback. Not modernized: `ParseRecords()`'s `#else` branch (disabled
by a hardcoded `#if 1`, so it's never actually compiled by any build
configuration, unlike `STRICT_HTML`'s real off-by-default flag) is
abandoned work per its own `/* DOES NOT WORK, Why? */` comment — left
alone as genuinely dead code, matching the session's established
"genuinely commented-out code stays untouched" convention. Added
class-level and per-function doc comments. `doctype/html.cxx` added to
`TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_html.cxx` covers: `ParseFields()` extracting a
simple `<TAG>value</TAG>` pair as a field; not indexing a tag on
`IgnoreHTMLTag()`'s list (`<P>`); correctly handling a minimized
`<DD>` list with no matching `</DD>` via the "look for the next
`<DT>`/`</DL>`" fallback, without crashing; and `Present()`'s `"B"`
element set not crashing (full field-lookup behavior needs a live
`IDBOBJ::GetFieldData()`, out of scope for this DOCTYPE's own test —
`Present()` itself is a thin wrapper around inherited
`SGMLNORM::Present()`).

## doctype/htmltag.cxx

`class HTMLTAG` (`: public DOCTYPE`) is an HTML DOCTYPE that only ever
looks inside `<HEAD>...</HEAD>`, indexing the `<TITLE>` text and every
`<META NAME="..." CONTENT="...">` as its own field — a character-at-a-
time `fgetc()` tokenizer, structurally similar to
`doctype/eos_guide.cxx` though not derived from it (different author,
independently written). Three bugs, the first shared verbatim with
`eos_guide.cxx`.

1. **`(char)fgetc(fp)` collided the real byte `0xFF` with `EOF`** —
   identical bug to `doctype/eos_guide.cxx`'s `BUGFIX #1`: `fgetc()`
   returns either a byte value (0-255) or the sentinel `EOF` (typically
   `-1`) as an `int`; narrowing to `char` before ever comparing to
   `EOF` collides the real byte `0xFF` with `EOF` on a signed-`char`
   platform, so parsing silently stopped the instant that byte
   appeared anywhere in the `<HEAD>` section — including in ordinary
   title text, not just malformed input. Confirmed via a real
   before/after test-revert (reverting just the `int ch;`/`ch =
   fgetc(fp);` change and rerunning the 0xFF regression test reproduced
   the exact failure — the `TITLE` field silently vanished — restoring
   the fix cleared it). Fixed the same way as `eos_guide.cxx`: keep
   `ch` as `int` throughout. `BUGFIX #1` in source.
2. **`isalnum()`/`toupper()` called on plain (possibly signed) `char`
   values from file content** — both are undefined behavior for an
   argument not representable as `unsigned char` (or `EOF`); a `META`
   tag's `NAME` value containing a byte `>= 0x80` (e.g. Latin-1/UTF-8
   content) would pass a negative value to `isalnum()` in the
   `NAME=`-value-length loop, and `TagMatch()`'s two `toupper()` calls
   plus its trailing `isalnum()` had the same exposure on `tag[]`
   (raw file content) — `tagType[]` is always one of this file's own
   ASCII string literals, so that half of the cast is defensive rather
   than fixing an observed bug. Same UB class as `src/nlatlon.cxx`'s
   earlier fix. Fixed by casting to `(unsigned char)` at all four call
   sites. Verified via `make tests-asan`
   (UndefinedBehaviorSanitizer) with a `META` tag containing a `0x80`
   byte in its `NAME` value — passes clean after the fix; would abort
   under UBSan without it. `BUGFIX #2` in source.
3. **`titlePosition` could be read uninitialized** — the
   `</TITLE>`-handling branch read `titlePosition` unconditionally,
   even if a matching `<TITLE>` had never actually been seen (e.g. a
   stray/malformed `</TITLE>` with no opener) — reading an
   uninitialized `long` local. Fixed by adding a `sawTitleOpen` guard,
   set only when `<TITLE>` is actually matched, and gating the
   `</TITLE>` branch on it. Not confirmed via a before/after crash
   repro (reading uninitialized stack memory is nondeterministic, not
   reliably reproducible), but the "after" behavior — a stray
   `</TITLE>` produces no `TITLE` field — is deterministically tested.
   `BUGFIX #3` in source.

No live `NULL`/`sprintf` usage. Added class-level and per-function doc
comments. `doctype/htmltag.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_htmltag.cxx` covers: extracting the `TITLE` field
and `META` `NAME`/`CONTENT` fields correctly by exact substring; not
stopping early on a `0xFF` byte in the title text (`BUGFIX #1`'s direct
regression, confirmed via the real before/after test-revert described
above); not triggering `UndefinedBehaviorSanitizer` on a `META` `NAME`
value containing a `0x80` byte (`BUGFIX #2`'s regression, checked under
`make tests-asan`); ignoring a stray `</TITLE>` with no matching
`<TITLE>` (`BUGFIX #3`'s regression); and adding no `TITLE` field when
there is none.

## doctype/iafadoc.cxx

`class IAFADOC` (`: public COLONDOC`) is a "colon:value"-tagged IAFA
(Internet Anonymous FTP Archive) file announcement DOCTYPE. Record
splitting and field parsing are entirely inherited from `COLONDOC`;
`IAFADOC` only customizes `Present()`, which composes a one-line
`BRIEF_MAGIC` ("B") headline by trying several fallback tag names in
turn (`Title`; else `Package-`/`Service-`/`Preferred-`/`Mailinglist-`/
`Newsgroup-Name`; else a truncated `Description`), appending an
`Author` when one is present.

**No bugs found.** A small, self-contained file — traced the full
fallback chain by hand (each `DOCTYPE::Present()` call resets its
output `STRING` first, so no stale-value carryover between attempts;
each nested `if`/`else` only reaches one label-plus-lookup branch, so
no double-labeling is possible; the `Description` truncation's
`EraseAfter(CUT_OFF-3)` + `Cat("...")` lands at exactly `CUT_OFF`
characters, confirmed not off-by-one) and found no functional defect.
Compiles clean under `-Wall -Wextra` with zero warnings; no live
`NULL`/`sprintf` usage. Added class-level and per-function doc
comments. `doctype/iafadoc.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`
(`doctype/colondoc.cxx`, its base class, was already linked in from an
earlier turn).

`tests/doctype/test_iafadoc.cxx` covers the full `Present()` fallback
chain: `Title`+`Author` combined; `Title` alone with no `Author`;
falling back to `Package-Name` when there's no `Title`; falling all
the way through to `Newsgroup-Name`; falling back to a
newline-truncated, ellipsis-suffixed `Description` when no name field
exists at all; and not crashing when nothing is found anywhere.

## doctype/iknowdoc.cxx

`class IKNOWDOC` (`: public COLONDOC`) is an "IKNOW"-flavored
colon-tagged DOCTYPE: every record must open with a `"Template:"`
field followed immediately by a `"Handle:"` field, and every tagged
value is additionally duplicated into a catch-all `"Value-only"`
field. The set of distinct template types seen across every record is
accumulated in a `TemplateTypes` member and written to a `"<db>.tpt"`
sidecar file when the object is destroyed. Four real bugs — the first
severe enough to abort an entire indexing run over a single malformed
record.

1. **A single malformed record aborted the entire process** —
   `ParseFields()` called `EXIT_ERROR` (`{fflush(stdout);
   fflush(stderr); exit(1);}`, `src/defs.hxx`) whenever a record's
   first field wasn't `"Template"` or its second wasn't `"Handle"` —
   terminating the *whole* indexing run, discarding all progress
   already made on every other file, over one bad record. No other
   doctype in this tree aborts the entire process this way; every
   sibling warns and skips just the offending record/field instead.
   Confirmed via a real before/after repro: reverting just this fix
   and running the full test suite made the process exit(1) mid-run,
   silently truncated with no final summary at all —
   ```
   $ tests/run_tests
   ...
   Record in "..." does not begin with a Template type!
   $ echo $?
   1
   ```
   (588 test cases across the *entire tree*, not just this file, never
   got to report a result). Fixed by keeping the existing diagnostic
   but cleaning up (`pdft`/`RecBuffer`/`tags`) and `return`ing —
   skipping just this record — instead of exiting. `BUGFIX #1` in
   source.
2. **A `"Template"` field's temporary buffer was never freed** — `PCHR
   pstr = new CHR[val_len+2]; ...; STRING foo = pstr;` never
   `delete []`'d `pstr` — `STRING::operator=(const CHR*)` copies into
   `foo`'s own buffer, so `pstr` leaked on every `"Template"` field.
   Same leak-on-every-call shape as `doctype/bibtex.cxx`'s `BUGFIX
   #1`/`#2`. Confirmed leak-free under `make tests-asan` after adding
   `delete [] pstr;`. `BUGFIX #2` in source.
3. **The whole-file-as-one-record fallback truncated the file's last
   byte** — `RecEnd = ftell(fp) - 1;` (unlike every sibling doctype's
   identical fallback, e.g. `doctype/cipc.cxx`/`doctype/html.cxx`,
   which use plain `ftell(fp)`) made `RecLength` one byte short of the
   real file size, so the final byte was never `fread()`'d into
   `RecBuffer` — and since the last tag's value is bounded by
   `RecLength` when there's no next tag to bound it instead, a final
   line with no trailing newline lost its last character. Confirmed
   via an exact-substring regression test against a file whose last
   line has no trailing `\n`. `BUGFIX #3` in source.
4. **An unnecessary, unsafe downcast in the destructor** —
   `((IDB*)Db)->GetDbFileStem(&temp);` reinterpret-cast `Db` (an
   `IDBOBJ*`) to `IDB*` before calling a method that's already declared
   `virtual` on `IDBOBJ` itself (`src/idbobj.hxx`) — the cast was
   entirely redundant (ordinary virtual dispatch through the original
   `IDBOBJ*` reaches the exact same override for any real, `IDB`-backed
   `Db`) and unsafe (calling a member through a pointer reinterpreted
   to a type the pointee isn't actually an instance of is undefined
   behavior for any `Db` that isn't an `IDB`). Fixed by dropping the
   cast. This also made `"rcache.hxx"`/`"dtreg.hxx"`/`"fprec.hxx"`/
   `"fpt.hxx"`/`"index.hxx"`/`"registry.hxx"`/`"idb.hxx"` entirely
   unused (they existed solely to support `IDB`'s full class
   definition for that one cast) — removed. `BUGFIX #4` in source.

`NULL` converted to `nullptr` at all 6 live call sites. No `sprintf`
usage. Added class-level and per-function doc comments.
`doctype/iknowdoc.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`
(`doctype/colondoc.cxx`, its base class, was already linked in from an
earlier turn).

`tests/doctype/test_iknowdoc.cxx` covers: extracting a well-formed
record's fields, including the `"Value-only"` duplicate; tracking
distinct `"Template"` values into `TemplateTypes`; not truncating a
record's last byte when it has no trailing newline (`BUGFIX #3`'s
direct regression); and — critically — not aborting the process on a
record with the wrong first or second field (`BUGFIX #1`'s direct
regression: every `TEST_CASE` in this file, and every other test file
sharing the same binary, only gets to run *because* this fix works).
Every test implicitly exercises `~IKNOWDOC()`'s now-safe
`Db->GetDbFileStem()` call too (`BUGFIX #4`), confirmed leak/UB-free
under `make tests-asan`.

## doctype/incoming/sgmlgils.cxx

`class SGMLGILS` (`: public SGMLNORM`) is a GILS SGML DOCTYPE that
dispatches `Present()` by `RecordSyntax` *OID string* (e.g.
`"1.2.840.10003.5.101"` for SUTRS) rather than by name, unlike every
other `RecordSyntax`-aware DOCTYPE in this tree. Uniquely among every
file processed so far, **this file has never successfully compiled in
this repository's history** — it lives in `doctype/incoming/`, a
staging directory referenced nowhere else in the entire tree (`grep
-rl incoming` finds nothing outside this one directory: no Makefile,
no other source file, nothing ever built or linked against it), and
had four hard compile errors, not just warnings.

1. **`SUTRS_OID`/`GRS1_OID` were undeclared identifiers** —
   `Present()` compared `RecordSyntax` against these two names, but
   neither is defined anywhere in this tree. Confirmed, not guessed:
   this file's own `"Unsupported record syntax"` fallback message
   literally quotes `"1.2.840.10003.5.101"` as the SUTRS OID — an
   exact match for `src/defs.cxx`'s real `SutrsRecordSyntaxOID`
   constant (`const CHR* SutrsRecordSyntaxOID = "1.2.840.10003.5.101";`).
   By the same naming pattern, `GRS1_OID` is `GRS1RecordSyntaxOID`
   (`"1.2.840.10003.5.105"`). Fixed by substituting the real constant
   names. `BUGFIX #1` in source.
2. **`GetSUTRSRecord()` was defined twice with an identical signature**
   — a hard One-Definition-Rule violation. The first definition was an
   empty stub (`*StringBuffer = ""; if(ElementSet == "B") { }`, doing
   nothing); the second, immediately following it, is a complete
   implementation (composes a `"B"` headline from Title/Control-
   Identifier/Originator/Local-Control-Number per GILS's suggested
   format, has placeholder strings for `"F"`/`"G"`, and reads a sibling
   `.htm` file for `"HTML HTML 0"`). This has every appearance of an
   ordinary editing mistake — writing a fuller replacement below an
   earlier draft and forgetting to delete the draft. Fixed by removing
   the empty first definition; the complete second one survives
   unchanged. `BUGFIX #2` in source.
3. **`GetGRS1Record()` was called but never declared or defined** —
   `Present()`'s `GRS1RecordSyntaxOID` branch called it unconditionally.
   Unlike the OID-name fix above, there was no existing "real"
   implementation to recover here — GRS1 (Generic Record Syntax 1)
   formatting was simply never written. Rather than invent formatting
   logic with no basis for correctness, added a declaration (to
   `sgmlgils.hxx`, purely additive and risk-free since this header is
   never included by any other file in the tree) and a minimal
   placeholder body matching this class's *own* established
   convention for genuinely-unimplemented element sets (compare
   `GetSUTRSRecord()`'s `"F"`/`"G"` branches, which are likewise just
   placeholder strings). `BUGFIX #3` in source.

No live `NULL`/`sprintf` usage. Also removed: `#define GILSRECORD_PATH
"/home1/kgamiel/dev/GILS/records"`, a dead macro (never referenced
anywhere in the file) hardcoding the original author's personal
development-machine path. Also fixed: the header's own file-identity
comment claimed `"File: sgmlnorm.hxx"` (copy-paste from its parent
class's header) instead of `"sgmlgils.hxx"`. Added class-level and
per-function doc comments. `doctype/incoming/sgmlgils.cxx` added to
`TEST_ENGINE_DOCTYPE_SRCS`, which in turn surfaced a real Makefile gap:
the `tests/obj/doctype-%.o`/`tests/obj-asan/doctype-%.o` pattern rules
only ever `mkdir -p`'d the flat `tests/obj`/`tests/obj-asan` roots, but
this file's path (nested one level under `doctype/incoming/`) needs
`tests/obj/doctype-incoming/` to exist first. Fixed by generalizing
both rules' directory creation from a hardcoded root to `mkdir -p
$(dir $@)` — identical behavior for every existing (non-nested) file,
now also correct for a nested one.

`tests/doctype/incoming/test_sgmlgils.cxx` (mirroring the source's own
`doctype/incoming/` nesting, per the established test-path convention)
covers: `Present()` dispatching both OIDs to the right handler
(`BUGFIX #1`'s direct regression) and reporting unsupported syntaxes;
`GetGRS1Record()`'s placeholder actually being reachable at all
(`BUGFIX #3`'s direct regression — before this turn, simply calling it
was a compile error); `GetSUTRSRecord()`'s `"B"` composing the expected
dash-joined summary (confirming `BUGFIX #2` kept the *complete*
definition, not the empty stub), its `"F"` placeholder, its `"HTML
HTML 0"` branch actually reading a real sibling `.htm` file, and its
fallback message for an unrecognized element set.

## doctype/irlist.cxx

`class IRLIST` (`: public MAILFOLDER`) is a mail-digest DOCTYPE:
`ParseRecords()` overrides the inherited `MAILFOLDER::ParseRecords()`
to also split at a `"*********"` magic separator line, in addition to
the base class's own blank-line-preceded mbox `"From "` splitting.
Both bugs found here were confirmed by direct comparison against
`doctype/mailfolder.cxx`'s own `ParseRecords()` — already processed,
already fixed — whose protections this override never picked up when
it was originally forked from (or written alongside) the base class.

1. **A "From " line anywhere in a digest message's body could
   spuriously fragment the record** — `IsMailFollowLine()` matches any
   well-formed Unix mbox envelope header (`"From <addr> <date>..."`),
   which is common *inside* quoted/forwarded mail — exactly what a
   digest is made of. `doctype/mailfolder.cxx`'s own `ParseRecords()`
   guards this exact check with a `Look` flag, true only when the
   *previous* line was blank (the real mbox convention: a blank line
   always precedes a new message's `"From "` line) — this override
   dropped that guard entirely, checking `IsMailFromLine(buf)`
   unconditionally on every line. Confirmed via a regression test: a
   `"From "`-formatted line embedded mid-message with no preceding
   blank line, followed by a genuinely blank-line-preceded `"From "`
   for the next real message, produces exactly 2 records (not 3) after
   the fix. The magic separator doesn't need this guard (9 asterisks
   is already distinctive on its own), so only the `IsMailFromLine()`
   branch is gated. `BUGFIX #1` in source.
2. **Unsigned integer underflow added a record ending ~4 billion bytes
   past the real file** — `GPTYPE` is `UINT4` (`src/defs.hxx`);
   whenever the triggering line was the very first line of the file
   (`SavePosition == 0` — e.g. a standard mbox-format file, which
   conventionally *starts* with a `"From "` line), `RecordEnd =
   SavePosition - 1` underflowed to `UINT_MAX`, and `RecordEnd > Start`
   (0) then passed, handing `Db->DocTypeAddRecord()` a record whose end
   is nowhere near the actual file. `doctype/mailfolder.cxx`'s own
   `ParseRecords()` already guards against exactly this
   (`RecordEnd = (SavePosition == 0) ? 0 : SavePosition - 1;`); this
   override never had the equivalent guard, at either of the two sites
   sharing the same `X - 1` pattern (the main loop's trigger handling,
   and the post-loop final-record logic, the latter reachable via a
   genuinely empty/zero-byte file). Confirmed via a real before/after
   test-revert:
   ```
   REQUIRE( r.second < content.size() )
   with expansion:
     4294967295 (0xffffffff) < 81
   ```
   restoring the fix cleared it. Fixed by mirroring
   `mailfolder.cxx`'s `(X == 0) ? 0 : X - 1` guard at both call sites.
   `BUGFIX #2` in source.

`NULL` converted to `nullptr` at the one live call site. No `sprintf`
usage. Added class-level and per-function doc comments.
`doctype/irlist.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`
(`doctype/mailfolder.cxx`, its base class, was already linked in from
an earlier turn).

`tests/doctype/test_irlist.cxx` covers: no bogus oversized record when
the first line of the file is a `"From "` line (`BUGFIX #2`'s direct
regression, confirmed via the real before/after test-revert described
above); an embedded, non-blank-line-preceded `"From "` line inside a
message body not fragmenting the record, verified against exact
byte-offset boundaries (`BUGFIX #1`'s direct regression); splitting
correctly at a magic separator line, also verified against exact
byte-offset boundaries; and no bogus record for a genuinely empty
file (`BUGFIX #2`'s other guarded call site).

## doctype/listdigest.cxx

`class LISTDIGEST` (`: public MAILFOLDER`) is a Listserv-style mail
digest DOCTYPE — a simpler sibling of `doctype/irlist.cxx`:
`ParseRecords()` splits only at a long `"====...===="` magic separator
line, with no `IsMailFromLine()` splitting at all (so `irlist.cxx`'s
`BUGFIX #1`, the missing blank-line guard, doesn't apply here — there's
nothing to guard). One confirmed bug, of the same underflow family as
`irlist.cxx`'s `BUGFIX #2` and the already-fixed
`doctype/mailfolder.cxx`.

1. **Unsigned integer underflow added a record ending ~4 billion bytes
   past the real file, for a genuinely empty file** — `GPTYPE` is
   `UINT4` (`src/defs.hxx`); the post-loop final-record logic,
   `RecordEnd = Position - 1;`, underflowed to `UINT_MAX` whenever
   `Position` stayed `0` (i.e. `fgets()` never succeeded even once —
   a zero-byte file), and `RecordEnd > Start` (0) then passed. Confirmed
   via a real before/after test-revert (the same `4294967295`
   wraparound signature as `irlist.cxx`'s `BUGFIX #2`). Fixed the same
   way: `RecordEnd = (Position == 0) ? 0 : Position - 1;`. `BUGFIX #1`
   in source.

**Investigated and confirmed NOT a bug**, despite the identical-looking
`RecordEnd = SavePosition - 1;` line inside the main loop (the same
shape that *was* a real, separately-numbered bug in `irlist.cxx`): in
this file, `Position += line_len;` runs unconditionally for *every*
line, before the magic-separator check, and that check requires
`line_len > magic_len` (40) to enter the block at all — so
`SavePosition` is provably at least 41 whenever `RecordEnd =
SavePosition - 1;` executes, never 0. Confirmed both by hand-trace and
a standalone runnable check (`SavePosition = 41` for a magic line as
literally the first line of the file) before concluding this site
needed no guard — deliberately left unguarded per "don't add
validation for a scenario that can't happen," with a source comment
explaining why, rather than copying `irlist.cxx`'s guard by rote.

`NULL` converted to `nullptr` at the one live call site. No `sprintf`
usage. Added class-level and per-function doc comments.
`doctype/listdigest.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`
(`doctype/mailfolder.cxx`, its base class, was already linked in from
an earlier turn).

`tests/doctype/test_listdigest.cxx` covers: splitting correctly at a
magic separator line, verified against exact byte-offset boundaries;
no bogus record when a magic separator is literally the first line of
the file (an empirical check on the "not a bug" finding above, not
just the hand-trace); and no bogus record for a genuinely empty file
(`BUGFIX #1`'s direct regression, confirmed via the real before/after
test-revert described above).

## doctype/litmed.cxx

`class LITMED` (`: public SGMLNORM`) is an SGML/HTML-tagged LITMED
(medical literature) DOCTYPE, explicitly adapted from
`doctype/html.cxx` (per this file's own "based on BSN's html.cxx"
comment, and its now-corrected copy-paste header that used to claim
`"File: html.cxx"` / `"Class HTMLTAG"`). Shares `html.cxx`'s
architecture (tag scanning via the inherited `SGMLNORM::parse_tags()`/
`find_end_tag()`, an `IsLITMEDFieldTag()` allowlist) but not its
"minimized tag" `<DD>`/`<DT>`/`<LI>`/`<TL>` fallback handling, which
was never carried over — leaving a whole helper function dead. Two
real bugs, one shared verbatim with `html.cxx`, one this file's own.

1. **`ParseFields()` leaked the open file handle on an `fseek()`
   failure** — identical shape to `doctype/html.cxx`'s `BUGFIX #1`:
   `if (-1 == fseek(fp, RecStart, SEEK_SET)) goto error;` jumped to a
   shared `error:` label whose body is just `cout << ...; return;` —
   correct for the *other* jump to that label (`fp == nullptr`, nothing
   to close) but not this one, where `fp` is a real, open handle from
   the successful `fopen()` above. Fixed the same way: `fclose(fp);`
   right before the `goto error;`. `BUGFIX #1` in source.
2. **The whole-file-as-one-record fallback truncated the file's last
   byte** — `RecEnd = ftell(fp) - 1;`, unlike `html.cxx`'s
   corresponding line (plain `RecEnd = ftell(fp);`, no `- 1`) — this is
   this file's *own* authoring error, not something carried over
   unfixed from its ancestor. Same off-by-one shape as
   `doctype/iknowdoc.cxx`'s `BUGFIX #3`. Confirmed via a real
   before/after test-revert: with the bug reverted, a file consisting
   of exactly `<title>My Title</title>` (no trailing newline) failed to
   find *any* `TITLE` field at all — losing the file's last byte (the
   closing `</title>`'s final `>`) broke `find_end_tag()`'s ability to
   recognize the tag pair at all, not merely losing one character of
   content. Fixed by dropping the `- 1`. `BUGFIX #2` in source.

Also removed while bringing this file to a clean `-Wall -Wextra` build
for the first time: a genuinely dead `find_next_tag()` helper
(surfaced by `-Wunused-function`) — `html.cxx` uses its own copy of
this same helper for the minimized-tag fallback mentioned above, but
that fallback logic itself was never adapted into this file, leaving
the helper with no caller. `NULL` converted to `nullptr` at all live
call sites. Documented, not changed: `IsLITMEDAttributeTag()`'s
`Tags[]` table holds only its own terminator sentinel — no real
entries were ever filled in (unlike `html.cxx`'s equivalent table),
so it always returns "not found"; left as-is rather than guessing at
LITMED-specific attribute tags with no basis in this file for what
they should be. Also documented: `STRICT_LITMED` is an *unconditional*
`#define STRICT_LITMED 1` (unlike `html.cxx`'s `#ifndef`-guarded
`STRICT_HTML`), confirmed via a real `-DSTRICT_LITMED=0` compile
attempt that GCC's own `"STRICT_LITMED" redefined` warning shows gets
silently overridden back to `1` — so `ParseFields()`'s `#else` branch
(`IgnoreLITMEDTag()`) is permanently unreachable in practice, the same
as `html.cxx`'s genuinely-dead `#if 1` branch, just spelled
differently. Added class-level and per-function doc comments.
`doctype/litmed.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_litmed.cxx` covers: extracting an allowlisted tag
pair (`"title"`) with no trailing byte lost (`BUGFIX #2`'s direct
regression, confirmed via the real before/after test-revert described
above); not indexing a tag absent from `IsLITMEDFieldTag()`'s
allowlist; extracting an `"xauthor"` field (a second allowlist entry,
for basic table-coverage beyond just `"title"`); and `Present()`'s
`"B"` element set not crashing (full field-lookup behavior needs a
live `IDBOBJ::GetFieldData()`, out of scope here, matching
`test_html.cxx`'s equivalent note). Field names are asserted in
uppercase (`"TITLE"`/`"XAUTHOR"`) per the established
`DF::SetFieldName()`-uppercases-internally gotcha.

## doctype/maildigest.cxx

`MAILDIGEST : public MAILFOLDER` is a byte-for-byte structural
duplicate of its sibling `doctype/listdigest.cxx`, differing only in
the magic separator string (`"------------------------------"`, 30
dashes, vs. `listdigest.cxx`'s 40 equals signs) and the class/file
names. `ParseFields()` is inherited unchanged from `MAILFOLDER`.

1. **`ParseRecords()`'s post-loop `RecordEnd` underflow for an empty
   file (`BUGFIX #1`)** — `GPTYPE` is `UINT4` (`src/defs.hxx`,
   unsigned); for a genuinely empty file (`fgets()` never succeeds even
   once), `Position` stays `0`, and the post-loop `RecordEnd = Position
   - 1;` underflowed to `UINT_MAX` (`4294967295`), which then passed
   the `RecordEnd > Start` (0) guard and added a bogus record ending
   ~4 billion bytes past the real (zero-byte) file. Exact same
   underflow shape as `doctype/listdigest.cxx`'s own `BUGFIX #1`,
   `doctype/irlist.cxx`'s `BUGFIX #2`, and the already-fixed
   `doctype/mailfolder.cxx::ParseRecords()`. Fixed identically:
   `RecordEnd = (Position == 0) ? 0 : Position - 1;`. Confirmed via a
   real before/after test-revert (reverting to plain `Position - 1`
   reproduced a `REQUIRE` failure with a bogus out-of-range record,
   exactly as predicted, before the fix was restored). `BUGFIX #1` in
   source.

   The mid-loop `RecordEnd = SavePosition - 1;` site does **not** need
   the same guard — by the same reachability argument already proven
   for `listdigest.cxx` (see its `BUGFIX #1` entry above): `Position`
   is incremented by the *current* line's length, which must exceed
   `magic_len` (30, for the 30-dash separator here) to enter that
   `if`-block at all, so `SavePosition` is always `>= 31` whenever that
   assignment runs. Not independently re-derived from scratch via a
   fresh standalone trace program (as was done for `listdigest.cxx`),
   since the logic shape and reasoning are identical byte-for-byte;
   cross-referenced instead. Left unguarded, matching "don't add
   validation for scenarios that can't happen."

`NULL` converted to `nullptr` at the one live call site (`fgets(...)
!= nullptr` loop condition). Added class-level doc comment to the
header and a `ParseRecords()` doc comment in the source, both
cross-referencing `listdigest.cxx` given the structural identity.
`doctype/maildigest.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_maildigest.cxx` covers: correct splitting at a
`"----...----"` magic separator line with exact byte offsets; no
bogus record when the magic separator is the literal first line (an
empirical re-confirmation of the mid-loop-site reachability argument
above, mirroring `listdigest.cxx`'s equivalent test); and no bogus
record for an empty file (`BUGFIX #1`'s direct regression test).

## doctype/marcdump.cxx

`MARCDUMP : public DOCTYPE` parses MARC records dumped by the Yaz
`marcdump` utility: `ParseRecords()` splits a file into one record per
run of lines starting at a line whose 3-character tag is `"001"`, and
`ParseFields()` splits each record's bytes into tag/value pairs. Four
bugs found, all fixed and confirmed via before/after test-reverts.

1. **`ParseRecords()` never flushed the last record (`BUGFIX #1`,
   most severe)** — the loop only calls `Db->DocTypeAddRecord()` when
   it finds the *next* record's `"001"` tag (per the file's own
   design: "indicate the previous range as a record" upon seeing the
   next one's marker), so the very last record in any file — for a
   single-record file, the *only* record — never had a following
   `"001"` to trigger that, and was silently never indexed at all.
   Traced by hand through the loop's byte-offset bookkeeping for both
   a one-record and a two-record file, then confirmed via a real
   before/after test-revert: with the post-loop flush removed, a
   single-record file produced zero calls to `DocTypeAddRecord()`, and
   a two-record file produced only one. Fixed by flushing the final
   accumulated `RS`/`marcLength` after the loop, the same way the loop
   itself flushes every earlier record. `BUGFIX #1` in source.

2. **`ParseFields()`'s "whole file as one record" fallback dropped the
   last byte (`BUGFIX #2`)** — when `NewRecord`'s `RecordEnd` is unset
   (0), `ParseFields()` falls back to treating the entire file as a
   single record, the same fallback shape already fixed in
   `doctype/colondoc.cxx`'s `BUGFIX #1`. This copy still had the old
   `RecEnd = ftell(fp) - 1;`, silently dropping the record's last byte
   — confirmed via a before/after test-revert: a record's last field
   ("Jane Doe") came back one character short ("Jane Do") with the fix
   reverted. Fixed identically to `colondoc.cxx`: `RecEnd = ftell(fp);`.
   `GPTYPE` is also unsigned (`UINT4`, `src/defs.hxx`), so for a
   genuinely empty file the old `- 1` underflowed `RecEnd` to
   `UINT_MAX` — investigated further with a standalone repro, since the
   obvious expectation was a confirmable ASan heap-buffer-overflow (the
   same shape as the digest-family bugs elsewhere in this catalog): the
   underflowed `RecLength` then gets `+ 1`'d for the `new CHR[...]`
   call, and *that* addition wraps back to 0 too (`UINT_MAX + 1` inside
   a 32-bit `GPTYPE` is 0), so the buggy code actually allocated a
   genuine 0-byte buffer rather than attempting a multi-gigabyte one.
   Writing `RecBuffer[0] = '\0'` immediately afterward is undefined
   behavior (one byte past a 0-byte allocation) either way, but a
   standalone repro confirmed this specific build's allocator/ASan
   combination doesn't flag it in practice (likely allocator slack
   around a 0-byte `new[]`) — so, unlike the digest-family bugs, this
   half of the fix is **not** ASan-confirmed, and the source/test
   comments say so explicitly rather than overclaiming. Fixed
   regardless, since UB not currently observed isn't UB relied upon.
   `BUGFIX #2` in source.

3. **`StartTag` read uninitialized when `sscanf()` fails to parse
   (`BUGFIX #3`)** — reachable via `BUGFIX #2`'s fallback once it stops
   crashing/allocating garbage first: for an empty `RecBuffer`, `tag`
   is an empty string, and `sscanf(tag, "%d", &StartTag)` returns
   immediately without ever writing to `StartTag` (confirmed with a
   standalone repro using a sentinel value, which came back completely
   untouched). The following `if (StartTag != 1)` check then read
   indeterminate stack memory. Fixed by initializing `StartTag = 0`,
   which deterministically fails that check and takes the existing
   "bogus first tag" error path — matching the file's own established
   convention for a malformed/empty record. `BUGFIX #3` in source.

4. **`ParseFields()`'s "bogus first tag" early return leaked
   `RecBuffer` (`BUGFIX #4`)** — found incidentally while testing
   `BUGFIX #3`: `make tests-asan` reported a real 1-byte leak (matching
   exactly the `new CHR[1]` allocated for the empty-file test case) from
   this exact path. `delete [] RecBuffer;` was present on the sibling
   "no tags found" early-return branch just below, but missing here.
   Confirmed via a before/after test-revert: removing the `delete []`
   reproduced the identical ASan leak report. `BUGFIX #4` in source.

Also applied: `GPTYPE bytePos`/`marcLength`/`RS` (was `int`), removing
a `-Wsign-compare` warning against `FileLength` (`GPTYPE`) without any
behavior change, since all three are always non-negative in practice.
Removed the entirely-unused `RE` local. Removed several other
`-Wunused-variable`/`-Wunused-but-set-variable` locals not otherwise
touched (`Status` captures in `Present()`/`PresentSutrs()`/
`PresentHtml()` where the return value was never used, `nSubFields` in
`PresentHtml()`, `i` in `parse_tags()`, `here` in `HasSubfieldTag()`/
`GetSubfield()`) by simply not capturing values nothing read — same
call sites, same behavior. Left the remaining `-Wunused-parameter`
warnings (`BeforeSearching`'s `SearchQueryPtr`, `parse_tags`'s `len`,
`usefulMarcDumpField`'s `fieldStr`) as pre-existing/acceptable,
consistent with how the identical pattern is already left in place
across the base-class virtual defaults this file transitively includes
(`idbobj.hxx`, `doctype.hxx`, `opobj.hxx`). `NULL` converted to
`nullptr` at all 7 call sites. Added class-level doc comment to the
header and doc comments on `ParseRecords()`/`ParseFields()` in the
source (matching `colondoc.cxx`'s doc-comment depth — not every small
free-function helper got one, consistent with that file's own
precedent). `doctype/marcdump.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_marcdump.cxx` covers: `UnifiedName()`'s identity
passthrough; `ParseRecords()` correctly indexing a single-record file
and correctly indexing the *last* of several records (both direct
`BUGFIX #1` regressions); `ParseFields()` extracting a tag/value pair
with no trailing byte lost (`BUGFIX #2`'s directly-confirmed half);
`ParseFields()` adding a second alias field via
`marcdumpFieldNumToName()`; and `ParseFields()` not crashing on an
empty file (`BUGFIX #2`'s underflow path, with an accurate comment
about what is and isn't ASan-confirmed there). Field names are
asserted in uppercase per the established
`DF::SetFieldName()`-uppercases-internally gotcha.

## doctype/markdown.cxx

**No bugs found.** A small, modern addition (authored "Copilot", not
part of the original CNIDR/BSn codebase) — already compiled clean
under `-Wall -Wextra` and had no `NULL`/`sprintf` to modernize.
`MARKDOWN : public DOCTYPE` inherits `ParseRecords()`/`ParseFields()`
unchanged; only `Present()`'s `"B"` (brief) element set is overridden,
via the file-local `ExtractMarkdownBrief()`: returns the document's
first ATX heading (`#`/`##`/... line, stripped of the leading `#` run
and surrounding whitespace) if one exists anywhere in the text,
otherwise its first non-empty line, otherwise an empty string. Traced
by hand for memory safety (every exit path frees the `NewCString()`
buffer; `STRING::Set()` copies by explicit length, not relying on
null-termination at the substring's end, confirmed by reading
`STRING::Set()`/`Copy()` in `src/string.cxx`) and for logic (an
empty/all-blank record, a lone `"#"` with no heading text, a heading
appearing after an earlier plain line) — no crash, leak, or incorrect
result found in any case. The heading match is deliberately lenient
(doesn't require a space after the `#` run, unlike strict CommonMark)
-- a heuristic design choice with no bug report or spec to check it
against, not something to "fix" without inventing new behavior.

Added file-level and class-level doc comments to the header, and
function-level doc comments on `ExtractMarkdownBrief()` and
`Present()` in the source. `doctype/markdown.cxx` added to
`TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_markdown.cxx` covers: extracting a heading
(stripped of leading `##` and whitespace); falling back to the first
non-empty line when there's no heading; preferring a heading that
appears *after* an earlier plain line (confirms the "keep scanning,
don't stop at the first line" behavior); an all-blank record leaving
the brief empty; and a non-`"B"` element set deferring to
`DOCTYPE::Present()` without crashing. Unlike
`tests/doctype/test_emacsinfo.cxx`/`test_bibtex.cxx` (which left
`Present()` untested because `RESULT::GetRecordData()` crashes on a
default-constructed `RESULT`), this file's entire logic lives behind
`Present()` -- `ExtractMarkdownBrief()` is `static`, unreachable from
outside the translation unit -- so skipping it would leave no real
coverage at all. Built a real, file-backed `RESULT` instead (temp
file + `SetPathName()`/`SetFileName()`/`SetRecordStart()`/
`SetRecordEnd()`, mirroring the `RECORD` fixtures used elsewhere, with
`RecordEnd` set to `content.size() - 1` per `RESULT::GetRecordSize()`'s
documented inclusive-end convention), the same technique available to
those earlier files but not attempted there.

## doctype/memodoc.cxx

`MEMODOC : public DOCTYPE` parses "TAG: value" memo documents (see the
file-local `parse_tags()`'s own header comment for the grammar,
including the `"____"`/`"----"`/`"++++"`/`"===="` 4-char run marking
the end of the tagged header and the start of a free-text
`"Memo-Body"`). Shares clear code lineage with the already-processed
`doctype/colondoc.cxx` (same "TAG: value" shape, same value-extraction
arithmetic almost verbatim) and picked up the same cluster of
authoring bugs as a result. Seven bugs found, all fixed; four
confirmed via before/after test-reverts (two of them under ASan), the
other three fixed by direct cross-reference to `colondoc.cxx`'s
already-confirmed identical fixes (impractical to independently
revert-test here for the reasons noted per bug below, same as there).

1. **`ParseFields()`'s "whole file as one record" fallback dropped the
   last byte (`BUGFIX #1`)** — `RecEnd = ftell(fp) - 1;`, the same
   off-by-one already fixed in `colondoc.cxx`'s `BUGFIX #1` and
   `marcdump.cxx`'s `BUGFIX #2`. Confirmed via a before/after
   test-revert: a record's only field ("Jane Doe") came back one
   character short ("Jane Do"). Fixed identically: `RecEnd =
   ftell(fp);`. `BUGFIX #1` in source.

2. **`parse_tags()`'s scan loop underflowed for any record under 4
   bytes (`BUGFIX #2`, most severe)** — `for (GPTYPE i = 0; i < len -
   4; i++)`; `len` is `GPTYPE` (unsigned `UINT4`), so for a record
   shorter than 4 bytes (trivially reached via `BUGFIX #1`'s fallback
   once it stops truncating first — a genuinely empty file gives
   `len == 0`), `len - 4` underflowed to just under `UINT_MAX`, and the
   loop body's `b[i+1]`/`b[i+2]`/`b[i+3]` reads ran past `RecBuffer`'s
   real allocation on its very first iteration. Confirmed via a
   before/after test-revert under ASan: a genuine heap-buffer-overflow,
   `AddressSanitizer: heap-buffer-overflow`, aborting the process.
   Rewritten as `i + 4 < len`, an exactly equivalent comparison for
   every `len` that doesn't underflow, since it never subtracts from
   the unsigned `len` at all. `BUGFIX #2` in source.

3. **`parse_tags()`'s 4-char separator check had a copy-paste typo
   (`BUGFIX #3`)** — the last disjunct checked `b[i+2] == '-'` instead
   of `b[i+3] == '-'`, unlike the three lines above it (each
   consistently checks `b[i+N]` against all four separator characters
   for its own `N`). Whenever the 3rd character of a would-be 4-char
   run was specifically `'-'`, the whole disjunct was satisfied
   regardless of the 4th character's real value, so a run like
   `"__-X"` (`X` being anything at all) wrongly matched as a section
   break — ending tag-hunting right there and silently dropping every
   tag after it. Confirmed via a before/after test-revert: a field
   placed after such a run went from found to not-found. Fixed by
   correcting the typo. `BUGFIX #3` in source.

4. **Value-extraction's end-of-buffer fallback used the wrong length
   (`BUGFIX #4`)** — `p = &RecBuffer[RecLength]` (the buffer's
   *allocated capacity*) for the last tag's value, not
   `&RecBuffer[ActualLength]` (how much `fread()` actually returned) —
   same fix as `colondoc.cxx`'s `BUGFIX #3`, for the same reason:
   harmless whenever the two are equal (the normal case, and the only
   case reachable from this file's own tests, same caveat noted in
   `colondoc.cxx`'s own entry), but wrong in principle for a short
   read. Fixed to match `ActualLength`. Not independently revert-tested
   for the same reason `colondoc.cxx`'s wasn't: no test in this file's
   suite can force `fread()` to under-read a real, fully-available
   file. `BUGFIX #4` in source.

5. **Value length unconditionally excluded a trailing newline that
   might not exist (`BUGFIX #5`)** — `INT val_len = (p - *tags_ptr) -
   off - 1;`, the trailing `- 1` assuming a real `'\n'` always sits
   just before `p`. True for every interior field (the format
   guarantees exactly one `'\n'` before the next `"Tag:"` line), but
   not for the *last* field's end-of-buffer fallback when the file
   doesn't end with `'\n'` — same fix as `colondoc.cxx`'s `BUGFIX #1b`.
   Confirmed via a before/after test-revert, independently of
   `BUGFIX #1` (reverting *only* this fix, with `BUGFIX #1` still
   applied and the file fully, correctly read): the same "Jane Doe" →
   "Jane Do" symptom, proving both bugs were independently necessary,
   not just two ways of describing the same defect. Fixed by only
   excluding the `'\n'` when `p[-1]` actually is one. `BUGFIX #5` in
   source.

6. **Trailing-whitespace trim checked one byte past the value's real
   end (`BUGFIX #6`)** — `isspace(RecBuffer[val_len + val_start])`
   instead of `RecBuffer[val_start + val_len - 1]` — same off-by-one
   already fixed in `colondoc.cxx`'s `BUGFIX #2`. Per that entry, this
   forms a pair with `BUGFIX #7` below that silently canceled out in
   the common case (trimming one byte too early, then storing one byte
   too many, netting out to the *apparently* correct span whenever
   there actually was trailing whitespace to trim) — which is almost
   certainly why neither was ever noticed by hand-testing. Not
   independently revert-tested here for that reason (same as
   `colondoc.cxx`): a test that happens to exercise both at once can't
   distinguish them, and this file's own `BUGFIX #1`/`BUGFIX #5` tests
   don't have trailing whitespace to trim in the first place, so this
   fix's effect is covered only jointly with `BUGFIX #7`, not in
   isolation. `BUGFIX #6` in source.

7. **`FC::SetFieldEnd()` stored one byte past the correct inclusive end
   (`BUGFIX #7`)** — `fc.SetFieldEnd(val_start + val_len)`, one past
   `val_start + val_len - 1` — same fix as `colondoc.cxx`'s `BUGFIX #4`
   for the same reason (matches `sgmlnorm.cxx`'s/`sgmltag.cxx`'s own
   `val_start + val_len - 1`, and `src/index.cxx` derives a field's
   length as `GetFieldEnd() - GetFieldStart() + 1`, which only works
   for an inclusive end). Together with `BUGFIX #6`, this changes
   `tests/doctype/test_memodoc.cxx`'s `ExtractField()` helper from an
   exclusive-end convention (matching `marcdump.cxx`'s FC convention,
   copied from that file's own test as a starting point) to an
   inclusive one (`+ 1`) — the fully-fixed test suite passing with that
   adjustment is the closest thing to a joint confirmation these two
   paired fixes get. `BUGFIX #7` in source.

`NULL` converted to `nullptr` at all 5 call sites. Added class-level
doc comment to the header and a `ParseFields()` doc comment in the
source. `doctype/memodoc.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_memodoc.cxx` covers: extracting a tagged field with
no trailing byte lost (`BUGFIX #1`/`BUGFIX #5`, confirmed independently
of each other); not crashing on an empty file and on a short non-empty
record (`BUGFIX #2`'s heap-overflow path, reached two different ways);
not treating a run ending in a non-separator character as a section
break (`BUGFIX #3`); and correctly treating a genuine 4-char run as a
section break, starting a `"Memo-Body"` field. Field names are asserted
in uppercase per the established
`DF::SetFieldName()`-uppercases-internally gotcha (initially missed
while writing this file's tests — all three of the first draft's
`FindField()` calls failed before this was caught and fixed).

## doctype/oneline.cxx

`ONELINE : public DOCTYPE` indexes one record per newline-terminated
line (e.g. a phonebook-style file). `ParseFields()`/`Present()` are
inherited unchanged from `DOCTYPE`; only `ParseRecords()` is
overridden.

1. **Phantom `Position` increment on EOF corrupted the "is there a
   record" check (`BUGFIX #1`)** — the line-scanning `for` loop's
   increment clause runs `ci=fgetc(Fp), Position=Position+1` together,
   so the read that finally returns `EOF` still bumps `Position` once
   even though it consumed no real byte. `Position` therefore ends up
   one past the true count of bytes read whenever the scan ends via
   `EOF` rather than a real `'\n'`. `GPTYPE` is unsigned (`UINT4`,
   `src/defs.hxx`), and this over-count corrupted two different checks
   built on it:
   - For a genuinely empty file, `Start (0) != Position` became
     wrongly true (`Position` went from `0` to a phantom `1`), and the
     pre-existing `Pos = Position - 2` compensation for the `EOF`
     branch then underflowed (`1 - 2`) to `UINT_MAX`, adding a bogus
     record ending ~4 billion bytes past the real, zero-byte file.
   - Far more commonly: for *any* file whose very last byte is `'\n'`
     (the normal case for real text files), the same over-count meant
     one extra pass through the outer loop found `Start != Position`
     true again after the real last line had already been correctly
     recorded, adding a spurious trailing record with `RecordEnd <
     RecordStart`.
   Confirmed via a before/after test-revert: reverting to the original
   `Pos = Position - 1`/`Position - 2` branching (without correcting
   `Position` first) reproduced both symptoms — an empty file gaining a
   bogus record and a newline-terminated file gaining a third,
   3rd-record, extra one where only two lines exist. Fixed by
   correcting the phantom increment immediately (`if (ci == EOF)
   Position = Position - 1;`) right after the scan loop, before either
   check runs — this also let the two now-redundant `Pos` branches
   collapse into one uniform `Pos = Position - 1`, since `Position`
   accurately reflects real bytes read either way once corrected.
   `BUGFIX #1` in source.

Added class-level doc comment to the header and a `ParseRecords()` doc
comment in the source, including the file's existing "`RecordEnd` lands
on the newline itself" convention (pre-existing, unrelated to the fix,
documented as-is). `doctype/oneline.cxx` added to
`TEST_ENGINE_DOCTYPE_SRCS`. No `NULL`/`sprintf` to modernize; file
already compiled clean under `-Wall -Wextra`.

`tests/doctype/test_oneline.cxx` covers: splitting a two-line file into
two records with exact byte offsets; indexing a final line with no
trailing newline; no bogus record for an empty file (`BUGFIX #1`'s
underflow path); no spurious trailing record when the file ends in a
newline (`BUGFIX #1`'s more commonly-hit path); and consecutive blank
lines each getting their own minimal record.

## doctype/para.cxx

`PARA : public DOCTYPE` indexes one record per paragraph, splitting on
a blank line (`"\n\n"`). `ParseFields()`/`Present()` are inherited
unchanged from `DOCTYPE`; only `ParseRecords()` is overridden. Two bugs
found, both confirmed via before/after test-reverts (one under ASan).

1. **`(i-1) > Start` underflowed for a "\n\n" marker at the very start
   of the file (`BUGFIX #1`)** — `i` and `Start` are both `GPTYPE`
   (unsigned `UINT4`, `src/defs.hxx`); the guard's purpose is to reject
   a degenerate/near-empty paragraph, but when the marker was found at
   `i == 0` (a file beginning with a blank line), `i - 1` underflowed
   to `UINT_MAX`, always greater than `Start` — wrongly *passing* the
   very check meant to reject this case, adding a bogus, empty
   "paragraph" record spanning just the two leading newline characters.
   Confirmed via a before/after test-revert: a file starting with
   `"\n\n"` gained an extra record. Rewritten as `i > Start + 1`, an
   exactly equivalent comparison for every `i` that doesn't underflow,
   since it never subtracts from the unsigned `i` at all — with the fix,
   a leading blank line is never treated as its own boundary; it's
   simply absorbed into whichever real paragraph follows it (one
   record, not two). `BUGFIX #1` in source.

2. **`RecBuffer` was never freed on the success path (`BUGFIX #2`)** —
   found incidentally while running `make tests-asan` on this file's
   new tests: `RecBuffer` (`new CHR[RecLength + 2]`) is explicitly
   `delete []`'d on the early-error paths (failed or short `fread()`),
   but the function falls straight through to its closing brace on the
   normal, successful path — a straightforward leak on every file this
   function successfully processes. Confirmed via a before/after
   test-revert under ASan: `AddressSanitizer` reported real, direct
   leaks matching exactly the allocations from the reverted run's own
   test temp files. Fixed by adding `delete [] RecBuffer;` at the end
   of the function. `BUGFIX #2` in source.

Also documented (not changed): `RecordEnd` for a paragraph lands on the
last newline of its trailing separator run, not the paragraph's own
last character — the same "`RecordEnd` lands on the delimiter itself"
convention already documented for `doctype/oneline.cxx`, confirmed here
by tracing the exact byte offsets a real two-paragraph test produces
(caught an initially-wrong test assertion that assumed the opposite
convention before this was traced through properly). No `NULL`/
`sprintf` to modernize. `doctype/para.cxx` added to
`TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_para.cxx` covers: splitting two paragraphs at a
blank line with exact byte offsets (including the trailing-separator
convention above); no bogus empty record for a file starting with a
blank line, which merges into the one real paragraph that follows
(`BUGFIX #1`); a single-paragraph file with no blank line anywhere
indexed as one record; and no bogus record for an empty file (the
pre-existing `RecEnd == 0` guard already handled this correctly, unlike
several sibling files this batch — confirmed, not assumed).

## doctype/referbib.cxx

`REFERBIB : public DOCTYPE` parses Unix `refer`-style bibliography
records: `ParseRecords()` splits a file into one record per
blank-line-separated entry, and `ParseFields()` splits each record's
"%X value" tag lines into fields. Shares the same "TAG: value"
value-extraction lineage as `doctype/colondoc.cxx` and
`doctype/memodoc.cxx` (four of the six bugs below are that same
cluster, cross-referenced rather than re-derived), plus one genuinely
novel bug of its own in `ParseRecords()`. Six bugs found, all fixed and
confirmed via before/after test-reverts (one under ASan).

1. **`ParseRecords()`'s post-loop flush checked the wrong variable
   (`BUGFIX #1`, novel to this file)** — `RecordEnd = (SavePosition ==
   0) ? 0 : Position - 1;` checks `SavePosition` but decrements
   `Position` — two different variables. `SavePosition` is only ever
   assigned *inside* the loop, when a blank-line record boundary is
   actually found; `Position` accumulates on every byte read
   regardless. For a file containing exactly one bibliography entry
   with no internal blank line (nothing unusual — a small database, or
   just the last entry with no trailing separator after it),
   `SavePosition` stays `0` for the whole file even though `Position`
   correctly reaches the real end-of-file offset, so this wrongly took
   the `RecordEnd = 0` branch. That then failed the `RecordEnd > Start`
   guard immediately below, and the record was silently never indexed
   at all. Confirmed via a before/after test-revert: a single-entry
   file with no blank line produced zero calls to `DocTypeAddRecord()`.
   Fixed by checking the same variable being decremented: `RecordEnd =
   (Position == 0) ? 0 : Position - 1;` — matching the already-correct
   mid-loop guard just above it, which uses `SavePosition` for *both*
   halves consistently. `BUGFIX #1` in source.

2. **`ParseFields()`'s "whole file as one record" fallback dropped the
   last byte (`BUGFIX #2`)** — `RecEnd = ftell(fp) - 1;`, the same
   off-by-one already fixed in `colondoc.cxx`'s `BUGFIX #1`,
   `marcdump.cxx`'s `BUGFIX #2`, and `memodoc.cxx`'s `BUGFIX #1`.
   Confirmed via a before/after test-revert. Fixed identically: `RecEnd
   = ftell(fp);`. `BUGFIX #2` in source.

3. **`parse_tags()`'s scan loop underflowed for any record under 4
   bytes (`BUGFIX #3`)** — `for (GPTYPE i = 0; i < len - 4; i++)`; same
   bug already fixed in `memodoc.cxx`'s `BUGFIX #2`. Confirmed via a
   before/after test-revert under ASan: a genuine heap-buffer-overflow
   aborting the process. Fixed identically: `i + 4 < len`. `BUGFIX #3`
   in source.

4. **Value-extraction's end-of-buffer fallback used the wrong length
   (`BUGFIX #4`)** — `p = &RecBuffer[RecLength]` instead of
   `&RecBuffer[ActualLength]`; same fix as `colondoc.cxx`'s `BUGFIX #3`
   and `memodoc.cxx`'s `BUGFIX #4`, not independently revert-tested for
   the same reason neither of those was (harmless whenever the two
   lengths are equal, the only case reachable from this file's own
   tests). `BUGFIX #4` in source.

5. **Value length unconditionally excluded a trailing newline that
   might not exist (`BUGFIX #5`)** — the trailing `- 1` in `INT val_len
   = (p - *tags_ptr) - off - 1;` assumed a real `'\n'` always sits just
   before `p`; same fix as `colondoc.cxx`'s `BUGFIX #1b` and
   `memodoc.cxx`'s `BUGFIX #5`. `BUGFIX #5` in source.

6. **Trailing-whitespace trim checked one byte past the value's real
   end, and `FC::SetFieldEnd()` stored one byte past the correct
   inclusive end (`BUGFIX #6`)** — `isspace(RecBuffer[val_len +
   val_start])` and `SetFieldEnd(val_start + val_len)`; same paired fix
   as `colondoc.cxx`'s `BUGFIX #2`/`BUGFIX #4` and `memodoc.cxx`'s
   `BUGFIX #6`/`BUGFIX #7` — per those entries, these two silently
   cancel out in the common case (trimming one byte too early, then
   storing one byte too many), which is almost certainly why neither
   was ever noticed by hand-testing. `BUGFIX #6` in source (both
   sites).

`NULL` converted to `nullptr` at all 15 call sites (`sed`, given the
volume — verified by re-reading the full diff afterward). Added
class-level doc comment to the header and `ParseRecords()`/
`ParseFields()` doc comments in the source. `doctype/referbib.cxx`
added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_referbib.cxx` covers: `UnifiedName()`'s tag-to-
field-name mapping, including a reserved-but-ignored tag returning
`nullptr`; a single reference with no internal blank line
(`BUGFIX #1`'s direct regression); splitting two references at a
blank line; extracting a tagged field with no trailing byte lost
(`BUGFIX #2`); not crashing on an empty file (`BUGFIX #3`'s heap-
overflow path); a recognized tag under its unified name; and an
unrecognized *lowercase* tag falling through to `"Misc"` (the table is
indexed by uppercase letter only — an initially-wrong test used an
already-recognized uppercase tag by mistake, caught and corrected
before it was ever run). Field names are asserted in uppercase per the
established `DF::SetFieldName()`-uppercases-internally gotcha (applied
proactively this time, not caught after a failing run).

## doctype/simple.cxx

`SIMPLE : public DOCTYPE` presents a document's leading text as a
headline (element set `"B"`): the first `NumLines` lines, `NumLines`
read once at construction from the doctype's `"LINES"` option (default/
minimum 1). Record splitting and field parsing are entirely inherited
from `DOCTYPE`'s own defaults; only `Present()`/`BeforeRset()`/
`AfterRset()` are overridden.

1. **Multi-line headlines silently dropped every line after the first
   (`BUGFIX #1`)** — the inner `while` loop scanning each line stops
   as soon as it sees `'\n'`, but its condition fails *before* the
   loop body's `z++` runs for that character, so `z` is left pointing
   *at* the newline, not past it. The outer `for (y=1; y<=NumLines;
   y++)` loop had nothing that advanced `z` past this point between
   iterations, so for `NumLines > 1` — the entire reason the `"LINES"`
   option exists — every iteration after the first re-tested the exact
   same `'\n'`, found its condition false immediately, and appended
   nothing. Confirmed via a before/after test-revert: with the fix
   reverted, a `LINES=2` request for a two-line file came back with
   only the first line's text instead of both concatenated. Fixed by
   skipping past the newline once the inner loop stops on one: `if (c
   == '\n') z++;`. `BUGFIX #1` in source.

`STRING::GetChr()`'s documented "returns 0 for an out-of-range index"
contract (`src/string.hxx`) means the leading-whitespace-skip loop
(`while (isspace(StringBuffer->GetChr(x))) x++;`) is already safe for
an all-whitespace or empty record — traced, not assumed, and confirmed
by a dedicated test. `UCHR` is already `unsigned char`
(`src/gdt.h`), so the `isspace()` calls need no signedness cast. No
`NULL`/`sprintf` to modernize. Added class-level doc comment to the
header and a `Present()` doc comment in the source.
`doctype/simple.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_simple.cxx` covers: the default `LINES=1` case
returning just the first line; skipping leading whitespace before the
headline; `LINES=2` correctly including the second line
(`BUGFIX #1`'s direct regression); `LINES` exceeding the file's real
line count not crashing (the inner loop's `GetChr()` past-EOF `0`
return stops it safely); and a non-`"B"` element set deferring to
`DOCTYPE::Present()`. `Present()`'s `"B"` path calls
`RESULT::GetRecordData()` directly (same as
`doctype/markdown.cxx`), so these tests build a real, file-backed
`RESULT`, and a `TESTIDBOBJ::GetDocTypeOptions()` override returns a
`"LINES=<n>"` STRLIST entry (matching the `"KEY=VALUE"` format
`STRLIST::GetValue()` parses, confirmed by reading
`src/strlist.cxx`) so each test can control `NumLines` at construction.

## doctype/soif.cxx

`SOIF : public DOCTYPE` parses Harvest SOIF (Summary Object Interchange
Format) records — unlike most other doctypes, `ParseRecords()` doesn't
split a file into multiple records at all; it always treats the whole
file as one. `ParseFields()` parses a leading `"@FILE { <url>\n"` line
(mapped to a `"url"` field) and `"name{len}:\t"` attribute headers.
Three distinct memory-leak bugs found, all confirmed via before/after
test-reverts under ASan (one reproducing dramatically — 22,968 bytes
across 309 allocations across the whole suite, since the leaked `DFT`
transitively owns every `FCT`/`DF` copied into it).

1. **`ParseRecords()` never freed `RecBuffer` on the success path
   (`BUGFIX #1`)** — same shape as `doctype/para.cxx`'s `BUGFIX #2`:
   `RecBuffer` (`new CHR[RecLength + 2]`) is `delete []`'d on the
   early-error paths (failed/short `fread()`) but the function falls
   straight through to its closing brace on the normal, successful
   path — a leak on every file this function successfully processes.
   Confirmed via a before/after test-revert under ASan. Fixed by adding
   `delete [] RecBuffer;` at the end of the function. `BUGFIX #1` in
   source.

2. **`ParseFields()`'s three malformed-record error paths freed
   `RecBuffer` but never `pdft` (`BUGFIX #2`)** — `pdft` (`new DFT()`)
   is allocated once, above the parsing loop, and correctly freed on
   the success path, but all three "badly started/ended/formatted
   record" early returns inside the loop free only `RecBuffer`, never
   `pdft` — a leak reachable on any malformed SOIF input. Confirmed via
   a before/after test-revert under ASan (reverting all three sites at
   once produced the 309-allocation leak mentioned above, since by that
   point in the suite's run many prior tests' `DFT`s had also gone
   through this same code path). Fixed by adding `delete pdft;` at each
   of the three sites. `BUGFIX #2` in source.

3. **`ParseFields()`'s `file` (a `NewCString()` copy of the path, used
   only for `perror()`) was allocated unconditionally and never freed
   at all (`BUGFIX #3`)** — allocated once near the top of the
   function regardless of whether either of its two use sites (both
   `perror(file)` calls, both inside early-return error branches) is
   ever reached, and never `delete []`'d anywhere in the function —
   leaking on *every single call* to `ParseFields()`, not just an error
   path. Confirmed via a before/after test-revert under ASan. Fixed by
   allocating it lazily, right before each of its two uses, and freeing
   it immediately after. `BUGFIX #3` in source.

Also removed two entirely-unused local variables in `ParseRecords()`
(`i`, `lastBrace`) that were triggering real `-Wunused-variable`
warnings — dead leftovers, confirmed via `grep` to have no other
reference anywhere in the file. The apparent `%u`-vs-signed-`INT`
`sscanf()` format mismatch in `ParseFields()` (`INT val_len` passed to
a `"%u"` conversion) was investigated but not changed: neither
`-Wall -Wextra` nor `-Wformat=2` flag it (confirmed with a standalone
repro), since `int`/`unsigned int` share an identical bit
representation on this platform — a real hardening target for a
future architecture, not a demonstrated bug here, so left alone rather
than inventing a fix with no observable effect. Replaced the stale
comment above `ParseFields()` (leftover from an earlier, different
title-tag-based design that was never updated to describe the actual
`name{len}:\t` implementation) with an accurate one, and added a
class-level doc comment to the header. `NULL` converted to `nullptr`.
`doctype/soif.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_soif.cxx` covers: `ParseRecords()` adding the whole
file as a single record; extracting the `@FILE` `"url"` field and a
`"name{len}:\t"` attribute field; not crashing (or leaking, under ASan)
on each of the three malformed-record error paths (`BUGFIX #2`) and on
a file that can't be opened (`BUGFIX #3`); and `Present()`'s `"F"`
(whole record) and no-fields-yet-registered paths.

## doctype/taglist.cxx

`TAGLIST : public SGMLTAG` indexes only tag pairs whose name matches a
fixed allowlist (`TITLE`/`H1`-`H4`), tracking each match's byte range in
`m_TagPos` so `ParseWords()` restricts word extraction to just those
ranges. Explicitly derived from `doctype/sgmltag.cxx` (per its own file
header) — most of `ParseFields()` is a near-verbatim copy of
`SGMLTAG::ParseFields()`, which is *already processed* and correct.
Five bugs found, four of them exactly the shape of "this copy of an
already-fixed function quietly dropped something the original has,"
confirmed by direct comparison against `sgmltag.cxx`. All five
confirmed via before/after test-reverts (three under ASan, one via an
isolated subprocess since it kills the process outright).

1. **`ParseFields()` never called `NewRecord->SetDft(*pdft)`
   (`BUGFIX #1`, most severe)** — `pdft` is built up field-by-field
   through the whole parsing loop, exactly as in `SGMLTAG::ParseFields()`,
   but then simply `delete`'d without ever being attached to
   `NewRecord`. This is not a partial or edge-case bug: **every field
   TAGLIST ever parsed, for any input, was silently thrown away** — the
   class has never actually indexed anything since whenever this line
   was dropped relative to the base class. Confirmed via a before/after
   test-revert: reverting just this one line took a passing "field
   extracted correctly" test straight to "zero fields found." Fixed by
   adding `NewRecord->SetDft(*pdft);` immediately before the existing
   `delete pdft;`, matching `SGMLTAG::ParseFields()`'s exact placement.
   `BUGFIX #1` in source.

2. **`ReplaceWithSpace()`'s kept-tag branch wrote through a garbage
   pointer (`BUGFIX #2`)** — `*tags_ptr[strlen(*tags_ptr)] = ' ';`;
   array subscript binds tighter than unary `*` in C++, so this parsed
   as `*(tags_ptr[strlen(*tags_ptr)])` — indexing `strlen(*tags_ptr)`
   *slots ahead in the `tags_ptr` array itself* (not into the current
   tag string's own buffer, which was clearly the intent given the
   comment right above it), then dereferencing and writing to whatever
   garbage or out-of-bounds pointer happened to occupy that slot.
   Confirmed via a before/after test-revert under ASan: a real `SIGSEGV`
   (`AddressSanitizer: SEGV on unknown address`), not a subtle
   corruption — this crashed immediately and reliably on ordinary input
   (`"<TITLE>...</TITLE>"`, `"TITLE"` being 5 characters, so the buggy
   read landed 5 array-slots past the current tag). Fixed by
   parenthesizing to dereference `tags_ptr` first:
   `(*tags_ptr)[strlen(*tags_ptr)] = ' ';`. `BUGFIX #2` in source.

3. **`ParseFields()` freed `m_TagPos` with scalar `delete` instead of
   `delete []` (`BUGFIX #3`)** — `m_TagPos` is always allocated via
   `new EntryType[numtags]` (both here and, correctly, in
   `TAGLIST::~TAGLIST()`), but the cleanup immediately before
   reallocating it on a second `ParseFields()` call used plain
   `delete m_TagPos;` — a `new[]`/`delete` mismatch, undefined behavior,
   only reachable when `ParseFields()` runs more than once on the same
   `TAGLIST` instance (`m_TagPos` is null the first time, so the buggy
   line is skipped entirely on a single call). Confirmed via a
   before/after test-revert under ASan: `AddressSanitizer:
   alloc-dealloc-mismatch`, pointing at the exact allocation and
   deallocation sites. Fixed to match the destructor's already-correct
   `delete [] m_TagPos;`. `BUGFIX #3` in source.

4. **`ParseWords()` called `exit(1)` on GP-buffer overflow instead of
   returning a sentinel (`BUGFIX #4`)** — same bug already fixed in
   `DOCTYPE::ParseWords()` (`doctype/doctype.cxx`, `BUGFIX #1`):
   `exit()`ing here aborted the *entire* `Iindex` process, losing all
   indexing progress, the moment a single document had more matched
   terms than fit in the current GP buffer, even though the caller
   (`INDEX::BuildGpList()`, `src/index.cxx`) already checks for and
   recovers from a `(GPTYPE)-1` sentinel return. Confirmed via an
   isolated subprocess run (a full in-process revert-test would have
   killed this very test suite): with `exit(1)` restored, the process
   exited with status 1 and printed no Catch2 summary at all, having
   been killed mid-run; with the fix, the same scenario returns
   `(GPTYPE)-1` and the suite continues normally. Fixed identically to
   `doctype.cxx`'s already-established fix: `return (GPTYPE)-1;`.
   `BUGFIX #4` in source.

5. **`ParseFields()`'s `file` was allocated but never freed
   (`BUGFIX #5`)** — same bug as `doctype/soif.cxx`'s `BUGFIX #3`, but
   here the correct fix was already sitting right there in the base
   class: `SGMLTAG::ParseFields()` correctly frees `file` at every one
   of its exit points, and this copy of the logic dropped every single
   one of those `delete [] file;` calls, the same way it dropped
   `SetDft()` (`BUGFIX #1`). Leaked on *every* call, not just an error
   path. Confirmed via a before/after test-revert under ASan. Fixed by
   adding `delete [] file;` at all 11 exit points, mirroring
   `sgmltag.cxx`'s placement exactly rather than re-deriving it.
   `BUGFIX #5` in source.

Also documented, not changed: `OrigRecBuffer` (a second, byte-identical
copy of the record, allocated, filled, and null-terminated) is never
read anywhere in the function beyond its own cleanup — confirmed via
`grep` across the whole file. Unlike `doctype/litmed.cxx`'s
`find_next_tag()` (removed, because a compiler warning plus a clear
cross-file comparison confirmed it was genuinely dead), there's no
compiler warning here (the variable *is* used, just not meaningfully)
and no confirmable reason it exists — the file's own top comment even
calls out "Read two copies of the file into memory (implementation
feature;-)" with what reads as the original author's own wry
acknowledgment — so this is left in place rather than guessed at.
`UsefulSearchField()`'s substring (not exact) match against
`ValidTags[]` (so e.g. a hypothetical `SUBTITLE` tag would match
`TITLE`) is also left as-is: a plausible-enough design choice with no
spec or test suggesting otherwise. `NULL` converted to `nullptr` at all
5 live call sites (3 in comments left untouched). Added class-level doc
comment to the header and function-level doc comments in the source.
`doctype/taglist.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.

`tests/doctype/test_taglist.cxx` covers: fields actually reaching the
record's `DFT` (`BUGFIX #1`'s direct regression); an unlisted tag being
ignored; `ReplaceWithSpace()` not corrupting memory on an allowlisted
tag (`BUGFIX #2`); `ParseFields()` called twice on the same instance
not corrupting memory (`BUGFIX #3`); and `ParseWords()` returning the
sentinel instead of exiting when its GP buffer is full (`BUGFIX #4`).

## doctype/usmarc.cxx

`USMARC : public DOCTYPE` parses USMARC/MARC21 library records.
Uniquely among every file processed this batch, its parsed record
state (`RecBuffer`, `marcDir`, `marcNumDirEntries`, `marcBaseAddr`,
`marcRecordLength`) lives in **globals** (declared in
`doctype/usmarc.hxx`), not `USMARC` instance members, explicitly shared
between a `ParseFields()` call (which populates them) and the
`ParseWords()` call expected to follow it for the same record (which
frees them). This turned out to be the most heavily-bugged file of the
whole batch: **nine** distinct bugs, several found only by actually
running the test suite against real bugs rather than by static
reading — a crash and a heap-buffer-overflow this session's own tests
triggered mid-development, not just pre-planned repros. All nine
confirmed real, six via before/after test-reverts (four under ASan).

1. **`ParseWords()` never checked `GpListSize` against `GpLength`
   before writing (`BUGFIX #1`, tied for most severe)** — confirmed via
   `-Wunused-parameter`: `GpLength` (the caller-provided capacity of
   `GpBuffer`) was referenced nowhere in the function. Every sibling
   `ParseWords()` (`doctype.cxx`, `doctype/taglist.cxx`) guards the
   equivalent write; this one had no guard at all — an unconditional
   heap-buffer-overflow write on any record with more indexable words
   than the caller's buffer could hold. Confirmed via a before/after
   test-revert under ASan (a real stack-buffer-overflow with a
   1-element test buffer). Fixed with the same guard-and-sentinel
   pattern already established in `doctype.cxx`'s `BUGFIX #1` and
   `taglist.cxx`'s `BUGFIX #4`. `BUGFIX #1` in source.

2. **`ParseWords()` passed the wrong bound to `IsStopWord()`
   (`BUGFIX #2`)** — `Db->IsStopWord(DataBuffer + Position, DataLength)`
   instead of `DataLength - Position` (the remaining length from here);
   every sibling caller correctly subtracts `Position`. Confirmed by
   reading the real implementation, `INDEX::IsStopWord()`
   (`src/index.cxx`): it uses the bound as a hard read limit for its own
   alphanumeric scan, *and* temporarily writes a `'\0'` partway through
   at the scanned word's end — so the oversized bound risked both an
   out-of-bounds read and an out-of-bounds write, not just a read.
   Fixed to match the established, correct pattern. `BUGFIX #2` in
   source.

3. **`RecBuffer`/`marcDir` leaked across calls, and a failed
   `readFileContents()` left `readMarcStructure()` no way to detect it
   (`BUGFIX #3`, tied for most severe)** — two problems sharing one
   root cause and one fix. `RecBuffer` and `marcDir` are globals,
   reassigned by `readFileContents()`/`readMarcStructure()` on every
   `ParseFields()` call with no free of whatever they held from a
   previous call — a real leak across any multi-record MARC batch (the
   normal case for this DOCTYPE), since `ParseFields()` runs once per
   record while `ParseWords()` (which frees them) may not run again
   until much later, for a *different* record. Separately,
   `readFileContents()` has several early-return paths (file won't
   open, seek fails, empty file, allocation fails, short read), and
   `readMarcStructure()` used to proceed regardless, with
   `readRecordLength()`/`readBaseAddr()` immediately dereferencing
   whatever `RecBuffer` happened to hold. This was **not** a theoretical
   concern: a real test with a nonexistent file segfaulted here
   (`RecBuffer` null, since a prior call's `ParseWords()` had correctly
   freed-and-nulled it) before this fix existed. Fixed by freeing and
   nulling both globals up front (in `readFileContents()` for
   `RecBuffer`, at the top of `readMarcStructure()` for `marcDir`, both
   before any early return can skip past their real reallocation), and
   adding `if (RecBuffer == nullptr) { marcNumDirEntries = 0; return;
   }` right after the `readFileContents()` call — which also makes
   every downstream directory loop (`ParseFields()`, `ParseWords()`)
   safely do nothing for a record that couldn't be read, rather than
   crash. `BUGFIX #3` in source (in both functions).

4. **`ParseRecords()` never closed its file handle (`BUGFIX #4`)** —
   `fp` is opened once at the top and never `fclose()`'d anywhere: not
   on either early-return error path, not on normal end-of-loop
   completion. A file-descriptor leak on every call. Fixed by adding
   `fclose(fp);` at all three exit points. `BUGFIX #4` in source.

5. **`readFileContents()`'s `file` was allocated but never freed
   (`BUGFIX #5`)** — same bug as `doctype/soif.cxx`'s `BUGFIX #3` and
   `doctype/taglist.cxx`'s `BUGFIX #5`: a `NewCString()` copy of the
   path, used only for two `perror()` calls, leaked on every call
   regardless of which path was taken. Fixed identically: allocated
   lazily right where needed, freed immediately after. `BUGFIX #5` in
   source.

6. **`findNextTag()`'s scan loops had no bound but the MARC delimiter
   bytes themselves (`BUGFIX #6`)** — for a malformed/corrupted record
   missing an expected `END_OF_FIELD`/`START_OF_SUBFIELD` delimiter, the
   scan ran straight past the buffer's real allocation with nothing to
   stop it — not even `RecBuffer`'s own null terminator
   (`readFileContents()` always sets `RecBuffer[RecLength] = '\0'`, but
   nothing checked for it). A real heap-buffer-overflow read, confirmed
   via a before/after test-revert under ASan using a record whose field
   terminator was overwritten with an ordinary character. Fixed by also
   stopping at `'\0'`, with the surrounding `if`/`else` reworked to
   treat that the same safe way as a genuine `END_OF_FIELD`. Confirmed
   via test-revert to be independently necessary even with `BUGFIX #9`
   already applied. `BUGFIX #6` in source.

7. **A subfield-specific `addSearchEntry()` call used an inconsistent,
   exclusive end (`BUGFIX #7`)** — `tagPos, tagPos + tagLength` instead
   of `tagPos, tagPos + tagLength - 1`; the *other two*
   `addSearchEntry()` call sites in the very same function both
   correctly use `fieldPos + fieldLength - 1` (inclusive), matching
   `FC`'s established inclusive-end convention elsewhere in this tree.
   Since most `ParseData[]` entries name a specific subfield letter
   (this is the common path, not an edge case), every one of those
   fields had its stored span include one byte too many: the delimiter
   immediately following the real content. Confirmed via a before/after
   test-revert: a subfield's extracted text gained a trailing control
   character. `BUGFIX #7` in source.

8. See `BUGFIX #3` above — the crash this uncovered is documented there
   rather than as a separate entry, since one fix (the `nullptr` check)
   resolves both the leak and the crash together.

9. **`fieldPos` was shared and mutated across different `ParseData[]`
   entries instead of being reset per entry (`BUGFIX #9`, tied for most
   severe)** — `fieldPos` is computed once per MARC directory entry,
   *outside* the loop over `ParseData[]`, but passed by reference into
   `findNextTag()` inside that loop, which mutates it. Many real fields
   match *several* `ParseData[]` entries at once (all six "24*"/title
   entries, for subfield letters a/b/h/i/j/k, are typical), so once one
   entry's subfield search advanced `fieldPos`, the next entry's search
   resumed from wherever that left off instead of the field's own true
   start — silently wrong for a well-formed record (a later subfield
   appearing before that point would never be found), and, for a
   malformed one, capable of walking `fieldPos` further past the buffer
   on every successive iteration, eventually exceeding even `BUGFIX
   #6`'s own per-call bound. This is exactly what happened: `BUGFIX #6`
   alone left a real heap-buffer-overflow reachable, caught live by
   `make tests-asan` while testing that very fix, not found by
   inspection. Fixed with a fresh local scan cursor (`scanPos`), reset
   to `fieldPos`'s true value for every `ParseData[]` entry, leaving
   `fieldPos` itself (also used by the wildcard-tag `addSearchEntry()`
   call) untouched. Confirmed via a before/after test-revert under ASan.
   `BUGFIX #9` in source.

`NULL` converted to `nullptr` at its one live call site. Added
class-level doc comment to the header (including a note on the global
state's shared-lifecycle contract) and function-level doc comments in
the source. `doctype/usmarc.cxx` added to `TEST_ENGINE_DOCTYPE_SRCS`.
Left as documented, not changed: `compareReg()`'s `if (s1 == nullptr ||
s2 == nullptr) { }` empty block (its own `FIXME` comment already
acknowledges this is unresolved, and neither of its two call sites ever
passes null in practice); `usefulMarcField()`'s unused parameter
(matches its own doc comment's stated "index everything for now"
design, an intentional stub); `compareReg()`'s second loop reading
`s1[i]` for `i` up to `strlen(s2)` without confirming `s1` is at least
that long (both real call sites always pass same-length fixed-size
strings in practice, so not reachable as a live bug, matching this
project's "don't add validation for scenarios that can't happen").

`tests/doctype/test_usmarc.cxx` covers, using a minimal hand-built,
byte-verified MARC record (offsets checked with a standalone script
before being hard-coded, per this session's established discipline for
binary-format test fixtures): `ParseRecords()` splitting a batch file at
each record's own length prefix; `ParseFields()` indexing both the raw
tag and friendly name with the exact right byte span (`BUGFIX #7`);
`ParseWords()` returning the sentinel instead of overflowing
(`BUGFIX #1`, exercising `BUGFIX #2` along the way); no leak when
`ParseFields()` runs twice without an intervening `ParseWords()`
(`BUGFIX #3`); no crash/leak when the file can't be opened (`BUGFIX #3`'s
crash half, `BUGFIX #5`); and no heap-buffer-overflow on a field
missing its end-of-field delimiter (`BUGFIX #6`/`BUGFIX #9` together —
this is the exact test that caught `BUGFIX #9` live, after `BUGFIX #6`
alone still weren't enough to make it pass under ASan). Every test that
calls `ParseFields()` without going on to call the real `ParseWords()`
as part of its own assertions explicitly calls it anyway purely for
cleanup, since `RecBuffer`/`marcDir` being globals means one test's
unclean state could otherwise leak into whichever other test Catch2's
randomized run order happens to execute next.

## doctype/uspat.cxx

**Scope note, read first:** at 5039 lines, this is by far the largest
file processed this whole effort (typical files this session have been
100–600 lines). A full function-by-function audit at the same depth as
every other file was not practical in one pass, so the audit was
deliberately scoped to `USPAT::ParseRecords()`/`ParseFields()`/
`Present()` and the `GB` column-tagged field parser (`GB::GetFieldName()`
via its public `BuildDft()` entry point) — the actual parse/index
pipeline. The remaining ~4500 lines are ~40 `pato_*` presentation-
formatting helper functions (Text/HTML/TextHtml element and
element-set renderers) that were left unaudited beyond confirming they
still compile with no new warnings from the edits below. This is stated
here explicitly rather than silently claiming full-file coverage.

`USPAT : public DOCTYPE` reads "Greenbook-style" US Patent text.
`ParseRecords()` forwards straight to `DOCTYPE::ParseRecords()`
unchanged, so — like `doctype/soif.cxx` — each input file is treated as
a single whole-file record, not split into multiple records; the actual
per-record parsing happens once in `ParseFields()`.

1. **`ParseFields()`'s `RecEnd = ftell(fp) - 1` off-by-one/underflow** —
   the same bug pattern already fixed (and ASan-confirmed) in
   `doctype/colondoc.cxx` `BUGFIX #1`, `doctype/marcdump.cxx`
   `BUGFIX #2`, `doctype/memodoc.cxx` `BUGFIX #1`, and
   `doctype/referbib.cxx` `BUGFIX #2` — this makes six files with this
   exact bug. `RecEnd` is `GPTYPE` (unsigned `UINT4`), and this fallback
   path only runs when the caller passes `RecordEnd == 0` (the
   whole-file sentinel), so for a genuinely empty file `ftell(fp)`
   returns `0` and the old `- 1` underflowed to `UINT_MAX`, which would
   make the following `new CHR[RecLength + 1]` attempt a huge
   allocation. Fixed by dropping the `- 1`; see `BUGFIX #1` in source.

2. **`Present()` leaks `PreferredRecordSyntax`** — `new STRING` is
   allocated near the top of the function as a workaround comment notes
   ("these next two lines will go away when `DOCTYPE::Present` is
   modified to support Preferred Record Syntax") and never freed on any
   of the function's exit paths, unlike every other heap allocation in
   the same function (`Buffer`, `ThePatent`, `pcPRS`, `RecBuffer`, all
   correctly freed at the end). Fixed by freeing it alongside the
   others; see `BUGFIX #2` in source.

3. **`GB::GetFieldName()`'s unbounded 4-byte tag read** (most severe
   finding in this file) — `GetFieldName()` reads a 4-character column
   tag via `memcpy(tmp, c_record+Start, 4)` with no bounds check on
   `Start`, and the loop that scans past blank/continuation lines to
   find the next field's start could exit having run `ColumnStartPtr`
   off the end of `c_record` (`>= c_length`) while still reporting
   `Done = 0` — a genuine bug, not a hypothetical one, since a record
   whose last field's line simply ends in a newline (an ordinary,
   common case) drives exactly this path. The caller, `GB::BuildDft()`,
   would then invoke `GetFieldName()` again with an out-of-bounds
   `Start`, straight into the unchecked `memcpy()`. Confirmed as a real
   **heap-buffer-overflow** via a before/after test-revert under ASan
   (`GB::GetFieldName() doctype/uspat.cxx:4959`, triggered by exactly
   this "record ends in a newline" scenario — see
   `tests/doctype/test_uspat.cxx`). Also reachable, degenerately, for a
   genuinely empty record (`Start=0, c_length=0`) on the very first
   call. Fixed with two guards: an upfront `if (Start + 4 > c_length)`
   check before the `memcpy()`, and a post-loop check that sets `Done =
   1` (instead of leaving it `0`) when the blank-line scan ran off the
   end of the buffer. See `BUGFIX #3` in source (both halves).

   **Known limitation, documented but not fixed:** tracing through
   `GB::BuildDft()`'s driving loop (`Done = GetFieldName(...); while
   (!Done) { process; Start = NextFieldStart; Done = GetFieldName(...);
   }`) shows that a record's *last* field is never inserted into the
   DFT — `Done` is only ever set to `1` on the same call that identifies
   that last field's name and boundaries, and the `while (!Done)` loop
   exits before processing it. This is a pre-existing property of the
   algorithm (confirmed present in the original, unfixed code too, via
   its other `Done=1` branch — the "no trailing newline" case), not
   something introduced by `BUGFIX #3` above; `BUGFIX #3` just makes the
   "record ends in a newline" case share the same (already-existing)
   behavior instead of crashing. Left unfixed because it's a potential
   functional/indexing-correctness change (it could alter DFT contents
   for every existing `USPAT`-indexed collection), not a memory-safety
   issue, and changing it deserves a deliberate human decision rather
   than a silent fix bundled into an unattended cleanup pass.

4. **`pato_ReadPatent()`'s unbounded `memccpy()`** — each line of the
   input `Buffer` is copied into a fixed 256-byte scratch buffer via
   `memccpy(Si, Buffer+Pos, '\n', 200)`, unconditionally reading up to
   200 bytes starting at `Buffer+Pos` regardless of how many bytes
   actually remain in `Buffer` — reachable whenever the final line's
   tail is shorter than 200 bytes and has no trailing `\n` for
   `memccpy()` to stop at first, letting the read run past the end of
   `Buffer`. Fixed by bounding the copy length to
   `min(200, PatentSize - Pos)`. See `BUGFIX #4` in source.

Modernization (`NULL` → `nullptr`, `sprintf` → `snprintf`) was applied
only within the functions actually touched above (`ParseFields()`,
`Present()`, `pato_ReadPatent()`), not blanket-applied across the
file's other ~180 `NULL` occurrences and ~45 `sprintf` calls living in
the unaudited `pato_*` helpers, per this project's "modernize only
while touching a function" rule. Confirmed via a full-file recompile
that the file's 282 pre-existing `-Wall -Wextra` warnings (almost all
`-Wwrite-strings`, from C-string-literal-to-`char*` conversions in the
untouched `pato_*` functions) are unchanged before and after these
edits, with none falling on the edited lines.

`tests/doctype/test_uspat.cxx` covers: `ParseFields()` not attempting a
huge allocation on an empty file (`BUGFIX #1`, `REQUIRE_NOTHROW`);
`GB::BuildDft()` not reading out of bounds for a zero-length record
(`BUGFIX #3`'s upfront guard); and `GB::BuildDft()` not reading out of
bounds when a two-field record's last line ends exactly at EOF
(`BUGFIX #3`'s post-loop guard — this is the exact scenario
before/after-confirmed as a real heap-buffer-overflow under ASan, per
above). `BUGFIX #2` and `BUGFIX #4` are **not** covered by an automated
test: `pato_ReadPatent()` is a private method and the header-signature
freeze rules out adding a test-only friend declaration to expose it,
and `Present()`'s only path into that code, `RESULT::
GetHighlightedRecord()`, is a no-op in this build (`DO_HIGHLIGHTING` is
never defined anywhere in this tree — see `src/result.cxx`), so
`StringBuffer` never carries real record content through `Present()`
here regardless of what's on disk. Both fixes were verified by
inspection only; this is stated explicitly in the test file's own
header comment as well.

## src/Debug.h

**Scope note, read first:** `src/Debug.h` and its implementation
`src/Debug.cc` are not included by any other file in `src/`, `doctype/`,
or `Isearch-cgi/` (confirmed by grepping the whole tree for
`#include.*"Debug.h"` — the only hit is `Debug.cc` itself), and
`DEBUGCLASS` is never defined anywhere in the top-level `Makefile`.
`Debug.h`'s real `class Debug` body is therefore compiled out
everywhere; every translation unit that ever did include this header
would get the no-op stub instead. `Debug.cc` isn't referenced by
`TEST_ENGINE_SRCS` or any production build rule either, so it isn't
compiled at all in the current build. Both files are, in short,
orphaned dead code. Processed anyway (it carries a
`docs/PROCESSING_STATUS.md` row), and the bugs below are real and
independently confirmed under ASan — they'd matter the moment
`DEBUGCLASS` is ever defined or the header is ever wired back in.

Four bugs found and fixed, all in `src/Debug.cc`'s `DEBUGCLASS`-guarded
implementation:

1. **`Debug::out()`'s unbounded `vsprintf()`** — formats a caller-
   supplied `fmt`/varargs message into a fixed 2048-byte stack buffer
   with no length bound. Any caller whose formatted message reaches
   2048+ bytes overflows it. Confirmed as a real **stack-buffer-
   overflow** via a before/after test-revert under ASan (`vsprintf`
   inside `Debug::out() src/Debug.cc:43`). Fixed with `vsnprintf()`
   bounded to `sizeof(buffer)`. See `BUGFIX #1` in source.

2. **`Debug::_init()`'s unbounded `_active_categories[i] = t`** (most
   severe finding in this file) — `_active_categories` is a fixed
   1024-entry array; the `while (t) { ... }` loop parsing the
   `;`-separated `DEBUG_OPT` environment variable had no bound on `i`,
   so a `DEBUG_OPT` with 1024 or more categories overflowed it.
   Confirmed as a real **global-buffer-overflow** via a before/after
   test-revert under ASan (`Debug::_init() src/Debug.cc:124`, landing
   exactly at the start of the adjacent `initialized` static variable's
   redzone — a concrete illustration of the overflow's reach). Fixed by
   bounding the loop to `i < 1023`, leaving room for the terminating
   `0`. See `BUGFIX #2` in source.

3. **`Debug::_init()`'s `initialized` never set on the main path** —
   the function's whole purpose for the `static bool initialized` guard
   is "parse `DEBUG_OPT` once"; the two early-return branches
   (`!c`/`!_environment_setting`) correctly set `initialized = true`,
   but the normal, successful-parse path fell through to `return;`
   without ever setting it. This made the guard dead: every single
   `Debug` object constructed after the first would re-run `_init()`,
   re-`strdup()`-ing `DEBUG_OPT` into `_environment_setting` and
   silently leaking the previous allocation each time, and
   re-tokenizing the string. Fixed by setting `initialized = true`
   before the final `return`. See `BUGFIX #3` in source.

4. **`Debug.cc` doesn't compile if `DEBUGCLASS` is ever defined** —
   `cerr`/`endl` are used unqualified throughout, with no
   `using namespace std;` and no `#include` in this file that would
   provide one; `<iostream>` alone only declares `std::cerr`/`std::endl`,
   not the unqualified names. Since `using namespace std;` doesn't cross
   translation-unit boundaries, this file cannot rely on some other
   already-compiled file's declaration the way many `.hxx`/`.cxx` files
   in this tree do (see the existing comments in `src/defs.hxx`,
   `src/fc.hxx`, `src/fct.hxx` documenting that ambient dependency) —
   this is a standalone `.cc` file with its own `#include` list, and
   that list doesn't include anything that pulls `using namespace std;`
   in. Confirmed directly: compiling this file with `-DDEBUGCLASS`
   before this fix fails with `'cerr' was not declared in this scope`.
   Fixed by qualifying both call sites with `std::`. See `BUGFIX #4` in
   source.

`tests/src/test_debug.cxx` covers all four bugs in one `TEST_CASE`
(`Debug::_init()`'s "only run once" static guard means every `Debug`
object constructed within one process shares the same parsed
`_active_categories` state, so the test controls ordering by doing
everything — building an oversized `DEBUG_OPT`, `setenv()`, and both
`Debug` constructions — before any assertion, rather than relying on
Catch2's per-`TEST_CASE` isolation). Since `Debug.cc` isn't part of
`TEST_ENGINE_SRCS` (adding it there would compile to an empty
translation unit, as `DEBUGCLASS` still wouldn't be defined for the
rest of the suite), the test file instead privately compiles its own
copy of the real implementation via `#define DEBUGCLASS 1` followed by
`#include "Debug.cc"` directly — a self-contained translation unit that
doesn't touch the Makefile or affect any other test's build. Both
`BUGFIX #1` and `BUGFIX #2` were confirmed as real crashes via a
before/after test-revert under ASan before being fixed (see above);
`BUGFIX #3` has no dedicated behavioral assertion (it's a
correctness/leak fix with no observable output difference in a
single-process test), and `BUGFIX #4` is inherently confirmed by the
test file compiling at all.

## src/Firewall.h

Same situation as [`src/Debug.h`](#srcdebugh) (see that entry's scope
note): `src/Firewall.h`/`src/Firewall.cc` aren't included anywhere else
in `src/`, `doctype/`, or `Isearch-cgi/`, `FIREWALLS` is never defined
by the top-level `Makefile`, and `Firewall.cc` isn't referenced by
`TEST_ENGINE_SRCS` or any production rule — both files are dead code in
the current build. Processed anyway (it carries a
`docs/PROCESSING_STATUS.md` row); the bugs below are real and
independently confirmed under ASan/by direct compilation.

Three bugs found and fixed, all in `src/Firewall.cc`'s
`FIREWALLS`-guarded implementation — a near-exact structural twin of
`src/Debug.cc`'s `Debug::out()`, right down to the same fixed 2048-byte
message buffer:

1. **`Firewall::hit()`/`Firewall::assert()`'s unbounded `vsprintf()`**
   (most severe finding in this file) — both functions format a
   caller-supplied `fmt`/varargs message into a fixed 2048-byte stack
   buffer with `vsprintf()`, no bound on the formatted length, at two
   separate call sites (`hit()` and `assert()` each have their own).
   The same bug as `src/Debug.cc`'s `BUGFIX #1`. Confirmed as a real
   **stack-buffer-overflow** via a before/after test-revert under ASan.
   Fixed with `vsnprintf()` bounded to `sizeof(buffer)` at both sites.
   See `BUGFIX #1` in source.

2. **`Firewall.cc` doesn't compile if `FIREWALLS` is ever defined,
   reason 1** — `cerr`/`endl` used unqualified in `trap()`, `hit()`, and
   `assert()`, with no `using namespace std;` and no `#include` in this
   file providing one. Identical issue and fix to `src/Debug.cc`'s
   `BUGFIX #4`. Fixed by qualifying all three call sites with `std::`.
   See `BUGFIX #2` in source.

3. **`Firewall.cc` doesn't compile if `FIREWALLS` is ever defined,
   reason 2** — `strcpy()` (in `hit()`/`assert()`) and `strcmp()` (in
   `_init()`) are used with no `#include <string.h>`/`<cstring>`
   anywhere in this file, unlike `src/Debug.cc` which does include it.
   Confirmed directly: compiling with `-DFIREWALLS` before this fix
   fails with `'strcpy' was not declared in this scope`. Fixed by
   adding the missing `#include`. See `BUGFIX #3` in source.

Unlike `src/Debug.cc`, `Firewall::_init()`'s own `static bool init`
one-time guard is correct as originally written (`init` starts `true`
and is explicitly set `false` inside the guarded block, the inverse
polarity of `Debug`'s but implemented consistently) — no equivalent of
`Debug.cc`'s `BUGFIX #3` applies here.

`tests/src/test_firewall.cxx` covers `BUGFIX #1` at both call sites in
one `TEST_CASE`, using the same private-compilation technique as
`tests/src/test_debug.cxx` (`#define FIREWALLS 1` then
`#include "Firewall.cc"` directly, since adding this file to
`TEST_ENGINE_SRCS` would compile to nothing without `FIREWALLS`
defined for the whole suite). `FIREWALLS` is deliberately left unset in
the test (`_init()`'s defaults keep `_active=true`, `_fatal=false`),
since `_fatal=true` would make `trap()` call `exit(1)` and kill the
test binary. One subtlety the test file documents inline: `Firewall::
assert` collides with the `<cassert>` function-like `assert` macro that
`catch_amalgamated.hpp` pulls in transitively, which would otherwise
rewrite `Firewall::assert(...)` before the compiler ever sees the
`Firewall::` qualifier — worked around with `#undef assert` after
including `catch_amalgamated.hpp`. `BUGFIX #1` was confirmed as a real
crash via a before/after test-revert under ASan before being fixed (see
above); `BUGFIX #2` and `BUGFIX #3` are inherently confirmed by the
test file compiling at all.

## src/Iget.cxx

**Scope note, read first:** this is the first `main()`-only CLI entry
point processed this session (a command-line record-retrieval tool, one
function, no header). It structurally cannot be linked into the shared
Catch2 test binary — its `main(int, char**)` would collide with
`catch_amalgamated.cpp`'s own `main()` (a duplicate-definition link
error) — and there is no other function to unit test in isolation.
Building it as a real standalone executable for black-box/subprocess
testing would require the legacy per-directory `src/Makefile` build (or
`make smoke-test`, itself explicitly manual/on-demand and not run by
`/process-next`/`/process-5`/`/process-10`), which is a materially
bigger undertaking than any other file processed this batch and was
judged out of scope for one file's turn in an otherwise `make
tests`/`make tests-asan`-driven pipeline. Verified instead via a full
manual trace-through of both bugs below (shown in detail) plus a
standalone `g++ -c` compile-clean confirmation (`-Isrc -Idoctype`,
`-Wall -Wextra`: no warnings on any of this file's own lines, matching
its state before these edits).

Two bugs found and fixed:

1. **Null-pointer dereference on a flag given without its required
   argument** (most severe finding in this file, and trivially
   reachable from ordinary command-line typos) — all four flag branches
   in the argument-parsing loop (`-d`, `-id`, `-p`, `-f`) follow the
   same pattern: `if (++x >= argc) { Error=GDT_TRUE; error_message.Cat(
   ...); }` followed unconditionally by `<Field> = argv[x];`, with no
   `else`. When the check fires (the flag was the last argument, e.g.
   running `Iget -d` with nothing after it), `x == argc` at that point,
   and `argv[argc]` is guaranteed `nullptr` by the C++ standard — which
   `STRING::operator=(const CHR*)` (`src/string.cxx:374`) dereferences
   unconditionally via `strlen(CString)`, a **null-pointer dereference**
   (confirmed by inspection of `string.cxx`, not by ASan, since this
   file can't be linked into the sanitizer-run test binary — see the
   scope note above). Fixed by moving the `argv[x]` read into an `else`
   branch at all four sites, so the `Error` flag set by the missing-
   argument check is left to reach the existing `if (Error) {
   error_message.Print(stderr); EXIT_ERROR; }` check further down
   instead of crashing first. See `BUGFIX #1` in source.

2. **Dead "unrecognized arguments" check (wrong comparison direction)**
   — `x = LastUsed + 1; if (x > argc) { ... "Unrecognized arguments" ...
   }`. `LastUsed` is only ever assigned a valid `argv` index (always `<
   argc`, doubly so after `BUGFIX #1` above), so `x = LastUsed + 1` can
   never exceed `argc` — `x > argc` is permanently false, and this
   validation has never fired. Traced through both a correct-usage case
   (`Iget -d db -id 123`, ends with `x == argc`, no error either way)
   and a broken-usage case (`Iget -d db extra_garbage`, ends with `x ==
   3, argc == 4`: the existing `x > argc` check misses it, while `x <
   argc` correctly flags it) to confirm the fix doesn't regress valid
   usage while it does catch the case the error message is clearly
   meant to catch. Fixed by changing `x > argc` to `x < argc`. See
   `BUGFIX #2` in source.

No test file was written for the reasons in the scope note above; both
fixes are documented in detail here and at their `BUGFIX #n` comment
sites in source instead. `/smoke-test` (manual, on-demand — see
CLAUDE.md) would be one way to get broader integration coverage across
`Iget` and the other `main()`-only CLI tools still in the queue
(`Iindex.cxx`, `Isearch.cxx`, `Iutil.cxx`) if that's ever wanted.

## src/Iindex.cxx

**Scope note, read first:** same `main()`-only structural limitation as
[`src/Iget.cxx`](#srcigetcxx) (can't be linked into the shared Catch2
test binary — duplicate `main()`), compounded here by size: at 913
lines this is the largest non-`doctype/` file processed this session.
`AddFile()` — the one function with real logic worth testing in
isolation — only sinks into `IDB::AddRecord()`, which requires a fully
constructed `IDB` backed by real on-disk index files
(`IDB::Initialize()` in `src/idb.cxx` explicitly "Opens (creating on
first use) the on-disk database..."), a disproportionate fixture to
build for one file's turn in a per-file cleanup pass. The deep audit
was scoped to `AddFile()` (the core per-file record-splitting logic,
and the most complex/critical-for-correctness code in the file) and the
argument-parsing loop (confirmed *not* to share `src/Iget.cxx`'s
`BUGFIX #1` pattern — every `++x >= argc` check here correctly
`RETURN_ZERO`/`RETURN_ERROR`s before reading `argv[x]`); a lighter pass
covered the file-listing/GILS-metadata-generation tail of `main()`.
Verified via a standalone `g++ -c` compile-clean confirmation (no
warnings on this file's own lines, matching its state before these
edits) and manual trace-through, the same methodology as `Iget.cxx`.

Two bugs found and fixed, both in `AddFile()`'s `mmap()`-backed
separator-splitting path (`#ifndef NO_MMAP`, the branch actually
compiled on this Linux target):

1. **Unchecked `mmap()` return value** (most severe finding in this
   file) — `Buffer = (CHR*) mmap(...)` is used immediately afterward
   (`EndOfBuffer = Buffer + FileSize`, then a scanning loop
   dereferencing `*Position` starting at `Buffer`) with no check for
   `MAP_FAILED` (`(void*)-1`, *not* `nullptr`, per the `mmap(2)`
   contract). A failed mmap therefore doesn't null-deref-crash in the
   usual sense — it dereferences whatever `(CHR*)-1` plus small offsets
   resolves to, an almost-certain **segfault**, confirmed by inspection
   of the `mmap(2)` contract rather than ASan (this file can't be linked
   into the sanitizer-run test binary — see the scope note above).
   Realistically reachable, not just theoretical: `mmap()` rejects a
   zero-length mapping with `EINVAL`, so indexing a genuinely empty file
   with `-s <separator>` set hits this on every run. Fixed by checking
   `Buffer == (CHR*)MAP_FAILED` immediately after the call, logging via
   `perror()` and returning (skipping just that file) instead of
   proceeding. See `BUGFIX #1` in source.

2. **Missing `munmap()`** — `Buffer` is mapped once per file (when a
   separator is set) and never unmapped on any path through this
   function — a straightforward resource leak, but a meaningful one
   here specifically: `Iindex`'s entire purpose is indexing potentially
   thousands of files in a single process run, so this leaks one file
   mapping per input file for the whole run's lifetime. Fixed by adding
   `munmap(Buffer, FileSize);` after the mapping's last use, alongside
   the existing `delete [] Sep;`/`fclose(Fp);` cleanup. See `BUGFIX #2`
   in source.

No test file was written for the reasons in the scope note above; both
fixes are documented in detail here and at their `BUGFIX #n` comment
sites in source instead, matching `src/Iget.cxx`'s precedent.

## src/Isearch.cxx

**Scope note, read first:** same `main()`-only structural limitation as
[`src/Iget.cxx`](#srcigetcxx)/[`src/Iindex.cxx`](#srciindexcxx) (can't
be linked into the shared Catch2 test binary — duplicate `main()`). At
754 lines, the deep audit prioritized the search/result-set pipeline
(`VIDB::Search()`/`AndSearch()` and how their return value is used) and
the argument-parsing loop over the JSON-output and interactive-browsing
tail of `main()`. Verified via a standalone `g++ -c` compile-clean
confirmation (comparing warning output against the pre-edit committed
version to confirm no new warnings — see below) and manual
trace-through, including reading `VIDB::Search()`/`AndSearch()`'s own
implementation in `src/vidb.cxx` to confirm the null-return path below
is real, not hypothetical.

One bug found and fixed:

1. **Unchecked `nullptr` from `VIDB::Search()`/`AndSearch()`** (most
   severe finding in this file) — `pirset = pdb->Search(squery);` (or
   `AndSearch()` under `-and`) is immediately followed by `n =
   pirset->GetTotalEntries();` with no null check. Both
   `VIDB::Search()` and `VIDB::AndSearch()` (`src/vidb.cxx`) explicitly
   initialize `RsetPtr = nullptr;` and return it early with the comment
   `// Bail out if no databases` when `c_dbcount <= 0` — a virtual
   database (`VIDB`, a union of sub-databases configured via a registry
   file) with zero usable sub-databases. `DBExists(DBName)` (checked
   earlier in `main()`) only confirms the top-level `VIDB` marker exists,
   not that it lists at least one still-openable sub-database, so a
   misconfigured or partially-broken virtual database reaches this
   **null-pointer dereference** in practice, not just in theory. Fixed
   by checking `!pirset` immediately after the `Search()`/`AndSearch()`
   call and exiting via the same `fprintf(stderr, ...); delete []
   WordList; delete pdb; RETURN_ERROR;` pattern already used by this
   function's other fatal-error paths (e.g. the `IsDbCompatible()`
   check just above). See `BUGFIX #1` in source.

Two pre-existing characteristics observed but deliberately left
unfixed, both confirmed by inspection and by diffing this file's
compile warnings against its pre-edit committed version (identical
before and after these edits):

- **`-RECT{North South West East}` is documented but unimplemented** —
  the usage/help text (printed when `argc < 2`) advertises this flag
  for geographic-rectangle search, and a variable named
  `SpatialRectFlag` is declared for it, but neither of the two
  argument-parsing loops in `main()` has any code path that recognizes
  a `-RECT{...}`-shaped token — it isn't in the first loop's
  `Flag.Equals(...)` chain, and it isn't in the second (word-list-
  building) loop's flag classification lists either, so a user who
  actually passes `-RECT{...}` as documented gets it silently treated
  as an ordinary search term instead of a spatial filter.
  `SpatialRectFlag` itself is written to (`=0`, in its own declaration)
  but never read anywhere else in the file, and never set to a nonzero
  value by anything (confirmed via `-Wunused-variable`, unchanged from
  the pre-edit file). Left unfixed: implementing real geo-rectangle
  query parsing would mean guessing at intended `SQUERY`/geosearch
  integration semantics that no longer exist anywhere in this file,
  which is feature work, not a bug fix, and squarely the kind of
  judgment call this project's own principles reserve for a human
  decision rather than an autopilot guess.
- **`LastUsed` is fully vestigial** — assigned in 18 separate places
  across the first argument-parsing loop (mirroring the same bookkeeping
  pattern `src/Iget.cxx` and `src/Iindex.cxx` use to find where
  positional file/word arguments begin, `x = LastUsed + 1`), but this
  file never reads `LastUsed` anywhere (confirmed via
  `-Wunused-but-set-variable`, also unchanged from the pre-edit file).
  Positional word arguments are instead found by an entirely separate,
  self-sufficient two-pass scan of `argv[]` further down in `main()`
  (building `NumWords`/`WordList` by independently re-classifying every
  `-flag`), whose own flag lists were checked and confirmed to exactly
  match the first loop's 18 recognized flags — so `LastUsed`'s dead
  bookkeeping doesn't indicate a functional bug, just an incomplete
  removal from an earlier refactor. Left as-is per this project's "don't
  restyle code you're not otherwise touching" principle — removing 18
  scattered assignment lines isn't tied to fixing a real defect here.

No test file was written for the reasons in the scope note above;
`BUGFIX #1` is documented in detail here and at its comment site in
source instead, matching `src/Iget.cxx`/`src/Iindex.cxx`'s precedent.

## src/nlist.hxx

Reprocessed via `/reprocess-blocked` (originally blocked at GENERAL
step 4 on 2026-08-07; see the resolved `docs/AUTOPILOT_LOG.md` entry for
the original finding). `NUMERICLIST` owns a heap-allocated `table` array
(`new NUMERICFLD[50*Ncoords]` in both constructors, freed in
`~NUMERICLIST()`) used for numeric-attribute range search.

1. **No copy semantics — confirmed heap-use-after-free on copy** — no
   user-declared copy constructor or `operator=` existed, so the
   compiler-generated ones shallow-copied `table`; confirmed via a
   standalone repro (`NUMERICLIST b = a;`, let `b` then `a` go out of
   scope) triggering a real heap-use-after-free in `~NUMERICLIST()`
   under ASan. No live call site copies a `NUMERICLIST` (only
   default-construction). **Decision (human, via `/reprocess-blocked`):
   make it non-copyable** rather than implement deep-copy semantics
   nothing needs — `NUMERICLIST(const NUMERICLIST&) = delete;` and
   `NUMERICLIST& operator=(const NUMERICLIST&) = delete;` added to the
   header. See `BUGFIX #1` in source.
2. **`Attribute`/`Relation` left indeterminate by both constructors** —
   every other member was explicitly set; matches the same
   "indeterminate primitive member" category already fixed in
   `RESULT`'s and `NUMERICFLD`'s constructors. Fixed by zero-
   initializing both in both constructors. See `BUGFIX #2` in source.

`src/intlist.hxx` (`INTERVALLIST`, derives from `NUMERICLIST`) has the
identical shape and was reprocessed in this same `/reprocess-blocked`
run; see its own entry below.

`tests/src/test_nlist.cxx` covers: `NUMERICLIST` is non-copyable
(`std::is_copy_constructible`/`is_copy_assignable`, `STATIC_REQUIRE_FALSE`
— `BUGFIX #1`); both constructors leave `GetAttribute()`/`GetRelation()`
at a deterministic `0` (`BUGFIX #2`), alongside a basic sanity check of
`GetCoords()`/`GetCount()`. `make tests`/`make tests-asan` pass clean
(676 test cases, 2292 assertions).

## src/nlist.cxx

`BUGFIX #1-2` above were already applied to this file during
`nlist.hxx`'s `/reprocess-blocked` turn (this file already carried
`.hxx`'s own `ISEARCH2-CLEANUP` marker, dated the same day — its
`docs/PROCESSING_STATUS.md` row just hadn't been synced to `done` yet,
same shape as `src/memcntl.cxx`'s stale row). This turn's own full
re-read found two pre-existing `-Wall -Wextra` warnings (not introduced
by the earlier turn) and nothing else:

3. **`DiskFind()`'s `Hold` local was declared but never read or
   written** — genuinely dead in both the `DOUBLE`- and `INT4`-keyed
   overloads, not a latent bug (its own comment already says "this is
   just a dummy — we don't use it"). Removed. See `BUGFIX #3` in
   source.
4. **`MemFind()`'s three parameters are unused in both overloads** —
   by design: `MemFind()` always returns `NO_MATCH` unconditionally
   (the in-memory search path documented as "never implemented" was
   never written), and is never called from anywhere in this class —
   `Find()` always goes straight to `DiskFind()`. Parameters kept to
   match the header's declared signature. Silenced with `(void)Key;
   (void)Relation; (void)Index;`, the same technique already used for
   `src/merge.cxx`'s `BUGFIX #4`. See `BUGFIX #4` in source.

Checked and found not a live bug, matching the precedent already set
for `src/intlist.cxx`'s `SortGPCmp` above: `SortCmpGP()` subtracts two
`NUMERICFLD::GetGlobalStart()` values (`INT4`, signed) directly into a
`DOUBLE` — signed subtraction is only theoretically UB on extreme
overflow, unlike the confirmed unsigned-narrowing bug already fixed in
`src/fct.cxx`'s `FctFcCompare` (which subtracted an *unsigned* `GPTYPE`).
No new test needed: both fixes are warning-only, no behavior changed;
`make tests`/`make tests-asan` pass clean (729 test cases, 2823
assertions, unchanged from before this turn).

## src/intlist.hxx

Reprocessed via `/reprocess-blocked` (originally blocked at GENERAL
step 4 on 2026-08-07; see the resolved `docs/AUTOPILOT_LOG.md` entry for
the original finding). `INTERVALLIST` (derives from `NUMERICLIST`,
above) owns its own separate heap-allocated `table` array (`new
INTERVALFLD[50*Ncoords]` in both constructors, freed in
`~INTERVALLIST()`).

1. **No copy semantics — confirmed heap-use-after-free on copy** — same
   shape as `NUMERICLIST`: no user-declared copy constructor or
   `operator=`, so the compiler-generated ones shallow-copied
   `INTERVALLIST`'s own `table` (and would have hit `NUMERICLIST`'s
   identical bug one level up too, before that base class's own fix).
   Confirmed via a standalone repro (`INTERVALLIST b = a;`, let `b` then
   `a` go out of scope) triggering a real heap-use-after-free in
   `~INTERVALLIST()` under ASan. No live call site copies an
   `INTERVALLIST`. **Decision (human, via `/reprocess-blocked`): make it
   non-copyable**, consistent with `NUMERICLIST`'s own choice above —
   `INTERVALLIST(const INTERVALLIST&) = delete;` and `INTERVALLIST&
   operator=(const INTERVALLIST&) = delete;` added to the header (note:
   `NUMERICLIST`'s own deleted copy constructor already makes
   `INTERVALLIST`'s compiler-generated one implicitly deleted too: the
   explicit declaration here is for clarity, not strictly required).
   See `BUGFIX #1` in source.
2. **`INTERVALLIST`'s own (shadowed) `Attribute`/`Relation` left
   indeterminate by both constructors** — found while fixing `BUGFIX
   #1`, the same "indeterminate primitive member" bug already fixed for
   the base class's own copies in `src/nlist.cxx`'s `BUGFIX #2`, but
   distinct here: `INTERVALLIST` redeclares its own `Attribute`/
   `Relation` (see the shadowing note below), separate storage from
   `NUMERICLIST`'s. Fixed by zero-initializing both in both
   constructors. Not independently testable via the public API — see
   the test note below. See `BUGFIX #2` in source.

**Known design issue, documented but not fixed** (per the original
`docs/AUTOPILOT_LOG.md` finding, still applicable): `INTERVALLIST`
redeclares its own private copies of nearly every member `NUMERICLIST`
already has (`Count`, `Attribute`, `Pointer`, `MaxEntries`,
`StartIndex`, `EndIndex`, `Relation`, `FileName`, `Ncoords`, plus its
own differently-typed `table`), so every `INTERVALLIST` instance
allocates and initializes two separate, independent table arrays — the
base's own 100-entry `NUMERICFLD[]`, entirely unused after construction,
plus the derived class's real `INTERVALFLD[]` — wasting an allocation
and ~1.2KB per instance. Fixing this means either making
`NUMERICLIST`'s members `protected` or dropping the public inheritance
entirely, both a human design call rather than a mechanical fix, and
out of scope for this pass.

`tests/src/test_intlist.cxx` covers: `INTERVALLIST` is non-copyable
(`BUGFIX #1`, same `STATIC_REQUIRE_FALSE` technique as `nlist.hxx`'s
test) and basic construction/destruction. `BUGFIX #2` has no dedicated
assertion: `INTERVALLIST` declares no accessor for its own shadowed
`Attribute`/`Relation` (`GetAttribute()`/`GetRelation()` are inherited
from `NUMERICLIST` and read the *base* class's already-fixed copies,
not `INTERVALLIST`'s own), so it was verified by inspection only,
documented explicitly in the test file's own comments. `make
tests`/`make tests-asan` pass clean (678 test cases, 2297 assertions).

## src/intlist.cxx

`BUGFIX #1`/`#2` above were already applied to this file during
`intlist.hxx`'s `/reprocess-blocked` turn; this file's own pass found
one more, isolated to `DiskFind(STRING, DOUBLE, ...)`'s `START_BLOCK`
case.

1. **Three unconditional `cerr` diagnostics, the only ones in this file
   not gated behind `#ifdef DEBUG`** — every other diagnostic message
   in this file (in this same function's `START_BLOCK` case above them,
   and throughout the `END_BLOCK`/`PTR_BLOCK` cases below) is wrapped in
   `#ifdef DEBUG`; these three, in the `TOO_HIGH`/`TOO_LOW`/`Low>=High`
   branches that report an ordinary "no match" outcome, were left
   ungated. A range/interval search coming up empty is a normal,
   expected result (a date or numeric query outside the indexed range,
   not an error condition) — left as-is, every such miss would spam
   `cerr` in a normal production build, on what's likely a common path
   given how frequently range searches partially miss. Confirmed by
   direct comparison against every sibling diagnostic in the same
   function (all correctly `#ifdef DEBUG`-gated) and the identically-
   shaped `END_BLOCK` case just below (which has no such messages at
   all, gated or not) — not independently reproduced with a live
   on-disk index (same disproportionate-fixture reasoning as
   `src/geosearch.cxx`'s `BUGFIX #2` last turn: the bug is already
   unambiguous by inspection against the function's own established
   convention). Fixed by wrapping all three in `#ifdef DEBUG`, matching
   every other diagnostic in the file. See `BUGFIX #3` in source.
   `make tests`/`make tests-asan` pass clean (713 test cases, 2379
   assertions).

## src/mergeunit.hxx

Reprocessed via `/reprocess-blocked` (originally blocked at GENERAL
step 4 on 2026-08-07; see the resolved `docs/AUTOPILOT_LOG.md` entry for
the original finding). `MERGEUNIT` is one participant in an N-way
index-merge, owning four separate heap-allocated arrays (`list`,
`Start`, `sistrings`, `Tag`).

1. **Four `delete`/`delete[]` mismatches — real, confirmed undefined
   behavior** (most severe finding in this file, and notably *not*
   dependent on the copy-semantics question below): `list`, `Tag`, and
   `Start` are each allocated with array `new` (in the constructor and
   in `SetLoadLimit()`) but three of the four deallocation sites freed
   them with scalar `delete` — `~MERGEUNIT()` (`list`/`Tag`/`Start`,
   only `sistrings` there was already correct) and `CacheLoad()`'s local
   `p`. Confirmed as a real **alloc-dealloc-mismatch** via a before/
   after test-revert under ASan (`MERGEUNIT::~MERGEUNIT()
   src/mergeunit.cxx:391`) — and this fires on the most basic
   construct-then-destroy usage of the class, e.g. the live `MERGEUNIT
   A[2];` at `src/index.cxx:2438`, not just on copying. Fixed by
   changing all four sites to `delete []`. See `BUGFIX #2` in source.
2. **No copy semantics — confirmed heap corruption on copy** — no
   user-declared copy constructor or `operator=` existed, so the
   compiler-generated ones shallow-copied all four owned arrays;
   confirmed via a standalone repro (`MERGEUNIT b = a;`, let `b` then
   `a` go out of scope) — though this repro actually surfaced `BUGFIX
   #2` above first (an ASan abort on `a`'s own destruction, before the
   copy-construction double-free could even be observed directly), since
   basic construction/destruction alone was already broken. No live call
   site copies a `MERGEUNIT` (only default/array construction).
   **Decision (human, via `/reprocess-blocked`): make it non-copyable**
   — `MERGEUNIT(const MERGEUNIT&) = delete;` and `MERGEUNIT&
   operator=(const MERGEUNIT&) = delete;` added to the header. See
   `BUGFIX #1` in source.
3. **Constructor leaves `Parent`/`fp`/`Map`/`ID`/`Gp` uninitialized** —
   `Initialize()` is a mandatory second-phase constructor that sets all
   but `Gp`, but `~MERGEUNIT()` already dereferences `Parent` (via
   `Parent->ffclose(fp)`, guarded only by `if(fp)`) if a `MERGEUNIT` is
   ever destroyed without `Initialize()` having been called first, which
   would read `fp`/`Parent` as indeterminate values. No confirmed call
   site was found that skips `Initialize()`, so this is latent, but
   matches the same "indeterminate primitive/pointer member" category
   already fixed multiple times this project (`RESULT`, `NUMERICFLD`,
   `NUMERICLIST`). Fixed by zero/null-initializing all five in the
   constructor. See `BUGFIX #3` in source.

**Found in a different file while reading `MERGEUNIT`'s real callers,
out of scope for this turn** (per the original `docs/AUTOPILOT_LOG.md`
finding, still applicable): `src/index.cxx:773` does `A = new
MERGEUNIT[sizeof(MERGEUNIT)*IndexNum];` — `new T[n]` allocates `n`
*objects* of type `T`, not `n` bytes, so this allocates `sizeof(MERGEUNIT)`
times more `MERGEUNIT` objects than intended. Belongs to `index.cxx`'s
own turn (Order 54, still pending in `docs/PROCESSING_STATUS.md`).

`tests/src/test_mergeunit.cxx` covers: `MERGEUNIT` is non-copyable
(`BUGFIX #1`); plain construct-then-destroy, both singly and via array
new/delete matching the live `src/index.cxx:2438` usage, doesn't trigger
a delete/delete[] mismatch (`BUGFIX #2`, confirmed as a real bug via a
before/after test-revert under ASan — see above) and doesn't crash on
the indeterminate-`fp` path either (`BUGFIX #3`, since `Initialize()` is
deliberately never called in these tests). `MERGEUNIT::Initialize()`
itself requires a real `PIDBOBJ`/`FILEMAP`/backing file, a
disproportionate fixture for this file's turn (matching the judgment
call already made for `src/Iindex.cxx`'s `AddFile()` —
`docs/BUG_CATALOG.md#srciindexcxx`), so `CacheLoad()`/`Load()`/`Flush()`
aren't exercised directly. `make tests`/`make tests-asan` pass clean
(681 test cases, 2301 assertions).

`src/mergeunit.cxx` (Order 152)'s own turn (a stale-tracking-row sync,
same shape as `src/memcntl.cxx`'s: this file's `Order` was assigned
after the reprocessing above already landed and marked it) found
nothing further on a full re-read. Checked and found not actionable:
`Initialize()`'s `stat(Tmp,&sb)` return value goes unchecked, so a
failed `stat()` would leave `ItemsToMerge` reading indeterminate
`sb.st_size` — but that value is only ever used in a `VERBOSE`-gated
progress `printf`, and the very next statement (`fp=Parent->ffopen(...)`
plus a null check) already catches the primary real-world failure mode
(the file not existing) before anything else depends on it. Also
checked: `Smallest()` indexes `sistrings[CachePosition]` with no bounds
check of its own, which would be a real out-of-bounds read if ever
called before `SetLoadLimit()`/`Initialize()`/`Load()` populate that
array — but both live call sites (`src/index.cxx:856` and `:2569`)
only call it inside a loop already gated by `Empty()==GDT_FALSE` on a
unit that's already been through `SetLoadLimit()`+`Initialize()`, so
this isn't reachable today.

## src/thesaurus.hxx

Reprocessed via `/reprocess-blocked` (originally blocked at GENERAL
step 4 on 2026-08-07; see the resolved `docs/AUTOPILOT_LOG.md` entry for
the original finding). Both `TH_PARENT_LIST` and `TH_ENTRY_LIST` own a
heap-allocated `table` (`new TH_PARENT[100]`/`new TH_ENTRY[100]` in each
constructor).

1. **`AddEntry()` has no bounds check against the hard-coded 100-entry
   table — confirmed, actively-reachable heap-buffer-overflow** (most
   severe finding in this file): both `TH_PARENT_LIST::AddEntry()` and
   `TH_ENTRY_LIST::AddEntry()` wrote `table[Count]` unconditionally
   against a table allocated once at construction, with no growth
   anywhere in this file, unlike every sibling table-owning class this
   project (`ATTRLIST`, `DFDT`, `MDT`, ...). `MaxEntries` was set to
   `100` in both constructors and never read again. Confirmed as a real
   **heap-buffer-overflow** via a before/after test-revert under ASan
   (`TH_PARENT::operator=() src/thesaurus.cxx:82`, called from
   `TH_PARENT_LIST::AddEntry()` on the 101st entry). **Confirmed
   actively reachable, not latent**: the index-time `THESAURUS`
   constructor parses a user-supplied synonym source file and calls
   `Parents.AddEntry()`/`Children.AddEntry()` once per parent/child term
   with no upper bound on the source file's size — any real thesaurus
   with more than 100 distinct parent terms, or more than 100 total
   child-term entries, corrupts the heap. Fixed by growing `table` when
   `Count == MaxEntries`, mirroring `ATTRLIST::Resize()`'s body. See
   `BUGFIX #1` in source (both classes).
2. **`GetEntry()`'s bounds check is off by one, in all four overloads**
   — `TH_PARENT_LIST`'s and `TH_ENTRY_LIST`'s `GetEntry(index, ...)` and
   `GetEntry(index)` all guarded with `if (index <= Count)`, not `index
   < Count`; `Count` is the 0-based index of the next *unused* slot
   (`AddEntry()`'s own `table[Count] = ...; Count++;`), so
   `GetEntry(Count, ...)` silently returned whatever indeterminate/
   never-written entry happened to be at `table[Count]` instead of
   leaving the output untouched or signaling "not found." Every live
   call site (`THESAURUS::LoadParents()`/`LoadChildren()`) only ever
   calls with `index` in `[0, Count-1]`, so this slack isn't hit today,
   but it's a real off-by-one in a public method. Fixed by changing all
   four checks to `index < Count`. See `BUGFIX #2` in source.
3. **No copy semantics — confirmed heap-use-after-free on copy** — no
   user-declared copy constructor or `operator=` existed for either
   class, so the compiler-generated ones shallow-copied `table`;
   confirmed via a standalone repro (`TH_PARENT_LIST b = a;`, let `b`
   then `a` go out of scope) triggering a real heap-use-after-free in
   `~TH_PARENT_LIST()` under ASan (`TH_ENTRY_LIST` has the identical
   shape, unverified standalone but structurally the same code). No
   confirmed copy-construction call site was found in the live tree.
   **Decision (human, via `/reprocess-blocked`): make both
   non-copyable** — `TH_PARENT_LIST(const TH_PARENT_LIST&) = delete;`/
   `operator=` and the same pair for `TH_ENTRY_LIST` added to the
   header. See `BUGFIX #3` in source.
4. **`TH_PARENT`'s and `TH_ENTRY`'s default constructors leave their
   `INT4` members indeterminate** — `TH_PARENT::TH_PARENT() {}` didn't
   initialize `GlobalStart` (`Term`, a `STRING`, self-initializes to
   empty regardless); `TH_ENTRY::TH_ENTRY() {}` didn't initialize either
   `GlobalStart` or `ParentPtr`. Same "indeterminate primitive member"
   category already fixed multiple times this project. Interacts with
   `BUGFIX #2` above: the one-past-the-end slot it could return was
   exactly one of these never-explicitly-set entries. Fixed by
   zero-initializing all three in their respective constructors. See
   `BUGFIX #4` in source.
5. **`TH_PARENT::Copy()` was declared and defined but its body was
   empty** — `void TH_PARENT::Copy(const TH_PARENT& OtherValue) { }` did
   nothing at all, unlike the adjacent (correct) `operator=` two lines
   below it, which actually copies `GlobalStart`/`Term`. Not called
   anywhere in the tree (confirmed by search), so this was dead code
   rather than an active bug — but clearly an unfinished implementation
   left behind rather than an intentional no-op, not a design ambiguity
   requiring a human decision, so completed to match `operator=`'s
   behavior rather than left as a separate open question. See `BUGFIX
   #5` in source.

**Found while reading, not fixed — a related but distinct issue from
`BUGFIX #1` above**: `TH_PARENT_LIST::LoadTable()`/`TH_ENTRY_LIST::
LoadTable()` read a `Count` value directly from a `.spx`/`.scx` index
file (`fread((CHR*)&Count, ...)`) and then loop `table[i]` for `i` up to
that freshly-read `Count`, with no check against `MaxEntries` — if a
`Count` larger than the table's actual allocated size were ever
persisted, this would overflow `table[]` too. Left unfixed: unlike
`AddEntry()`'s direct exposure to arbitrary user-supplied synonym text,
`LoadTable()` only reads index files this same class itself previously
wrote via `WriteTable()` — a materially weaker trust/reachability story
(only reachable via external tampering or corruption of the program's
own generated files, not normal usage) that wasn't part of what the
original `docs/AUTOPILOT_LOG.md` finding flagged or what this turn's
scope was decided against.

`tests/src/test_thesaurus.cxx` covers: `TH_PARENT_LIST`/`TH_ENTRY_LIST`
are non-copyable (`BUGFIX #3`); both grow past their initial 100-entry
table when adding 150 entries (`BUGFIX #1`, confirmed as a real bug via
a before/after test-revert under ASan — see above); `GetEntry()` treats
`index == Count` as out of range, for both the pointer-returning and
out-parameter overloads (`BUGFIX #2`); `TH_PARENT::Copy()` actually
copies now (`BUGFIX #5`); both classes' default constructors start their
`INT4` members at a deterministic `0` (`BUGFIX #4`). `THESAURUS` itself
wasn't exercised at the time this section was written (its constructors
need real files on disk); see `src/thesaurus.cxx` below for the
`THESAURUS`-level bugs and tests added once that file's own turn built
the on-disk test fixtures. `make tests`/`make tests-asan` pass clean
(687 test cases, 2319 assertions).

## src/thesaurus.cxx

Full re-read (942 lines) alongside the already-processed
`thesaurus.hxx` above (`BUGFIX #1-5`, all confirmed present and correct
in source). Found two new, confirmed-real bugs in the index-time
constructor and two more in `GetParent()`/`GetChildren()`.

1. **Index-time constructor dereferences a null `strtok()` result when
   `SourceFileName` doesn't exist or is empty — confirmed SIGSEGV**:
   `THESAURUS(const STRING& SourceFileName, ...)` calls
   `sBuf.ReadFile(SourceFileName)` without checking its return value.
   If the file doesn't exist (or is genuinely empty), `sBuf` stays
   empty, `b = sBuf.NewCString()` yields an empty C-string, and
   `pBuf = strtok(b,"\n")` returns `nullptr` — dereferenced
   unconditionally by `while (*pBuf == ' ') pBuf++;` at the top of the
   parsing loop, with no null check anywhere before it. Confirmed via a
   standalone repro: a nonexistent `SourceFileName` produced a real
   `SIGSEGV` (`AddressSanitizer: SEGV on unknown address 0x0`) at the
   `*pBuf` dereference. Fixed by returning early (an empty/missing
   source file degenerates to an empty thesaurus) when `pBuf` is
   `nullptr`, matching the "gracefully handle missing input" convention
   `OpenParentsFile()`/`OpenChildrenFile()`'s own callers already use
   elsewhere in this class. See `BUGFIX #6` in source.
2. **`b` (the parsed source buffer) leaked when `OpenSynonymFile("wb")`
   fails** — found while fixing `BUGFIX #6` above: `b` was only freed
   (`delete [] b;`) after the parsing `do-while` loop completed
   normally; the early-return path taken when the output `.syn` file
   can't be opened (e.g. `DbPathName` not writable or nonexistent)
   never freed it. A real, if narrow, memory leak on a plausible
   indexing-time failure mode. Fixed by freeing `b` on that early-return
   path too. See `BUGFIX #7` in source.
3. **`GetParent()` and `GetChildren()` both leak a `FILE*` on every
   successful call — confirmed via `/proc/self/fd` inspection**: both
   open the synonym file via `OpenSynonymFile("rb")` and read through
   it, but neither closes it on the success path (only `GetChildren()`'s
   `fgets()`-failure branch called `fclose()`). Confirmed via a
   standalone repro that counted this process's open file descriptors
   before/after 50 `GetParent()`+`GetChildren()` call pairs: reverting
   the fix showed the count growing by exactly 2 per iteration (100 new
   fds over 50 iterations); with the fix, the count is unchanged.
   Reachable through the live `SQUERY::ExpandQuery()` synonym-expansion
   path (`src/squery.cxx`), which calls both once per query term — a
   long-running search process doing repeated thesaurus-expanded
   queries would eventually exhaust its file descriptor limit. Fixed by
   adding the missing `fclose(fp)` to both functions' success paths.
   See `BUGFIX #8` in source (both functions).
4. **`GetChildren()` returns the last child term with an embedded
   trailing newline — confirmed via standalone repro**: `fgets(buf,
   MAX_SYN_LENGTH, fp)` reads the full line including its trailing
   `'\n'`, which was never stripped before `TheEntry.Replace("=","+");
   Children->Split('+',TheEntry);` — so whichever child term is last on
   the line comes back as e.g. `"PUPPY\n"` instead of `"PUPPY"`.
   Confirmed with a standalone repro built from a real two-line synonym
   file (`DOG=CANINE+PUPPY`); reverting the fix reproduced the exact
   corrupted last entry (`GetLength() == 6` instead of `5`). Reachable
   through the same live `SQUERY::ExpandQuery()` path as `BUGFIX #8` —
   `ExpandQuery()` uses the returned child terms directly to build OR'd
   query terms, so a newline-suffixed term would silently fail to match
   anything in the index, corrupting real thesaurus-expanded searches.
   (The function's own first "child" always being the original parent
   term itself, e.g. `"DOG"` appearing before `"CANINE"`/`"PUPPY"`, is
   intentional, documented behavior per the function's own comment —
   not part of this bug.) Fixed with `TheEntry.Trim();` before the
   `Replace()`/`Split()` calls, the same convention already used
   elsewhere in this file (the index-time constructor's
   `ThisChild.Trim(); ThisChild.TrimLeading();`). See `BUGFIX #9` in
   source.

`tests/src/test_thesaurus.cxx` was extended with real on-disk
`THESAURUS` fixtures (a `TempFile` helper following the same
`mkstemp()` convention as `test_fpt.cxx`/`test_record.cxx`), covering:
a missing `SourceFileName` degrades to an empty thesaurus instead of
crashing (`BUGFIX #6`); construction doesn't crash when
`OpenSynonymFile("wb")` fails (`BUGFIX #7`, not independently
observable as a leak through the public API, so this is mainly a
regression smoke test); `GetChildren()`'s last entry has no trailing
newline and the exact expected length (`BUGFIX #9`); `GetParent()`
resolves a child to its parent; and 25 iterations of
`GetParent()`+`GetChildren()` hold the open-fd count steady (`BUGFIX
#8`). All fixes verified via revert-and-repro under ASan/UBSan: standalone
scratch repros for `BUGFIX #6`/`#7`/`#9`, and equivalent revert-and-repro
logic embedded directly in the new Catch2 test for `BUGFIX #8`. `make
tests`/`make tests-asan` pass clean (745 test cases, 2850 assertions).

## src/tokengen.hxx

Reprocessed via `/reprocess-blocked` (originally blocked at GENERAL
step 4 on 2026-08-07; see the resolved `docs/AUTOPILOT_LOG.md` entry for
the original finding). `TOKENGEN` splits a search-query string into
tokens (`nexttoken()`), owning a single heap-allocated `InCharP` (a
`NewCString()` duplicate of the input, freed in `~TOKENGEN()`).

1. **No copy semantics — confirmed double-free on copy** — no
   user-declared copy constructor or `operator=` existed, so the
   compiler-generated ones shallow-copied `InCharP`; confirmed via a
   standalone repro (`TOKENGEN b = a;`, let `b` then `a` go out of
   scope) triggering a real **double-free** in `~TOKENGEN()` under ASan.
   No live call site copies a `TOKENGEN` (every site either
   heap-allocates via `new TOKENGEN(...)` or direct-initializes a local
   from a `STRING`). **Decision (human, via `/reprocess-blocked`): make
   it non-copyable** — `TOKENGEN(const TOKENGEN&) = delete;` and
   `TOKENGEN& operator=(const TOKENGEN&) = delete;` added to the header.
   See `BUGFIX #1` in source.
2. **`nexttoken()`'s unmatched-quote/brace fallback corrupted the token
   in two distinct, confirmed ways** (most severe finding in this file
   — real bugs in live search-query parsing, not edge cases): on a
   failed quote or brace match, both branches tried to discard whatever
   was spuriously accumulated during the failed scan via
   `token->EraseAfter(token->SearchReverse(CLOSING_CHAR))` — but this
   only works when `token` actually *contains* a literal instance of
   `CLOSING_CHAR` to search for. For the quote-*stripping* case, the
   opening quote is deliberately never appended to `token` (it's
   skipped, not stripped *from* the token), so `SearchReverse('"')`
   always returned `0` and `EraseAfter(0)` wiped the token completely —
   not just the failed scan's contents, but any valid text accumulated
   *before* the quote was even reached. For braces, the code searched
   for the *closing* `'}'`, which by definition was never found in this
   branch (that's why the fallback runs at all), instead of the opening
   `'{'`, which *was* appended — so this always wiped the entire token
   too. Confirmed with two standalone repros (later re-verified against
   the actual fixed binary via a small standalone probe program, not
   just hand-traced): tokenizing `prefix"unmatched rest` with
   quote-stripping enabled produced `["nmatched", "rest"]` (losing
   `prefix` entirely *and* misreading `unmatched` as `nmatched` — see
   `BUGFIX #2`'s second half below); tokenizing `prefix{unmatched rest`
   produced `["unmatched", "rest"]` (losing `prefix{` entirely). Fixed
   by remembering the token's length *before* each scan attempt and
   restoring to that on failure, instead of searching for a
   possibly-absent delimiter character. See `BUGFIX #2` in source (both
   the quote and brace cases).

   **Second, related bug found and fixed in the same branch**: the
   quote-stripping case's fallback also skipped an extra character —
   `BeginQuote = ++input;` when entering the quote block already
   advances past the opening quote, but the fallback's `input =
   ++BeginQuote;` incremented it a *second* time, skipping the first
   real character after the quote (the `unmatched` → `nmatched` part of
   the repro above). The non-stripping case doesn't pre-increment
   `BeginQuote`, so its own `++BeginQuote` in the fallback is correct
   and was left unchanged; the fix makes the resume position conditional
   on `DoStripQuotes` instead of applying the same increment
   unconditionally to both cases.
3. **Header not self-contained** — `tokengen.hxx` declared
   `STRING`/`STRLIST`-typed members and parameters with
   `#include "string.hxx"`/`#include "strlist.hxx"` commented out.
   Confirmed by compiling `tokengen.hxx` as the sole `#include` in a
   translation unit: 4 errors before the fix, 0 after (re-verified after
   applying the fix, not just planned). The same defect already fixed in
   `src/fc.hxx`'s own `BUGFIX #1` and many other files this project. See
   `BUGFIX #4` in source (numbered after the parsing fixes above since
   it was the smaller, mechanical fix of the two originally flagged).

**Verified, not changed**: a pre-existing asymmetry in matched-brace
handling — the opening `'{'` is kept in the resulting token but the
closing `'}'` is silently dropped (`if (*input == '}') { input++;
istoken = 1; }` never appends it). Confirmed via the same standalone
probe used for `BUGFIX #2`, not assumed. Not flagged by the original
`docs/AUTOPILOT_LOG.md` finding and not a crash/safety issue, so left
as-is; the test file documents actual behavior rather than an assumed
symmetric one.

`tests/src/test_tokengen.cxx` covers: `TOKENGEN` is non-copyable
(`BUGFIX #1`); plain whitespace-separated words; matched quotes kept as
one literal token; the unmatched-quote-with-stripping repro from
`BUGFIX #2` above, confirming both the lost-prefix and skipped-character
bugs are fixed (`prefix"unmatched rest` → `["prefixunmatched",
"rest"]`); the analogous unmatched-brace repro; and matched braces
(documenting the verified `'{'`-kept-`'}'`-dropped asymmetry rather than
assuming symmetry). `make tests`/`make tests-asan` pass clean (693 test
cases, 2334 assertions).

## src/tokengen.cxx

`BUGFIX #1-2` above (non-copyable, and the quote/brace fallback-
corruption fixes) were already applied to this file during
`tokengen.hxx`'s turn, including adding the processed marker — but this
file's own `docs/PROCESSING_STATUS.md` row was never synced to `done`.
This turn is that sync: full re-read of `nexttoken()`/`DoParse()`/
`GetEntry()`/`GetTotalEntries()`/`SetQuoteStripping()` confirmed all
prior fixes present and correct, and found nothing new. `SetQuoteStripping()`
forcing a full reparse (`HaveParsed = GDT_FALSE; DoParse();`) was checked
for a stale-entry risk if the new parse produced fewer tokens than a
prior one — it can't: quote-stripping only changes a quoted token's
*content*, never the total token count for a given input, so `DoParse()`'s
`TokenList.SetEntry(i, ...)` always overwrites exactly the same index
range on every reparse. No `NULL`/`sprintf`, zero warnings under `-Wall
-Wextra`. No test changes: existing coverage in
`tests/src/test_tokengen.cxx` already exercises every fixed code path.
`make tests`/`make tests-asan` pass clean (745 test cases, 2850
assertions, unchanged from before this turn).

## src/squery.hxx

Reprocessed via `/reprocess-blocked` (originally blocked at GENERAL
step 4 on 2026-08-07; see the resolved `docs/AUTOPILOT_LOG.md` entry for
the original finding). `SQUERY` owns a heap-allocated `THESAURUS
*Thesaurus` (via `OpenThesaurus()`), unlike every other file in this
batch: it already had a user-declared `operator=` (just a buggy one),
but no copy constructor at all.

1. **No copy constructor, and `operator=` shallow-copied `Thesaurus` —
   confirmed heap-use-after-free** (most severe finding in this file):
   the compiler-generated copy constructor and the existing
   hand-written `operator=` both aliased `Thesaurus` rather than
   copying anything meaningfully. Confirmed via a standalone repro (open
   a thesaurus, copy-construct a second `SQUERY`, call
   `CloseThesaurus()` on each) triggering a real
   **heap-use-after-free** via a before/after test-revert under ASan
   (`STRING::~STRING() src/string.cxx:1317`, reached through
   `THESAURUS::~THESAURUS()` from the second, now-dangling
   `CloseThesaurus()` call). No confirmed copy-construction call site
   exists in the live tree. **Decision (human, via `/reprocess-blocked`):
   copies start with no thesaurus of their own** — added a real
   `SQUERY(const SQUERY&)` and fixed `operator=`, both copying
   `Opstack`/`c_kwaqs_term` but leaving the target's `Thesaurus` `null`
   rather than sharing or duplicating the source's (there's no defined
   meaning for sharing open synonym-file state). `operator=` also now
   frees the target's own previous `Thesaurus` first, instead of leaking
   it via the overwrite. See `BUGFIX #1` in source (both the new copy
   constructor and the fixed `operator=`).

   **Found while fixing `BUGFIX #1`, a separate incomplete-copy bug in
   the same function**: the existing `operator=` never copied
   `c_kwaqs_term` at all (only `Opstack` and the buggy `Thesaurus` alias
   were touched) — confirmed by inspection that `SetKWAQSTerm()`/
   `GetKWAQSTerm()` are real, meaningful accessors for this member, not
   dead code. Fixed alongside `BUGFIX #1` rather than filed separately,
   since it's the same function and the same kind of "operator= doesn't
   actually copy everything" defect.
2. **`~SQUERY()` never freed `Thesaurus`** — the destructor body was
   empty; only `CloseThesaurus()` (a separate, caller-must-remember-to-
   call method) freed it. Any `SQUERY` that called `OpenThesaurus()` and
   was destroyed without an explicit matching `CloseThesaurus()` leaked
   the `THESAURUS` and, transitively, its open file handles. No
   confirmed leak site was found in the live tree (every `OpenThesaurus()`
   caller pairs it with `CloseThesaurus()`), so this was latent, but a
   straightforward RAII fix once `BUGFIX #1`'s ownership question was
   settled. See `BUGFIX #2` in source.
3. **`CloseThesaurus()` didn't null `Thesaurus` after freeing it — became
   load-bearing the moment `BUGFIX #2` was applied**: `delete Thesaurus;`
   left `Thesaurus` a dangling pointer, so a second `CloseThesaurus()`
   call (or `ExpandQuery()`'s `if (!Thesaurus) return;` check running
   afterward) would operate on freed memory. This was already latent
   before this turn, but fixing the destructor (`BUGFIX #2`) without also
   fixing this would have introduced a **new, universally-reachable
   double-free** — every normal `SQUERY` that calls `OpenThesaurus()` +
   `CloseThesaurus()` (the paired usage every live caller already
   follows) would then double-free at destruction, since `CloseThesaurus()`
   already freed `Thesaurus` and the destructor would try to free it
   again. Found while implementing `BUGFIX #2`, not by the original
   `docs/AUTOPILOT_LOG.md` finding. Fixed by nulling `Thesaurus` after
   freeing it. See `BUGFIX #3` in source.
4. **`OpenThesaurus()` leaked a previously-open thesaurus if called
   twice** — `Thesaurus = new THESAURUS(...)` overwrote the pointer
   unconditionally, with no check for an already-open one. No confirmed
   double-`OpenThesaurus()` call site was found in the live tree, so
   latent, but a direct, easily-fixed analogue of the destructor leak
   above found while auditing every `Thesaurus`-touching method in this
   file. Fixed by freeing any existing `Thesaurus` before overwriting.
   See `BUGFIX #4` in source.

Also noted, cosmetic and harmless (removed while here): `squery.hxx`
included itself (a no-op given the include guard, confirmed by the fact
the file already compiled before this change) — almost certainly a
stray copy-paste, not a functional bug.

`tests/src/test_squery.cxx` covers: the copy constructor copies
`Opstack`/`c_kwaqs_term` but starts with no thesaurus, and both the
original's and the copy's `CloseThesaurus()` calls are independently
safe (`BUGFIX #1`, confirmed as a real bug via a before/after
test-revert under ASan — see above); `operator=` likewise copies both
members and doesn't share `Thesaurus` (`BUGFIX #1`, including the
previously-missed `c_kwaqs_term` copy); `OpenThesaurus()` called twice
doesn't leak (`BUGFIX #4`, verified under ASan's LeakSanitizer, part of
the standard `make tests-asan` run); `CloseThesaurus()` is safe to call
twice (`BUGFIX #3`); a `SQUERY` destroyed with an open thesaurus and no
explicit `CloseThesaurus()` doesn't leak (`BUGFIX #2`, also verified
under LeakSanitizer). `THESAURUS`'s search-time constructor tolerates a
nonexistent path/file cleanly (confirmed by reading `src/thesaurus.cxx`
— `OpenParentsFile()`/`OpenChildrenFile()` just `fopen()` and return
early on failure), so these tests point `OpenThesaurus()` at a scratch
path that doesn't exist rather than building real `.syn`/`.spx`/`.scx`
files on disk. `make tests`/`make tests-asan` pass clean (698 test
cases, 2343 assertions).

## src/squery.cxx

`BUGFIX #1-4` above were already applied to this file during
`squery.hxx`'s `/reprocess-blocked` turn (this file already carried
`.hxx`'s own `ISEARCH2-CLEANUP` marker, dated the same day — its
`docs/PROCESSING_STATUS.md` row just hadn't been synced to `done` yet,
same shape as `src/memcntl.cxx`/`src/mergeunit.cxx`/`src/operator.cxx`'s
stale rows). This turn's own full re-read of `SetTerm()`/`SetRpnTerm()`/
`GetTerm()`/`ExpandQuery()`/the accessors found nothing further —
`GetTerm()`/`ExpandQuery()` both correctly operate on a *local* copy of
`Opstack` (via `GetOpstack()`, which deep-copies), draining and
deleting that local copy without touching the member; `operator=`'s
lack of an explicit self-assignment guard is harmless, since every
sub-assignment it performs (`Opstack = Opstack` — safe since
`src/opstack.cxx`'s own `BUGFIX #4`, fixed earlier this batch;
`c_kwaqs_term = c_kwaqs_term` — `STRING`, safe) and the unconditional
"free then null `Thesaurus`" step match this class's own documented
"copies always start with no thesaurus" semantics regardless of
self-assignment. No `NULL`/`sprintf`, zero warnings under `-Wall
-Wextra`.

Test coverage gap found and closed: the existing
`tests/src/test_squery.cxx` only covered the `Thesaurus`-lifecycle
bugs above — `SetTerm()`/`GetTerm()`, this class's actual query-parsing
core, had no dedicated tests at all (only incidentally exercised via a
single plain term in the copy-constructor/`operator=` tests). Added
tests for a single term, a field-qualified right-truncated term
(`TITLE/whale*`), multiple OR-joined terms (confirmed reversed on
round-trip — `Opstack` is LIFO by design, not a bug: `SetTerm()`
pushes each term, `GetTerm()` pops them back off), and a direct
`SetOpstack`/`GetOpstack` round-trip. `make tests`/`make tests-asan`
pass clean (739 test cases, 2840 assertions).

---

This closes out the `/reprocess-blocked` run: all six files that were
`blocked` at the start of this run (`nlist.hxx`, `intlist.hxx`,
`mergeunit.hxx`, `thesaurus.hxx`, `tokengen.hxx`, `squery.hxx`) are now
`done`.

## src/Iutil.cxx

**Scope note, read first:** same `main()`-only structural limitation as
[`src/Iget.cxx`](#srcigetcxx)/[`src/Iindex.cxx`](#srciindexcxx)/
[`src/Isearch.cxx`](#srcisearchcxx) (can't be linked into the shared
Catch2 test binary — duplicate `main()`). This is the last of the four
`main()`-only CLI tools in the queue. Verified via a standalone
`g++ -c -Wall -Wextra` compile-clean confirmation (diffed against the
pre-edit file's own warnings: identical set, same 3
`-Wunused-parameter` and 1 `-Wunused-but-set-variable` warnings, just
shifted line numbers — no new warnings, none resolved) and manual
trace-through of every early-return path in `main()`, the same
methodology as the other three CLI tools.

Four bugs found and fixed, all resource-management issues rather than
the null-deref-on-missing-flag-argument pattern the other three CLI
tools had (this file's argument-parsing loop already reads `argv[x]`
only after each `++x >= argc` check's own `RETURN_ZERO`, so it doesn't
share `Iget.cxx`'s `BUGFIX #1`):

1. **`pdb` (the open `IDBC`/`IDB` handle) leaked when `-optimize` is
   given but the database has no `.inx.1` file yet** — `pdb = new
   IDBC(...)` is allocated well before the `-optimize` branch; when the
   `stat()` check on `.inx.1` fails, the branch prints "does not need
   optimizing" and `RETURN_ZERO`s without ever deleting `pdb`, unlike
   every other exit path in this function. Trivially reachable: this is
   the exact condition (no merge output yet) that a freshly-indexed or
   already-optimized database is in, not a contrived edge case. Fixed
   by adding `delete pdb;` before the `RETURN_ZERO`. See `BUGFIX #1` in
   source.
2. **Same `pdb` leak, mirrored in the `-collapse` branch** — identical
   shape one `if` block down: `stat()` on `.inx.1` failing prints
   "cannot be collapsed" and `RETURN_ZERO`s without deleting `pdb`.
   Fixed the same way. See `BUGFIX #2` in source.
3. **`REGISTRY*` leaks in the `-gilsdocs` per-document metadata loop**
   — `REGISTRY* metadef = parseMetaDefaults(MetaFn)` is never deleted
   after the loop that uses it, and inside the loop, `REGISTRY* meta =
   dtp->GetMetadata(record, "gils", metadef)` (confirmed via
   `doctype.cxx`'s default `GetMetadata()`: `return defaults->clone();`
   — a fresh heap allocation, caller-owned) is reassigned every
   iteration with the previous iteration's `meta` never freed. Unlike
   the other three bugs here, this one scales with database size — one
   `REGISTRY` leaked per document indexed, for the whole `-gilsdocs`
   run. Fixed by adding `delete meta;` at the end of each loop
   iteration and `delete metadef;` after the loop. See `BUGFIX #3` in
   source.
4. **Unchecked `fopen()` result in the `-gilsindex` branch** — `FILE*
   fp = fopen(GilsFile, "a")` is used immediately afterward
   (`pdb->WriteCentroid(fp)`, then `fprintf(fp, ...)`, then
   `fclose(fp)`) with no null check, unlike the sibling `-gilsdocs`
   block a few lines above it (which does check `if (fp)` before use) —
   an inconsistency that flagged this as worth a closer look. A failed
   `fopen()` (unwritable `gils.out`-adjacent directory, full disk, bad
   permissions on the target `.gils` file) is a **null-pointer
   dereference**, confirmed by inspection rather than ASan since this
   file can't be linked into the sanitizer-run test binary — see the
   scope note above. Fixed by wrapping the three calls in `if (fp)`,
   with a `perror(GilsFile)` on the else branch matching this file's
   own established error-reporting style (e.g. the `-replace` block's
   `perror(fromPath)` on a failed `rename()`). See `BUGFIX #4` in
   source.

One characteristic observed but deliberately left unfixed:

- **`Skip` is set but never read** — `-debug`'s optional numeric
  argument is parsed into `Skip` (`Skip = Temp.GetInt();`,
  confirmed via `-Wunused-but-set-variable`, unchanged from the
  pre-edit file) but nothing in this file ever reads it back; the
  `getcwd()`-filled `Cwd` buffer it reuses as scratch space is
  similarly never read for its original purpose either. Left as-is per
  this project's "don't restyle code you're not otherwise touching"
  principle — this is dead bookkeeping from an incomplete feature or
  refactor, not a correctness defect, the same category as
  `Isearch.cxx`'s already-documented vestigial `LastUsed`.

No test file was written for the reasons in the scope note above; all
four fixes are documented in detail here and at their `BUGFIX #n`
comment sites in source instead, matching `Iget.cxx`/`Iindex.cxx`/
`Isearch.cxx`'s precedent. This closes out the `main()`-only CLI tool
group — `Iget.cxx`, `Iindex.cxx`, `Isearch.cxx`, and `Iutil.cxx` are all
now `done`.

## src/datesearch.cxx

`INDEX`'s date-search methods (`index.hxx`, already `done`, is frozen —
no header changes here). Two bugs found and fixed, both the same root
cause reached through two twin functions.

1. **`SingleDateSearchBefore()`/`SingleDateSearchAfter()` returned a
   null `PIRSET` for a query date that failed to parse** (most severe
   finding in this file) — both functions try each of
   `YEAR_PREC`/`MONTH_PREC`/`DAY_PREC` in a `switch` on
   `QueryDate.GetPrecision()`, with a `default:` case for anything
   else. `SRCH_DATE::GetPrecision()` can and does return a fourth
   value, `BAD_DATE` (`date.hxx`'s `Date_Precision` enum), whenever the
   query string doesn't parse to a value in any of the three ranges
   `SetPrecision()` checks (`date.cxx`) — trivially reachable via an
   ordinary malformed date query term, not a contrived input. Both
   `default:` cases returned `YYYY`, a local `PIRSET` still at its
   declaration-time `(PIRSET)NULL`/`nullptr` and never assigned
   anything else on that path. Several callers in this same file
   immediately dereference the result with no null check:
   `DateRangeSearchContains()`'s `pirset->And(*pirset1)`,
   `DateRangeSearch()`'s `ZRelDuring`/`ZRelDuring_Strict` (`->Or()`/
   `->And()`) and `ZRelOverlaps` (`->GetTotalEntries()`), and
   `SingleDateSearch()`'s `ZRelDuring`/`ZRelEQ`/`ZRelDuring_Strict`
   (`->And()`). Confirmed with a standalone repro under
   AddressSanitizer + UndefinedBehaviorSanitizer
   (`tests/src/test_datesearch.cxx`, run against the pre-fix code with
   the fix temporarily reverted): `SingleDateSearchBefore()`/
   `SingleDateSearchAfter()` both returned `nullptr` as expected, and
   `DateRangeSearchContains()` crashed with a real SIGSEGV — UBSan
   pinpointed it precisely: `src/datesearch.cxx:1020:14: runtime error:
   member access within null pointer of type 'struct IRSET'`. Fixed by
   returning `new IRSET(Parent)` (an empty, non-null result set)
   instead of the null `YYYY` in both `default:` cases — this matches
   `DateSearch()`'s own contract everywhere else in this file (it never
   returns null; "no hits" is always a valid empty `IRSET`), and fixes
   every affected call site at the source rather than patching each one
   individually. See `BUGFIX #1`/`#2` in source; regression tests in
   `tests/src/test_datesearch.cxx` (including one exercising
   `DateRangeSearchContains()` end-to-end, the call path that used to
   crash). `make tests`/`make tests-asan` pass clean (701 test cases,
   2351 assertions).

One characteristic observed but deliberately left unfixed:

- **`DateRangeSearch()`'s `ZRelOverlaps` case looks incomplete** — it
  computes `pirset` via `SingleDateSearchAfter()`, prints a debug
  `cerr << "Got " << nhits << " with start dates after " ...` line, then
  has a large commented-out block that would have combined several more
  `SingleDateSearchBefore()`/`SingleDateSearchAfter()` calls (using the
  otherwise-unused `other_pirset`/`another_pirset` locals) before a
  trailing `delete other_pirset;` — harmless post-fix (`other_pirset` is
  never reassigned on this path, so it's always `nullptr` there, and
  `delete nullptr` is well-defined), but the case never actually
  restricts `pirset` to genuine interval-overlap matches the way its own
  name and comment imply; it just returns the "start dates after
  QueryStartDate" set unfiltered. Left as-is: completing "Overlaps"
  semantics would mean guessing intended `SQUERY` matching behavior
  nothing in this file specifies anymore, which is feature work, not a
  bug fix, and squarely the kind of judgment call this project's own
  principles reserve for a human decision rather than an autopilot
  guess -- the same category as `Isearch.cxx`'s already-documented
  `-RECT{}` finding.

## src/geosearch.cxx

`INDEX`'s geographic bounding-box search (`BoundingRectangle()`/
`Interval()`; `index.hxx`, already `done`, is frozen — no header
changes here). Two bugs found and fixed.

1. **`NumericSearch()` (`src/numsearch.cxx`) returns a null `PIRSET`
   for a non-numeric field, and every call site here dereferenced it
   immediately** (most severe finding in this file) — `NumericSearch()`
   explicitly `return((PIRSET)NULL)` when the queried field isn't
   numerically typed in this database (unconfigured, or typed "TEXT");
   `numsearch.cxx` itself hasn't reached its own turn yet (Order 158),
   so its body wasn't touched here, only this file's handling of what
   it can return. `BoundingRectangle()` calls it 4 times
   (`NORTHBC`/`EASTBC`/`SOUTHBC`/`WESTBC`) and `Interval()` calls it 4
   more (reached from `BoundingRectangle()` for the non-enclosed
   portion of the query, and directly by any other caller), and none of
   the 8 call sites checked before using the result — the exact same
   "leaf search function returns null on an unusable field, callers
   don't check" shape as last turn's `datesearch.cxx` finding. Confirmed
   with a standalone repro under AddressSanitizer + UndefinedBehavior
   Sanitizer (`tests/src/test_geosearch.cxx`, run against the pre-fix
   code with the fix temporarily reverted): a `TESTIDBOBJ` with no
   configured field types (so `NORTHBC` etc. all look non-numeric, the
   same condition a real misconfigured or non-geo database hits) made
   `BoundingRectangle()` crash with a genuine SIGSEGV at
   `LessThanNorth->And(*LessThanEast)`, the very first dereference.
   Fixed by checking each `NumericSearch()` result immediately and
   bailing out with a freshly-allocated empty `IRSET` (freeing whatever
   was already allocated in that function first) instead of
   dereferencing a possible null — matching `datesearch.cxx`'s own fix
   shape: a geo query against fields that aren't numeric can't be
   evaluated, so "no matches" is the correct empty-but-valid result,
   not a crash. See `BUGFIX #1` in source; regression tests in
   `tests/src/test_geosearch.cxx` covering `BoundingRectangle()`,
   `Interval()` directly, and the Date-Line-crossing split path (which
   funnels through `Interval()` too).
2. **`Interval()` leaked `ResultA` on one of its five empty-intersection
   early returns** — the function bails out early five separate times
   whenever an intermediate result set comes up empty, and four of
   those five correctly `delete ResultA` first (it's the accumulator
   carried across the whole function); the fifth — right after
   `ResultC->And(*ResultD)` comes up empty — returned `ResultC` without
   ever freeing `ResultA`. Confirmed by direct comparison with the
   other four sibling branches in the same function, which are
   unambiguous precedent for what this one should do too; not
   independently reproduced with a live on-disk index (would need a
   populated numeric index to make `ResultA` non-empty going in, a
   disproportionately large fixture for a leak that's already obvious
   by inspection against its own function's other branches). Fixed by
   adding the same `delete ResultA;` this path was missing. See
   `BUGFIX #2` in source. `make tests`/`make tests-asan` pass clean
   (712 test cases, 2378 assertions).

## src/merge.cxx

Heapsort utilities (`hsort`/`buildHeap` for a generic `void*` buffer,
`GpHsort`/`buildGpHeap` for a `GPTYPE` array) plus a `MERGE` class
declared but never implemented. `merge.hxx` (Order 151) hadn't been
processed yet, so this turn read both files fresh — no sibling turn to
close out.

1. **`buildGpHeap`'s sibling-comparison bound check was off by one →
   confirmed heap-buffer-overflow** — guarded the `data[childpos+1]`
   read with `childpos < heapsize` (already true from the enclosing
   `if`, so redundant as written) instead of `childpos + 1 < heapsize`
   — the check its sibling `buildHeap()` a few dozen lines below
   already gets right for the identical "does this node have a second
   child" question. Whenever `childpos` landed exactly on
   `heapsize - 1` (its own last valid index), this read one element
   past `heapsize` — and on `GpHsort()`'s very first pass, `heapsize`
   equals the full array size, so that was one past the actual
   allocation. Confirmed with a standalone repro under ASan: an
   8-element array (the smallest size where this boundary is reached)
   produced a heap-buffer-overflow read, and — since this is a
   correctness bug as much as a memory-safety one — a stress test
   across sizes 1-64 showed the *output* was already wrong (reading
   adjacent, not-yet-placed heap elements as if they were real sibling
   values, misordering the sort) even on sizes too small to cross an
   actual allocation boundary. Fixed by matching `buildHeap()`'s
   already-correct check. See `BUGFIX #1` in source. Verified fixed:
   the same repro and stress test now pass cleanly, and
   `tests/src/test_merge.cxx` has a dedicated regression test sweeping
   sizes 1-20 including the exact boundary.

2. **`merge.cxx` never included `merge.hxx` at all** — the root cause
   that let `BUGFIX #3` (below) go undetected: with the two files never
   sharing a translation unit, the compiler never had a chance to
   cross-check the header's declarations against what's actually
   defined here. Fixed by adding the `#include`, the same convention
   every other file in this tree follows for its own header. See
   `BUGFIX #2` in source.

3. **`merge.hxx`'s `buildHeap()` declaration didn't match its
   definition** — declared taking `int *data`, but the actual
   definition here takes `void *data` and does generic, width-
   parameterized byte arithmetic internally (the same `qsort`-style
   contract `hsort()`/`buildGpHeap()`'s own declarations already use)
   — `int *data` would have meant reinterpreting arbitrary-width
   elements as `int`s, wrong for any width other than `sizeof(int)`.
   Only surfaced once `BUGFIX #2` made both declarations visible in the
   same translation unit. Header change, but safe to apply directly
   rather than stop-and-ask (the usual GENERAL step 4 concern): `grep`
   across the whole tree confirms `merge.hxx` has no includer besides
   `merge.cxx` itself and no external caller of any function it
   declares, so nothing else's visible contract changes — the same
   reasoning `src/md5.hxx`'s `BUGFIX #1` used for its in-place
   `uint32` fix. Fixed by matching the header to what's actually
   implemented. See `BUGFIX #3` in source (`merge.hxx`).

   Also fixed in the same header, same "no external includer" basis:
   `AddChunk`'s `GlobalStart` parameter was declared `GPTYYPE` — a
   typo, the only occurrence of that misspelling anywhere in the tree
   — corrected to `GPTYPE`. `MERGE::AddChunk` itself is declared but
   has no definition anywhere in this tree and nothing calls it;
   left as a declaration only rather than invented from scratch, since
   there's no specification or caller to validate an implementation
   against.

4. **`buildGpHeap`'s `reverse` parameter is silently unused** —
   modernization/hygiene, not a behavioral gap: unlike `buildHeap()`,
   this function has no reverse-aware branch at all, but every actual
   call in this tree (both inside `GpHsort()`, its only caller) passes
   `reverse=0` literally, and nothing outside this file calls
   `buildGpHeap` at all (confirmed via `grep`) — so there's no
   observable case where a caller's requested reversal is silently
   dropped. Kept for signature symmetry with `buildHeap()`. Silenced
   with `(void)reverse;` rather than removing the parameter (a header
   signature change) or implementing unused reverse logic (inventing
   behavior with no caller to validate it against). See `BUGFIX #4` in
   source.

Also fixed to compile clean under `-Wall -Wextra` (pre-existing
warnings, not introduced this turn): several `-Wsign-compare`
comparisons between `int` (`position`/`childpos`) and `size_t`
(`heapsize`) in both `buildGpHeap()` and `buildHeap()`, resolved with
casts rather than changing either parameter's type.

`src/merge.cxx` wasn't linked into `TEST_ENGINE_SRCS` before this turn
(only the real production build, via `src/Makefile`'s legacy `OBJ`
list reached through the top-level `Makefile`'s `isearch`/`smoke-test`
targets, ever compiled it) — added, and `tests/src/test_merge.cxx`
covers `GpHsort`/`hsort` correctness across sizes 1-20 (`buildHeap`/
`buildGpHeap` are exercised transitively; `MERGE::AddChunk` has no
definition to test). `make tests`/`make tests-asan` pass clean (729
test cases, 2823 assertions).

## src/merge.hxx

`BUGFIX #2-4` above (the missing `#include`, `buildHeap()`'s signature
drift, the `GPTYYPE` typo, and the unused `reverse` parameter) were
already found and fixed directly in this header during `src/merge.cxx`
(Order 150)'s turn — this file's own turn found nothing further.
Confirmed self-contained by compiling it as the sole `#include` in a
translation unit (clean). Added a short file-level doc comment per
GENERAL step 7; no test changes needed beyond what
`tests/src/test_merge.cxx` already covers.

## src/multiterm.cxx

No separate header (`INDEX::MultiTermSearch`, implemented here, is
declared in `src/index.hxx`). One large function: a binary search over
one or more sorted, chunked `.inx` sistring index files for an exact/
prefix text term, optionally field-restricted via a cached/disk field-
position lookup.

1. **`Cache` allocated with array `new`, freed with scalar `delete` —
   confirmed alloc-dealloc-mismatch** — `Cache=new GPTYPE[CacheSize*2]`
   but `delete Cache;` (not `delete [] Cache;`) at cleanup. Confirmed
   real with a standalone repro under ASan, reproducing the exact
   allocation/deallocation shape used here (`alloc-dealloc-mismatch
   (operator new [] vs operator delete)`). Reachable on any field-
   restricted search (`CheckField==1`, i.e. any search that names a
   field — a mainline usage pattern, not an edge case). Fixed by
   changing to `delete [] Cache;`. See `BUGFIX #1` in source.

2. **The `.num` chunk-count file's `ffopen()` result went unchecked
   before `fgets()`** — `fx=Parent->ffopen(CheckName,"r");` followed
   directly by `fgets(buf,256,fx)` with no null check; a null `FILE*`
   there is undefined behavior (typically a crash). `INDEX::TermSearch`
   (this function's only caller) already verifies the same file opens
   successfully moments earlier, so this is a narrow TOCTOU-style gap
   (the file being removed, or `ffopen`'s cache evicting differently,
   between that check and this reopen) rather than a demonstrated live
   failure — defensive fix, not independently reproduced, matching this
   function's own null-check convention two lines below (the "fgets
   failed" case) and the one a few dozen lines further down for the
   per-chunk `.inx` file's `ffopen`. See `BUGFIX #2` in source.

3. **`sprintf` for the per-chunk filename suffix** — modernized to
   `snprintf(buf, sizeof(buf), ...)`. `kk` (an `INT` chunk number) into
   a 256-byte buffer was never a live overflow, just modernization. See
   `BUGFIX #3` in source. Also modernized throughout: all `NULL` →
   `nullptr`.

4. **A nested-scope re-declaration of `pirset` shadowed and leaked the
   outer allocation — confirmed via LeakSanitizer** (most severe
   finding in this file): the function allocates
   `PIRSET pirset = new IRSET(Parent);` once at entry, but the
   "only stop words, return an empty IRSET" branch inside the
   per-chunk loop re-declared `PIRSET pirset = new IRSET(Parent);` in
   its own nested scope — shadowing, not reusing, the outer variable —
   then returned that new one. The outer `pirset` (itself just an
   empty `IRSET`, since nothing touches it before this branch can be
   reached — `PhraseBeg`/`OrigTermLength` are recomputed fresh from the
   same `QueryTerm` on every chunk, so this branch either fires on the
   very first chunk, before anything's been added, or never fires at
   all) was then orphaned. Confirmed with a standalone repro under
   LeakSanitizer reproducing the exact shadowing shape with the real
   `IRSET` class: 32KB leaked per call (the `IRSET` object plus its
   internal `Table`/`ATTRLIST`/`STRING` allocations). Not independently
   reproduced through `MultiTermSearch` itself, which needs a real
   on-disk index fixture (`.num`/`.inx` files) disproportionate to a
   leak already unambiguous from C++ scoping rules alone — the same
   judgment call already made for `src/geosearch.cxx`'s `BUGFIX #2`.
   Fixed by returning the outer `pirset` directly instead of shadowing
   it. See `BUGFIX #4` in source.

No test file added: `MultiTermSearch` is this file's only function,
and — like `INDEX::Search`/`AddRecordList` per `tests/src/test_index.cxx`'s
own note — needs a full on-disk index (`DFDT`, `MDT`, postings/`.inx`
files) to exercise meaningfully; both fixes above were instead verified
via standalone repros reproducing the exact allocation/deallocation
patterns with the real classes involved. `make tests`/`make tests-asan`
pass clean (729 test cases, 2823 assertions, unchanged from before this
turn since no new tests were added).

## src/numsearch.cxx

No separate header (`INDEX::NumericSearch`/`SortNumericFieldData`,
implemented here, are declared in `src/index.hxx`). Implements
`INDEX`'s numeric/computed-field search: `NumericSearch()` binary-
searches an on-disk `NUMERICLIST` for a boundary matching a query
value under a relation (`=`/`<`/`<=`/`>=`/`>`); `SortNumericFieldData()`
sorts and rewrites each `NUM`/`COMPUTED`/`DATE`/`DATE-RANGE` field's
on-disk index by value, then by global pointer (and by end value too,
for date ranges) as part of indexing.

1. **`NumericSearch()` returned raw `nullptr` when asked to search a
   non-numeric field — the actual source of a null-return pattern
   already confirmed to crash 8+ call sites this pass** — this is the
   function `src/geosearch.cxx`'s `BUGFIX #1` (8 call sites) and
   `src/datesearch.cxx`'s `BUGFIX #1` were patching call-site symptoms
   of; this turn fixes the actual root. `if(FieldType == "TEXT") return
   ((PIRSET)NULL);` fired whenever `NumericSearch()` was asked to
   search a field that isn't numeric (including the default when no
   field type is registered at all). Every current caller has since
   been fixed to guard against a null return — `geosearch.cxx`'s 8 call
   sites directly, everything reached through `src/index.cxx` by way of
   its blanket `if (!NewIrset) NewIrset = new IRSET(Parent);` catch-all
   (confirmed by reading every call path: `src/multiterm.cxx`'s
   non-TEXT-field fallback, `INDEX::TermSearch`'s `DOUBLE`/`STRING`
   overloads, all ultimately reach that one guarded assignment) — so
   this exact `return nullptr` is no longer reachable-and-unguarded
   anywhere in the tree today. Fixed at the root anyway, matching this
   codebase's own established convention (every other "can't search
   this way" boundary condition, including this same function's own
   `NO_MATCH` path a few lines below, already returns an empty `IRSET`,
   never `nullptr`) rather than leaving a footgun for any future
   caller. See `BUGFIX #1` in source. Verified with a new
   `tests/src/test_numsearch.cxx`: unlike `NumericSearch()`'s main body
   (which needs a real on-disk numeric index file to exercise
   meaningfully — the same documented scope boundary
   `tests/src/test_index.cxx` already draws for this area), this
   specific path fires before any file I/O — a fresh `TESTIDBOBJ`'s
   `FieldTypes` `HASH` is empty, so any field name resolves to `TEXT`
   with no on-disk setup needed — confirmed via revert-and-repro
   (reverting to `nullptr` fails the new test's `REQUIRE(result !=
   nullptr)` immediately).

Also checked and found clean: no `NULL`/`sprintf` in live code (one
`sprintf` exists only inside a large commented-out "rset cache" block,
left untouched); compiles with zero warnings attributable to this file
itself under `-Wall -Wextra` (every warning surfaced while compiling it
comes from other, not-yet-processed headers this file happens to
include transitively). `NumericSearch()`'s main binary-search body and
`SortNumericFieldData()` were both read in full and found structurally
sound — every `fopen()`/`fread()` result is already checked, and
`NUMERICLIST::LoadTable()`/`Resize()` (this file's own turn,
`src/nlist.cxx` above) already clamp out-of-range indices internally.
`make tests`/`make tests-asan` pass clean (730 test cases, 2825
assertions).

## src/zpresent.cxx

**Scope note, read first:** same `main()`-only structural limitation as
[`src/Iget.cxx`](#srcigetcxx)/[`src/Isearch.cxx`](#srcisearchcxx) (can't
be linked into the shared Catch2 test binary — duplicate `main()`).
Unlike those two, this file's bugs were verified with a real end-to-end
runtime differential, not just static inspection: built the actual
production binaries (`make isearch`), indexed a real document under the
`FGDC` doctype (`./bin/Iindex -d ... -t FGDC ...`, whose `Present()`
genuinely branches on the record-syntax parameter — confirmed by
reading `doctype/fgdc.cxx`), and ran `./bin/zpresent` against it with
different `-f` values before and after each fix.

Two bugs found and fixed:

1. **`-f` (record syntax) is parsed, validated, and echoed into the
   output, but never actually used** (most severe finding in this
   file) — `main()` computes `RecordSyntax` from the `-f` flag with the
   same OID-normalization logic as `src/Isearch.cxx` (verbatim
   identical block), and prints it into the `<isearch:results ...
   prs="...">` attribute — but the one `pdb->Present(RsRecord, ESet,
   XmlRecordSyntax, &Record);` call hardcoded the literal
   `XmlRecordSyntax` constant instead of passing the local
   `RecordSyntax` variable. `Isearch.cxx`'s own three `Present()` calls
   all correctly pass their computed `RecordSyntax`; this was the one
   place in `zpresent.cxx` that didn't. Confirmed live and reachable,
   not just by inspection: indexed a real document as `FGDC` (whose
   `Present()` wraps a title in `<title>...</title>` XML tags when
   `RecordSyntax.CaseEquals("XML")`, or leaves it plain otherwise —
   `doctype/fgdc.cxx` lines ~1219-1238) and ran `zpresent -f TEXT`
   against it: with the bug, the output was still `<metadata><idinfo>
   ...<title>...` XML-wrapped despite the `prs="SUTRS"` attribute
   claiming otherwise; with the fix, `-f TEXT` correctly produced plain,
   untagged output. Revert-and-repro (reverted the fix, rebuilt, reran
   the same two commands) reproduced the exact broken behavior, then
   restoring the fix reproduced the correct behavior again. Fixed by
   passing `RecordSyntax` instead of `XmlRecordSyntax`. See `BUGFIX #1`
   in source.
2. **Dead "unrecognized arguments" check (wrong comparison direction)**
   — the same defect as `src/Iget.cxx`'s `BUGFIX #2`: `x = LastUsed + 1;
   if (x > argc) { ... "Unrecognized arguments" ... }`. `LastUsed` is
   only ever assigned a valid `argv` index (always `< argc`), so `x =
   LastUsed + 1` can never exceed `argc` — `x > argc` was permanently
   false, and this check had never actually fired. Traced through a
   valid-usage case (`zpresent -d db -id 123`, ends with `x == argc`,
   no error either way) and a broken-usage case (`zpresent -d db
   extra_garbage`, ends with `x == 3, argc == 4`: the old `x > argc`
   check misses it, `x < argc` catches it) to confirm the fix doesn't
   regress valid usage. Confirmed against the real binary both ways: a
   trailing unrecognized argument now correctly produces an
   `status="Error"`/`Unrecognized arguments` response instead of being
   silently accepted. Unlike `Iget.cxx`'s equivalent `BUGFIX #1`
   (missing-argument-value null-deref), this file doesn't have that
   companion bug — every `-flag`-without-a-value branch here calls
   `XmlBail()`, which unconditionally `exit()`s, so there's no fall-
   through to the unchecked `argv[x]` read. Fixed by changing `x > argc`
   to `x < argc`. See `BUGFIX #2` in source.

Also noted, not changed: `GDT_BOOLEAN Error=GDT_FALSE;` is set once
(`Error=GDT_TRUE;`, immediately followed by an `XmlBail()` call that
exits) and never read anywhere — confirmed dead via
`-Wunused-but-set-variable` (the only warning on this file's own lines;
every other warning from a standalone `-Wall -Wextra` compile comes
from already-existing conditions in transitively-included headers, not
from anything touched this turn). Same shape as `src/Isearch.cxx`'s
vestigial `LastUsed` finding — an incomplete-removal leftover from an
earlier refactor, not a functional bug, and left alone per this
project's "don't restyle code you're not otherwise touching" principle.
`DTREG dtreg(0);` (a raw `0` for a `PIDBOBJ` parameter) was left
un-modernized to match `src/Isearch.cxx`'s identical, already-processed
line, rather than diverging from that sibling file's established
precedent.

No test file was written for the reasons in the scope note above;
both fixes are documented in detail here and at their `BUGFIX #n`
comment sites in source instead, plus the runtime differential
confirmation above (stronger than `Iget.cxx`/`Isearch.cxx`'s
inspection-only verification). `make isearch`/`make smoke-test` both
pass clean; `make tests`/`make tests-asan` (unaffected by this file,
which isn't part of `TEST_ENGINE_SRCS`) still pass clean (745 test
cases, 2850 assertions).

## src/zsearch.cxx

**Scope note, read first:** same `main()`-only structural limitation as
[`src/Iget.cxx`](#srcigetcxx)/[`src/Isearch.cxx`](#srcisearchcxx)/
[`src/zpresent.cxx`](#srczpresentcxx) (can't be linked into the shared
Catch2 test binary). Verified with a mix of runtime confirmation
(building the real production binary and library, `make isearch`, and
triggering each bug directly) and AddressSanitizer against a standalone
build linked against the real `libIsearch.a` — stronger than
`Iget.cxx`/`Isearch.cxx`'s inspection-only verification, matching
`zpresent.cxx`'s precedent from this same batch.

Two bugs found and fixed:

1. **Missing-argument-value null-pointer dereference on all six value-
   taking flags** (most severe finding in this file, and trivially
   reachable from an ordinary command-line typo) — the exact same
   defect as `src/Iget.cxx`'s `BUGFIX #1`, but present at six sites
   here instead of four (`-o`, `-d`, `-p`, `-f`, `-prefix`, `-suffix`):
   `if (++x >= argc) { Error=GDT_TRUE; error_message.Cat(...); }`
   followed *unconditionally* by an `argv[x]` read, no `else`. When the
   flag is the last argument, `x == argc` and `argv[argc]` is guaranteed
   `nullptr` by the C++ standard, dereferenced via
   `STRING::operator=(const CHR*)`'s `strlen()` call. Confirmed with the
   real production binary, not just inspection: `./bin/zsearch -d <db>
   -o` (and the same for each of the other five flags) segfaulted
   (`SIGSEGV`, exit 139) before this fix; after moving each `argv[x]`
   read into an `else` branch (six sites), all six now exit cleanly
   (exit 0) with the proper XML error message instead. See `BUGFIX #1`
   in source (all six sites).
2. **Use of `WordList`/`pdb` after they're freed, when the database is
   incompatible — confirmed heap-use-after-free** — `if
   (!pdb->IsDbCompatible()) { Error=GDT_TRUE; error_message.Cat(...);
   delete [] WordList; delete pdb; }` frees both, sets `Error`, but
   (unlike every other error path in this file) never exits — execution
   falls straight through into code that unconditionally reads
   `WordList[z]` (building `QueryString`, a few lines below) and, later,
   calls methods through `pdb` again (`Search()`/`AndSearch()`) if
   `Error` didn't get checked before then. Confirmed real with
   AddressSanitizer: hand-edited a real indexed database's `.dbi` file
   (`MagicNumber` line, which `IDB::IsDbCompatible()` reads and compares
   against `IsearchMagicNumber` in `src/idb.cxx`) to force
   `IsDbCompatible()` to return false, then ran a standalone ASan build
   linked against the real `libIsearch.a` — a genuine
   **heap-use-after-free READ** at the `WordList[z]` access, freed a few
   lines earlier by the same function. Fixed by printing the same
   error/exit XML block already used (twice) elsewhere in this file and
   exiting immediately, instead of falling through past the just-freed
   pointers. Re-verified clean under the same ASan build after the fix
   (no error, proper XML error response, exit 0), and confirmed a
   restored/compatible database still searches normally. See `BUGFIX #2`
   in source.

Also noted, not changed:
- `PRSET prset=(PRSET)NULL; PIRSET pirset=(PIRSET)NULL;` modernized to
  `nullptr` (dropping the now-redundant C-style casts).
- **Vestigial flags matching `Isearch.cxx`'s already-documented
  finding** — `QuitFlag`, `ByteRangeFlag`, `TerseFlag` (all "set but not
  used"), and `SpatialRectFlag`/`-RECT{...}` (declared, "used" only in
  its own `=0` initializer, unimplemented) are the same shape of
  incomplete-removal/never-implemented leftovers already found and
  deliberately left alone in `src/Isearch.cxx`'s own turn. Confirmed via
  `-Wall -Wextra`, diffed against the pre-edit committed file (identical
  warning set before and after this turn's two fixes, just shifted line
  numbers) — nothing here is new.
- **`INFIX2RPN* Parser` isn't `delete`d on the parse-failure branch** —
  found while reading the `-infix` handling (`Parser = new
  INFIX2RPN(...)`; `delete Parser;` only runs inside `if
  (Parser->InputParsedOK())`, not the `else`). Looked like a leak on
  first read, and the identical code exists un-flagged in the
  already-processed `src/Isearch.cxx` too — but verified with
  LeakSanitizer against a real invalid-infix-query invocation
  (`zsearch -d <db> -infix '((( unbalanced'`) that it is **not**
  actually flagged as a leak: `Parser`'s value is still sitting in a
  live stack slot inside `main()` at the moment `exit(0)` runs a few
  lines later (this file, like every CLI tool in this project, calls
  `exit()` from inside `main()` rather than returning), so LSan's
  reachability scan finds it "still reachable," not leaked — matching
  (and explaining the mechanism behind) the same pre-existing 852-byte
  "leak" this file already shows on an ordinary successful search
  (`SQUERY`/`ATTRLIST`/`STRING` internals, also never explicitly freed
  before `exit()`, also not LSan-flagged for the same reason). Since the
  process terminates immediately after either way, there's no practical
  resource impact to fix, and "fix" here would mean adding a `delete`
  that a leak checker itself confirms is inert in this control flow —
  left unchanged, matching `Isearch.cxx`'s identical, already-reviewed
  code.

No test file was written for the reasons in the scope note above; both
fixes are documented in detail here and at their `BUGFIX #n` comment
sites in source instead, plus the runtime/ASan confirmation above.
`make isearch`/`make smoke-test` both pass clean; `make tests`/`make
tests-asan` (unaffected by this file, which isn't part of
`TEST_ENGINE_SRCS`) still pass clean (745 test cases, 2850 assertions).

## doctype/Unified/gen_unified.c

**Scope note, read first:** a standalone C `main()`-only build-time
generator, not part of the C++ engine at all and not wired into the top
-level `Makefile` (`doctype/Unified/` has its own, separate `Makefile`,
never invoked from the tree root) — same untestable-via-Catch2
limitation as the `main()`-only CLI files, for a different reason (this
one isn't even C++). Reads `UnifiedList.txt` (`presentation-string
[bib-1-number]` per line) and writes `unified.h`/`unified.c`/
`unified.inc`: a table mapping Bib-1/GILS field names to numeric IDs,
consumed by several `doctype/*.cxx` parsers. **This turn also
discovered and corrected a status-tracking gap**: `doctype/Unified/
unified.c`/`unified.h` (queue rows 184/185) and their duplicate copies
`doctype/unified.c`/`doctype/unified.h` (rows 228/229, byte-identical,
apparently kept in sync by hand for the main build's `#include` path)
are this generator's output — the exact `generated`-row relationship
CLAUDE.md describes for `src/dtreg.cxx`/`.hxx`, which `ANALYZE` didn't
recognize for this generator/directory shape. All four rows reclassified
from `pending` to `generated` as part of this turn (see below) rather
than run through GENERAL individually.

Three bugs found and fixed, verified via `-Wall -Wextra` (compile-clean
confirmation) and a real regeneration diff (rebuilt `gen`, ran it
against the real `UnifiedList.txt`, confirmed byte-identical output to
the currently-committed `unified.c`/`.h`/`.inc` — proving these fixes
are purely defensive and don't change behavior):

1. **Missing `<stdlib.h>`/`<ctype.h>` — confirmed via compiler warning,
   not just inspection** (most severe finding in this file) — `exit()`,
   `atoi()`, `isspace()`, `isdigit()`, `tolower()` were all called with
   neither header included. `gcc -Wall -Wextra` on the original file
   reported five separate `-Wimplicit-function-declaration` warnings
   (`exit`, `isspace`, `isdigit`, `atoi`, `tolower`) plus
   `-Wimplicit-int` on `main` itself. Implicit declarations assume an
   `int`-returning, unprototyped signature — undefined behavior per the
   C standard even though it happens to be harmless on this specific
   platform (the real signatures' calling conventions match closely
   enough by luck). The same "header not self-contained" defect class
   already fixed throughout this project (e.g. `src/fc.hxx`'s
   `BUGFIX #1`). Fixed by adding both includes; `-Wall -Wextra` is clean
   after. See `BUGFIX #1` in source.
2. **`freopen()`'s return value never checked** — `if (argc != 1)
   freopen(argv[1], "r", stdin);` with no check. If `argv[1]` doesn't
   exist or can't be opened, `freopen()` fails and leaves `stdin`
   unusable; the `fgets()` loop below would then simply never execute,
   silently producing near-empty (header-only) `unified.h`/`.c`/`.inc`
   instead of reporting the real problem — a silent-corruption failure
   mode in a code-generation tool, arguably worse than a loud crash.
   Confirmed with the real binary: `./gen /nonexistent/path` now prints
   `ERROR: could not open input file ...` and exits nonzero, instead of
   silently emitting broken output. Fixed by checking the return value.
   See `BUGFIX #2` in source.
3. **`fp3` (the `.inc` output file) not checked for `NULL`, unlike
   `fp1`/`fp2`** — `if (fp1 == NULL || fp2 == NULL) { ...; exit(-1); }`
   omitted `fp3`, so if opening `unified.inc` specifically failed (e.g.
   a pre-existing non-writable file or directory with that name, while
   `unified.h`/`.c` still open fine), execution would reach the
   unconditional `fprintf(fp3, ...)` a few lines down as a null-pointer
   dereference. A narrow window (all three opens target the same
   directory, so a directory-wide permission problem is already caught
   by the `fp1`/`fp2` check) but a real, asymmetric gap relative to its
   two siblings. Fixed by adding `fp3 == NULL` to the check. See
   `BUGFIX #3` in source.

Also modernized: `sprintf` → `snprintf` in `myopen()`; `main(int argc,
char **argv)` → `int main(int argc, char **argv)` (the missing return
type was exactly what triggered `-Wimplicit-int` above).

**Verified, not changed** — a shape of bug that looked plausible on
first read but was ruled out empirically, matching this project's
"don't overclaim without a reproduced failure" discipline: the
per-character field-name loop nulls out `buf[i]`/`temp[i]` at the
*first* whitespace character (to isolate the presentation-string from
an optional trailing Bib-1 number) but keeps scanning and writing
`temp[]` past that point for the rest of the line. This looked like it
could contaminate a field name with trailing digits, or truncate a
genuinely multi-word (space-separated, not hyphenated) field name down
to just its first word. Traced through a real example
(`"Abstract 62\n"`) by hand: since both `fprintf(..., "%s", temp)` and
`buf` reads stop at the *first* `'\0'`, the later, past-the-terminator
writes are dead — never observed by any output function — confirming
the real generated output (`_Sabstract`, no digit suffix) matches
what's expected. The multi-word-name truncation shape doesn't apply
either: checked every real entry in `UnifiedList.txt` after stripping
`#`-comments and confirmed none has more than 2 whitespace-separated
tokens (name, optional number) — the file's own convention always joins
multi-word names with `-`, translated to `_` by this same loop, never
with a raw space. Not reachable against the real input; left unchanged.

No test file was written for the reasons in the scope note above;
verified via `-Wall -Wextra` compile-clean and the regeneration-diff
confirmation described above (stronger than a pure inspection-only
verification, closer to this batch's `zpresent.cxx`/`zsearch.cxx`
runtime-confirmed bugs).

## doctype/Unified/unified.c, doctype/Unified/unified.h, doctype/unified.c, doctype/unified.h

Machine-generated by `doctype/Unified/gen_unified.c` from
`doctype/Unified/UnifiedList.txt` (see that file's entry above) — not
hand-edited, not run through the GENERAL pipeline. `doctype/unified.c`/
`.h` are byte-identical duplicates of `doctype/Unified/unified.c`/`.h`,
kept in sync for the main build's `#include "unified.h"` path (which
expects the file directly under `doctype/`, not in the `Unified/`
subdirectory). Regenerated during `gen_unified.c`'s own turn and
confirmed byte-identical to what's already committed (that generator's
turn touched compile-time safety, not output-affecting logic). All four
`docs/PROCESSING_STATUS.md` rows reclassified `pending` → `generated`
as part of this turn, matching the `src/dtreg.cxx`/`.hxx` convention —
`ANALYZE` didn't recognize this generator/directory shape and had
queued them as ordinary `pending` files.

## doctype/dtconf.cxx

**Scope note, read first:** a standalone C++ `main()`-only build-time
generator, the canonical `generated`-relationship source CLAUDE.md
already describes: reads `doctype/dtconf.inf` (one doctype stem per
line) plus each listed doctype's own `.hxx` (to pull its class name and
`Description:` comment), and writes **three** files relative to its own
working directory — `../src/dtreg.hxx`, `../src/dtreg.cxx` (both
already `generated` rows, confirmed unaffected below), and
`../src/Makefile` itself (regenerated from `../src/Makefile.000`,
substituting doctype-specific `###DTOBJ###`/`###DTHXX###`/
`###DTMAKE###` sections). That last one made this turn unusually
high-stakes: the *real* `src/Makefile` has been extensively hand-
extended by this entire cleanup project (`tests`/`tests-asan`/
`smoke-test` targets, `TEST_ENGINE_SRCS`, etc. — see CLAUDE.md's BUILD
section) and has diverged from `src/Makefile.000` by design. Actually
running the real binary against the real tree would have **overwritten
and destroyed all of that test infrastructure**. Verified instead in a
fully isolated sandbox (`/tmp/...`, never touching the real tree):
copied `dtconf.inf`, every `doctype/*.hxx`, and `src/Makefile.000` into
a scratch `doctype/`+`src/` pair, compiled and ran the fixed
`dtconf.cxx` there, and diffed its output against the real, currently-
committed files.

Two bugs found and fixed, both confirmed via `-Wall -Wextra` — not just
inspection:

1. **`sprintf(t, "%s.hxx", DtFn[TotalDt])` — confirmed possible buffer
   overflow via `-Wformat-overflow`** — the compiler's own warning
   spelled it out: `sprintf output between 5 and 84 bytes into a
   destination of size 80`. `DtFn[TotalDt]` can be up to 79 characters
   (bounded only by `fgets()`'s own `MAXSTR` cap on the `dtconf.inf`
   line it came from), and appending `.hxx` plus the terminator can
   need up to 84 bytes — 4 more than `t`'s 80-byte buffer. Not
   reachable with any of the real (short) doctype names in
   `doctype/dtconf.inf`, but a real latent overflow for a
   sufficiently long one. Fixed two ways: `snprintf(t, sizeof(t), ...)`
   instead of `sprintf`, and widened `t`'s buffer from `MAXSTR` to
   `MAXSTR+8` so the true worst case (79 + 4 + 1) fits with room to
   spare — `-Wall -Wextra` is fully clean after (the `snprintf`-alone
   fix left a residual, expected `-Wformat-truncation` warning, since
   `snprintf` can still truncate within its declared bound; widening
   the buffer removes even that). See `BUGFIX #1` in source.
2. **No bound on `TotalDt` against `MAXDT` (500)** — `DtName`/`DtFn`
   are fixed-size `static char [MAXDT][MAXSTR]` arrays; `TotalDt` was
   incremented once per `dtconf.inf` entry with no check against
   `MAXDT` anywhere, so a config file listing more than 500 doctypes
   would silently overflow both static arrays. `doctype/dtconf.inf`
   currently lists 38 — nowhere near the limit, so not practically
   reachable today, but a real static-buffer overflow for unbounded
   config input, the same category of finding already fixed several
   times this project for other fixed-capacity tables (e.g.
   `TH_PARENT_LIST::AddEntry`'s `BUGFIX #1`). Fixed by refusing to
   process further entries once the table is full instead of writing
   past it (printing a warning to `stderr` and stopping, rather than
   silently truncating the doctype list with no explanation). See
   `BUGFIX #2` in source.

**Verified, not changed**: if a listed doctype's `.hxx` opens
successfully but contains no `class ...` line at all, `DtName[TotalDt]`
is never written — since it's a zero-initialized `static` array slot
used exactly once, this just prints an empty class name for that entry
(`printf("\t%s", DtName[TotalDt])` prints nothing after the tab) rather
than crashing. Every one of the 38 real `.hxx` files this turn's
sandbox run processed does contain a `class` declaration (confirmed:
all 38 printed real class names, none empty), so this is unreachable
against the real tree today; not fixed, since guessing at
"file has no class" recovery behavior for a scenario the current tree
never hits isn't worth guessing at.

**Regeneration confirmed behavior-preserving**: the sandbox run's
`dtreg.hxx`/`dtreg.cxx` output diffed byte-identical against the real,
currently-committed `src/dtreg.hxx`/`src/dtreg.cxx` — proving both
fixes are purely defensive and change nothing about what gets
generated for the real `dtconf.inf`. The sandbox's regenerated
`Makefile` (from the copied `Makefile.000`) was inspected for sane
structure (601 lines, correct per-doctype `.o` rules) but was
**deliberately never compared against, or written over, the real
`src/Makefile`** — that file has diverged from `Makefile.000` on
purpose throughout this whole project, and doing so would risk
suggesting it should be regenerated, which it must not be. The real
`src/Makefile` was not touched by this turn.

No test file was written for the reasons in the scope note above (a
`main()`-only build-time tool, not linkable into the Catch2 binary);
verified via `-Wall -Wextra` compile-clean and the isolated-sandbox
regeneration-diff confirmation described above.

## Isearch-cgi/config.cxx

`BUGFIX #1` above (self-containment) was already applied to the paired
header during `config.hxx`'s own turn, but this `.cxx`'s
`docs/PROCESSING_STATUS.md` row was never synced to `done`. This turn
is that sync: the file is a single line, `const CHR *IsearchCGIVersion
= VERS;`, matching the header's `extern` declaration exactly — nothing
to find. No `NULL`/`sprintf`, zero warnings under `-Wall -Wextra`. No
test changes: `tests/Isearch-cgi/test_config.cxx` (written during
`config.hxx`'s turn) already covers this exactly —
`IsearchCGIVersion != nullptr` and `IsearchCGIVersion == VERS`. `make
tests`/`make tests-asan` pass clean (745 test cases, 2850 assertions,
unchanged from before this turn).

## Isearch-cgi/isrch_fetch.cxx

**Scope note, read first:** same `main()`-only structural limitation as
[`src/Iget.cxx`](#srcigetcxx)/[`src/zsearch.cxx`](#srczsearchcxx) (a CGI
executable this time, not a CLI tool, but the same duplicate-`main()`
problem for linking into the Catch2 binary). Verified by building the
real production binary (`make isearch-cgi`) and running it directly,
plus a standalone AddressSanitizer/UndefinedBehaviorSanitizer build
(`-fsanitize=address,undefined,pointer-overflow`) linked against the
real `libIsearch.a`.

Two bugs found; one fixed and confirmed via a real crash, one fixed
defensively despite not reproducing under any sanitizer tried:

1. **`argc < 4` check still lets `argv[4]` be read — confirmed
   SIGSEGV** (most severe finding in this file) — the usage message
   documents 4 required parameters (`<dbpath> <dbname> <key> <es>`,
   i.e. `argv[1..4]`, needing `argc >= 5`), but the guard only checked
   `argc < 4` (i.e. `argc >= 4`, guaranteeing only `argv[1..3]`).
   `ESet = argv[4];` right after runs unconditionally — when `argc ==
   4` exactly (the last parameter omitted), `argv[4]` is the C++-
   standard-guaranteed `nullptr` sentinel, dereferenced via
   `STRING::operator=(const CHR*)`'s `strlen()` call. Confirmed with
   the real binary: `isrch_fetch dbpath dbname key` (3 real arguments)
   segfaulted before this fix, printed the usage message and exited
   cleanly after. Fixed by checking `argc < 5`. See `BUGFIX #1` in
   source.
2. **`name+strlen(name)-5`/`-4` pointer arithmetic underflows for short
   filenames** — used twice (deciding whether to wrap the presented
   record in `<pre>`/`</pre>`) to check whether the document's filename
   ends in `.html`/`.htm`, with no guard on `strlen(name)` being at
   least 5 (or 4) first. `File` comes from `RsRecord.GetFileName()`
   after `pdb->KeyLookup(RecordKey, &RsRecord)`, whose result is never
   checked — an invalid/nonexistent `RecordKey` (fully attacker-
   controlled input on a CGI endpoint) leaves `File` empty, making
   `strlen(name)==0` and the computed pointers 4-5 bytes before the
   allocated buffer. Technically undefined behavior (forming an
   out-of-bounds pointer, even only for a comparison, not a
   dereference) — but it did **not** reproduce as an observable crash
   under a real invalid-key run, nor under `-fsanitize=address,
   undefined`, nor with `pointer-overflow` added explicitly: no memory
   is actually read at the bad address (only pointer *values* are
   compared), so ASan's redzone checks never trigger, and the
   arithmetic doesn't hit whatever narrower conditions
   `-fsanitize=pointer-overflow` checks for on this platform. Followed
   this project's "don't overclaim without a reproduced failure"
   discipline in reporting that honestly — but fixed anyway, since it's
   real UB per the C++ standard regardless of whether any available
   tool catches it, and the fix is cheap and safe: length-guard both
   checks, and compute the result once instead of duplicating the
   (previously fragile) pointer arithmetic at both call sites. See
   `BUGFIX #2` in source.

Also noted, not changed: `pdb` (`new IDB(...)`) and `name`
(`File.NewCString()`) are never explicitly freed before any `return`/
`exit()` path. Confirmed via a LeakSanitizer run that this is real but
inert — the process exits immediately after either way, so the OS
reclaims everything regardless; same shape as `src/zsearch.cxx`'s
identical, already-reviewed finding this batch.

No test file was written for the reasons in the scope note above; both
fixes are documented in detail here and at their `BUGFIX #n` comment
sites in source instead, plus the runtime/ASan confirmation above.
`make isearch-cgi` passes clean; `make tests`/`make tests-asan`
(unaffected by this file) still pass clean (745 test cases, 2850
assertions).

## Isearch-cgi/isrch_html.cxx

**Scope note, read first:** same `main()`-only structural limitation as
[`Isearch-cgi/isrch_fetch.cxx`](#isearch-cgiisrch_fetchcxx). At 845
lines this is the largest CGI frontend in the tree — a full HTML search
results renderer supporting SIMPLE/ADVANCED/BOOLEAN search modes built
from raw CGI form fields. Every bug below was independently confirmed
with the real production binary via `REQUEST_METHOD`/`QUERY_STRING`
environment variables (the standard CGI invocation convention), not
just inspection, including one revert-and-repro on the fix itself.

Four bugs found and fixed, all the same root defect repeated at
different sites — a CGI field that wasn't submitted (entirely ordinary:
a shorter search form, a "next page" request, "search all fields")
makes `CGIAPP::GetValueByName()` return `nullptr`
(`Isearch-cgi/cgi-util.cxx`), and something downstream dereferences it
unconditionally:

1. **`query = cgidata->GetValueByName("ISEARCH_TERM")` (two sites) —
   confirmed SIGSEGV** (most severe finding in this file, and
   trivially reachable) — assigning a possibly-`nullptr` `PCHR`
   straight into a `STRING` calls `STRING::operator=(const CHR*)`,
   which dereferences it unconditionally via `strlen()`
   (`src/string.cxx`). Two independent call sites hit this: the
   `ADVANCED` search branch, and the "next page" (`Start > 1`) refetch
   further down. Confirmed both with the real binary:
   `SEARCH_TYPE=ADVANCED` with no `ISEARCH_TERM` field, and separately
   `START=5` with no `ISEARCH_TERM` field, both segfaulted before this
   fix. Fixed by guarding both assignments with the same
   ternary-default idiom every *other* field in this function already
   uses (`DATABASE`, `START`, `MAXHITS`, etc.) — treating a missing
   field the same as an empty one. See `BUGFIX #1` in source (both
   sites).
2. **`StrCaseCmp(p, ...)`/`.Cat(p)` reused a possibly-`nullptr` `p` for
   the `BOOLEAN` search type's 2nd-and-later terms — confirmed SIGSEGV**
   — `p` is set from `GetValueByName("OPERATOR")`, but only *inside* an
   `if (p)` guard; when `OPERATOR` wasn't submitted, `p` stayed
   `nullptr` and was reused unconditionally a few lines later inside
   the term-building loop (`StrCaseCmp(p, "AND")`, etc. — `StrCaseCmp`
   calls `strcasecmp()` directly, no null tolerance, same as
   `Iget.cxx`/`zsearch.cxx`'s already-fixed argument-parsing bugs this
   batch). Confirmed with the real binary: a `BOOLEAN` search with 2
   terms and no `OPERATOR` field segfaulted before this fix. Fixed by
   defaulting `p` to `"OR"` right where it's set, matching this same
   file's own documented "otherwise is an implied OR" convention for
   the `SIMPLE` branch — `p` is now never null going into the loop. See
   `BUGFIX #2` in source.
3. **`pirset->SortByScore()` with no null check — confirmed SIGSEGV via
   revert-and-repro, the same root cause as `src/Isearch.cxx`'s
   already-fixed `BUGFIX #1`** — traced one level deeper than that
   file's own investigation: `INDEX::Search()`'s final `TempStack >>
   NewIrset;` (`src/index.cxx`) explicitly sets `NewIrset` to `nullptr`
   via `OPSTACK::Pop()`'s empty-stack branch when the `SQUERY` being
   searched has zero operands, and that `nullptr` propagates unchanged
   through `IDB::Search()`/`AndSearch()`'s passthrough
   `AfterSearching()` call — reaching `Search()`'s `pirset=pdb->Search
   (query);` here with no check before `pirset->SortByScore();` right
   after. Initially justified only by this source trace (no live CGI
   request could reach it without hitting `BUGFIX #1` first) — but
   after applying `BUGFIX #1`, the exact `Start>1`-with-no-`ISEARCH_TERM`
   request that used to crash there now reaches *this* code with a
   genuinely empty query, giving a real live repro after all. Verified
   with an isolated revert-and-repro: reverting only this fix (keeping
   `BUGFIX #1/#2/#4`) reproduced the same `SIGSEGV` on that exact
   request; restoring it produces a clean `<B>Unable to process
   query.</B>` response instead. Fixed by checking `!pirset` and
   returning early, matching `Isearch.cxx`'s own fix shape. See
   `BUGFIX #3` in source.
4. **`PrintField = field` in `get_term()` — confirmed SIGSEGV** — same
   shape as `BUGFIX #1`/`#2`: `field` comes from `get_field("FIELD_%i",
   i)`, stays `nullptr` when that field wasn't submitted (an entirely
   normal request — no field restriction means search every field),
   and was assigned straight into a `STRING` unconditionally a few
   lines later. Confirmed with the real binary: a `BOOLEAN` search with
   terms but no `FIELD_1`/`FIELD_2` fields segfaulted here specifically
   (after `BUGFIX #2` fixed the earlier `OPERATOR`-related crash on the
   same request, this is what the request hit next). Fixed with the
   same ternary-default idiom as `BUGFIX #1`. See `BUGFIX #4` in
   source.

Also modernized: all `(PCHR)NULL`/`== NULL` (17 + 4 occurrences) →
`nullptr`; all 9 `sprintf` calls in `get_field()`/`get_term()` →
`snprintf`, bounded to the same `MAXSTR+1` the destination buffers were
already allocated with (functionally safe before too — every one used
`%.128s`/`%.256s` precision limiters — but modernized regardless,
matching this project's standing convention). Confirmed zero new
warnings: diffed a standalone `-Wall -Wextra` compile against the
pre-edit committed file (100 warnings before, 100 after, all pre-
existing unused-variable/`-Wdangling-else` noise, same as
`src/Isearch.cxx`'s/`src/zsearch.cxx`'s own already-documented
vestigial-variable findings).

No test file was written for the reasons in the scope note above; all
four fixes are documented in detail here and at their `BUGFIX #n`
comment sites in source instead, verified via the real production
binary (including one full revert-and-repro) rather than inspection
alone. `make isearch-cgi` passes clean; `make tests`/`make tests-asan`
(unaffected by this file) still pass clean (745 test cases, 2850
assertions).

## Isearch-cgi/isrch_srch.cxx

**Scope note, read first:** same `main()`-only structural limitation as
[`Isearch-cgi/isrch_html.cxx`](#isearch-cgiisrch_htmlcxx) — and closely
related to it: this file's own header comment identifies it as
`isrch_html.cxx`'s ancestor ("Derived from isrch_srch.cxx"), and the
two share almost all of their structure, including four of
`isrch_html.cxx`'s four bugs verbatim. Adds JSON output support
(`OUTPUT=JSON`/`FORMAT=JSON`) that `isrch_html.cxx` doesn't have — that
code was checked separately and found sound (see below). Verified with
the real production binary via `REQUEST_METHOD`/`QUERY_STRING`, plus
`gdb` to pin down one crash whose stack trace pointed somewhere
unexpected.

Four bugs in this file itself, all identical in shape and confirmed the
same way as `isrch_html.cxx`'s own `BUGFIX #1/#2/#3/#4` (see that
file's entry for the full mechanism explanation — not repeated here):

1. **`query = cgidata->GetValueByName("ISEARCH_TERM")` (two sites,
   `ADVANCED` branch and the `Start>1` refetch) — confirmed SIGSEGV.**
   See `BUGFIX #1` in source (both sites).
2. **`StrCaseCmp(p, ...)`/`.Cat(p)` reused a possibly-null `p` for
   `BOOLEAN` search's 2nd+ terms — confirmed SIGSEGV.** Defaulted to
   `"OR"`, same as `isrch_html.cxx`. See `BUGFIX #2` in source.
3. **`pirset->SortByScore()` with no null check — confirmed SIGSEGV,
   but not the same crash `isrch_html.cxx` had.** Initially looked like
   the same fix would suffice (and it's still necessary — `pdb` here is
   a `VIDB*`, the exact class `src/Isearch.cxx`'s original `BUGFIX #1`
   covers for the "zero usable databases" case), but the *same repro*
   that crashed `isrch_html.cxx` at this point crashed here one level
   *deeper*: `gdb`'s backtrace showed `SIGSEGV` inside
   `IRSET::StoreDbNum()`, called from `VIDB::Search()` itself, called
   from this file's own `Search()` — i.e. the crash was happening
   *before* `pirset` was even assigned, inside `pdb->Search(query)`.
   Traced this to a **separate, more severe bug in the already-`done`
   `src/vidb.cxx`** (see `BUGFIX #3` newly added to
   [`src/vidb.hxx`](#srcvidbhxx)'s catalog entry for the full
   explanation and fix): `VIDB::Search()`/`AndSearch()` dereferenced
   `RsetPtrs[i]` right after `MainIndex->Search()`/`AndSearch()` with
   no null check, and that inner call can itself return `nullptr` for a
   degenerate query. Fixed `src/vidb.cxx` directly (a `.cxx` file, not
   header-frozen) as part of this turn, then added the *same*
   `!pirset`-after-`Search()` guard here too, matching
   `isrch_html.cxx`'s `BUGFIX #3` and `Isearch.cxx`'s original
   `BUGFIX #1` — both layers needed the check: this file's own guard
   for the "no databases at all" case, `vidb.cxx`'s fix for the
   "degenerate per-database query" case underneath it. Confirmed clean
   after both fixes together. See `BUGFIX #3` in source.
4. **`PrintField = field` in `get_term()` — confirmed SIGSEGV** when
   `FIELD_%i` wasn't submitted. See `BUGFIX #4` in source.

**JSON output path checked separately, found sound**: `PrintJsonEscaped()`
correctly escapes `"`, `\`, and control characters (`\uXXXX` for
anything `< 0x20`); `PrintJsonError()` and `WantsJsonOutput()` both
null-check their `GetValueByName()` results before use, unlike the four
bugs above. This looks like newer, more carefully-written code than the
rest of the file (which dates to 1996-1998) — no fixes needed here.
Also noted: this file already guards `FetchCount` against `Start >
HitCount` (`INT FetchCount = 0; if (Start <= HitCount) {...}`) — a
check `isrch_html.cxx`'s otherwise-near-identical code lacks (it could
compute a negative `FetchCount` for a stale "next page" request past
the actual result count). Not a crash in either file — the resulting
loop just doesn't execute — but worth noting as a discrepancy between
the two files' equivalent code, in case a future turn wants to
backport this file's stricter version into `isrch_html.cxx`.

Also modernized: same `(PCHR)NULL`/`== NULL` → `nullptr` (17 + 4
occurrences) and `sprintf` → `snprintf` (9 occurrences) sweep as
`isrch_html.cxx`. Confirmed zero new warnings via the same before/after
`-Wall -Wextra` diff (98 warnings, unchanged, all pre-existing unused-
variable noise).

No test file was written for the reasons in the scope note above; all
four fixes (plus the `vidb.cxx` fix they exposed) are documented in
detail here and at their `BUGFIX #n` comment sites in source instead,
verified via the real production binary (including `gdb` for the one
non-obvious crash). `make isearch`/`make isearch-cgi`/`make smoke-test`
all pass clean; `make tests`/`make tests-asan` (which *do* link
`src/vidb.cxx`, unlike `isrch_srch.cxx` itself) still pass clean (745
test cases, 2850 assertions, unchanged — the `vidb.cxx` fix doesn't
touch any code path the existing suite exercises).

## Isearch-cgi/search_form.cxx

**Scope note, read first:** same `main()`-only structural limitation as
[`Isearch-cgi/isrch_fetch.cxx`](#isearch-cgiisrch_fetchcxx) — a smaller
utility (313 lines) that generates the HTML search form the other
three CGI frontends' output feeds into, invoked either as `search_form
<dbpath> <dbname> [www]` or `search_form -pagetype <dbpath> <dbname>
[www]`. Both bugs confirmed with the real production binary.

Two bugs found and fixed:

1. **`strcpy(temp,argv[1])` into a fixed 64-byte buffer — confirmed
   stack buffer overflow** (most severe finding in this file) — `argv[1]`
   is a short flag (`-simple`, etc.) in the `-pagetype` form, but it's
   the **dbpath itself** in the plain form — an arbitrary-length
   filesystem path with no reason to be under 64 characters. Confirmed
   with the real binary: a >64-character dbpath triggered glibc's
   `*** buffer overflow detected ***` (`SIGABRT`) before this fix. The
   copy turned out to be entirely unnecessary — `argv[1]` was only ever
   read (via `temp[0]`, `strcmp(temp, ...)`), never mutated, in this
   scope — so the fix removes the copy altogether and checks/compares
   `argv[1]` directly instead of copying it into any buffer, small or
   otherwise. See `BUGFIX #2` in source (numbered after `BUGFIX #1`
   since the overflow's root cause — the unnecessary copy — is what the
   `BUGFIX #1` code sits inside of).
2. **`argc < 3` doesn't cover the `-pagetype` form's real 4-argument
   requirement — confirmed SIGSEGV, plus a true out-of-bounds
   argv[] read** — the plain form needs `argc >= 3` (`dbpath`,
   `dbname`), but the `-pagetype` form needs `argc >= 4` (`flag`,
   `dbpath`, `dbname`) — checked once, unconditionally, before either
   branch even runs. When `argc == 3` in the flag form (e.g.
   `search_form -simple /onepath`), `db = argv[3]` reads the C++-
   standard-guaranteed `nullptr` sentinel at `argv[argc]` (harmless on
   its own — `db` turned out to be dead, assigned but never read
   afterward, confirmed via `-Wunused-but-set-variable`, left alone
   matching this project's established vestigial-variable precedent) —
   but `www = argv[4]` read **one past** that guaranteed range
   entirely, genuine undefined behavior, not just a null read. Further
   down, `DBRootName = argv[3];` (a real `STRING` assignment) crashed
   via `strlen(nullptr)`. Confirmed with the real binary: `search_form
   -simple /tmp` (`argc==3`) segfaulted before this fix. Fixed by
   moving the argument-count check into the branch that actually knows
   its own requirement (`argc < 4` inside the flag-form branch, after
   the flag itself is already known), and bounds-checking the optional
   `www` argument in both branches (`(argc > N) ? argv[N] : nullptr`)
   instead of reading it unconditionally. See `BUGFIX #1` in source.

Also modernized: the 6 `sprintf(temp, "..._%%i", i)` calls (building
`FIELD_N`/`TERM_N`/`WEIGHT_N` form field names) → `snprintf`. Confirmed
zero new warnings via the same before/after `-Wall -Wextra` diff
technique (87 warnings, unchanged — the 2 warnings on this file's own
lines, `db`/`dbpath` "set but not used", are pre-existing and already
explained above under `BUGFIX #1`).

No test file was written for the reasons in the scope note above; both
fixes are documented in detail here and at their `BUGFIX #n` comment
sites in source instead, verified via the real production binary
(including under `-fsanitize=address,undefined` for the fixed
`www`/`argc` handling). `make isearch-cgi` passes clean; `make
tests`/`make tests-asan` (unaffected by this file) still pass clean
(745 test cases, 2850 assertions).

## Isearch-cgi/api_config.hxx, Isearch-cgi/api_config.cxx

**Processed together**: `api_config.hxx` declares `struct ApiConfig`
(server-side configuration for the JSON API frontend, `isrch_api.cxx` —
still `pending`) and `LoadApiConfig()`, populated entirely from
environment variables (`ISEARCH_DB_PATH`, `ISEARCH_API_MAX_HITS`,
`ISEARCH_API_ALLOWLIST`) by the `.cxx`'s two `static` helper functions.
Confirmed self-contained (compiles clean as the sole `#include`).
Unlike every CGI file processed earlier this batch, this one is *not*
`main()`-only — `ApiConfig`/`LoadApiConfig()` are ordinary, directly
testable code — so this is also the first CGI-directory file this
batch to get a real Catch2 test file (`tests/Isearch-cgi/
test_api_config.cxx`, wired into `TEST_ENGINE_CGI_SRCS` in the
top-level `Makefile`). Much newer, more carefully written code than
the four `main()`-only files processed earlier in this batch (all of
which trace to 1996-1998; this one reads like it was written
specifically for the JSON API addition) — every `getenv()` result is
already null-checked before use, `strtol()`'s parse success is already
validated. Still found two real bugs, both confirmed via the new test
suite, including one full revert-and-repro:

1. **`strtol()`'s `long` result cast straight to `INT` with no range
   check — confirmed silent truncation, not a crash** —
   `ReadPositiveIntEnv()` validates that `strtol()` consumed the whole
   string and produced a positive value, but never checked it against
   `INT_MAX` before `static_cast<INT>(parsed)`. Confirmed with a
   standalone repro before writing the permanent test:
   `ISEARCH_API_MAX_HITS=99999999999` (a plausible admin typo — one
   digit short of a `long` overflow, not some exotic edge case)
   produced `max_hits_ceiling=1215752191`, an arbitrary truncated value
   with no relationship to the intended cap, not the sane maximum a
   typo like this should fall back to. Real severity is low — this
   environment variable is server-side configuration, not remote-
   request input — but the fix is cheap and safe: clamp to `INT_MAX`
   instead of truncating. See `BUGFIX #1` in source.
2. **`ParseAllowList()`'s `entry.Trim()` only strips *trailing*
   whitespace — confirmed via the new test suite, including a full
   revert-and-repro** — `STRING::Trim()` (`src/string.cxx`) only walks
   back from the end of the string; leading whitespace needs the
   separate `TrimLeading()` call, the same two-call convention already
   established elsewhere in this tree (e.g. `src/thesaurus.cxx`).
   `ParseAllowList()` only called `Trim()`, so every entry after the
   first in an ordinarily-formatted `"alpha, beta, gamma"` allowlist
   (spaces after each comma — the overwhelmingly natural way to write
   one) kept its leading space, silently corrupting the allowlist for
   every entry but the first: whatever downstream code checks a
   request's database name against this list would never match `"
   beta"`/`" gamma"` against a clean `"beta"`/`"gamma"`. Confirmed with
   the new Catch2 test (`ApiConfig splits, trims, and skips empty
   entries in ISEARCH_API_ALLOWLIST`) — failed before the fix (entries
   came back as `" beta"`/`" gamma"`), and confirmed the test is a real
   regression guard via revert-and-repro: temporarily removed the added
   `TrimLeading()` call and reran `make tests`, watched the exact same
   test fail again, then restored the fix. See `BUGFIX #2` in source.

Also modernized: the 4 code-level `NULL` uses converted to `nullptr`.

`tests/Isearch-cgi/test_api_config.cxx` (new) covers: defaults with no
environment variables set; `ISEARCH_DB_PATH` picked up correctly; a
valid `ISEARCH_API_MAX_HITS`; fallback to the default for a non-numeric
or non-positive value; the `INT_MAX`-clamp confirming `BUGFIX #1`;
comma-splitting/trimming/empty-entry-skipping for
`ISEARCH_API_ALLOWLIST` confirming `BUGFIX #2`; an empty allowlist when
unset; and a sanity check on the three exported constants. `make
tests`/`make tests-asan` pass clean (754 test cases, 2866 assertions —
up from 745/2850, the 16-assertion increase being this new file's own
coverage). `make isearch-cgi` also confirmed to still build clean
(`api_config.cxx` links into `isrch_api`).

