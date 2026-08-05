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
