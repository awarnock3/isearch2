/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1994. 

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby granted
without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact. 

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of noteworthy
uses of this software. 

3. The names of MCNC and Clearinghouse for Networked Information Discovery
and Retrieval may not be used in any advertising or publicity relating to
the software without the specific, prior written permission of MCNC/CNIDR. 

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. 

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE
POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
************************************************************************/

/*@@@
File:		soundex.cxx
Version:	1.00
Description:	Soundex support for STRING class
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "soundex.hxx"

// The classic Soundex letter-to-digit map: vowels/H/W/Y/space carry no
// digit ('0'); everything else not in the map (e.g. embedded
// punctuation or digits) passes through unchanged, matching the
// original behavior for non-letter input.
static CHR
SoundexDigit(CHR ch) {
	switch (ch) {
		case ' ':
		case 'A':
		case 'E':
		case 'H':
		case 'I':
		case 'O':
		case 'U':
		case 'W':
		case 'Y':
			return '0';
		case 'B':
		case 'F':
		case 'P':
		case 'V':
			return '1';
		case 'C':
		case 'G':
		case 'J':
		case 'K':
		case 'Q':
		case 'S':
		case 'X':
		case 'Z':
			return '2';
		case 'D':
		case 'T':
			return '3';
		case 'L':
			return '4';
		case 'M':
		case 'N':
			return '5';
		case 'R':
			return '6';
		default:
			return ch;
	}
}

void SoundexEncode(const STRING& EnglishWord, PSTRING StringBuffer) {
	STRING s1, s2;
	s1 = EnglishWord;
	s1.UpperCase();
	INT x, y;
	y = s1.GetLength();

	if (y == 0) {
		*StringBuffer = "";
		return;
	}

	// BUGFIX #1: the old two-pass approach (strip every '0' first, then
	// collapse adjacent duplicates) got the classic Soundex edge cases
	// wrong two different ways: (1) it never compared the kept first
	// letter against the next letter's own code, so "PFister" -- where P
	// and F share the same digit -- kept both instead of collapsing them
	// ("P123" instead of the correct "P236"); (2) stripping zeros before
	// deduplicating merges same-digit letters that were originally
	// separated by a vowel into a false adjacency, undercounting them
	// ("HONEYMAN" -- N, then M, then N again, each separated by a vowel
	// -- came out "H500" instead of the correct "H555", and "TYMCZAK"
	// came out "T520" instead of "T522"). Confirmed against reference
	// Soundex test vectors (Pfister/Tymczak/Honeyman, all textbook edge
	// cases) via a standalone repro before this fix; Robert/Rupert
	// (no first-letter collision) already matched and still do.
	//
	// Fixed with a single pass that tracks the previous letter's own
	// digit (seeded from the first letter's digit, even though the
	// first letter itself is kept literally) and only appends a new
	// digit when it's non-zero and differs from that running previous
	// digit -- the textbook algorithm, applied uniformly instead of as
	// two separate strip/collapse passes.
	CHR firstLetter = s1.GetChr(1);
	s2 += firstLetter;
	CHR prevDigit = SoundexDigit(firstLetter);

	for (x=2; x<=y; x++) {
		CHR digit = SoundexDigit(s1.GetChr(x));
		if (digit != '0' && digit != prevDigit) {
			s2 += digit;
		}
		prevDigit = digit;
	}

	if ( (y=s2.GetLength()) > 4 ) {
		s2.EraseAfter(4);
	} else {
		y = 4 - y;
		for (x=1; x<=y; x++) {
			s2 += '0';
		}
	}
	*StringBuffer = s2;
}
