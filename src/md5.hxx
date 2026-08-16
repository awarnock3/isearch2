// Classic MD5 message-digest implementation (RFC 1321), public domain,
// written by Colin Plumb in 1993. See src/md5.cxx for the algorithm
// itself; src/md5sum.cxx is this tree's only current caller.
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef MD5_H
#define MD5_H

#include <stdint.h>

// BUGFIX #1: uint32 was typedef'd from `unsigned long`, which is 64
// bits wide on any LP64 platform (every 64-bit Unix/Linux target this
// tree builds on today) -- not actually 32 bits as MD5's bit-rotation
// arithmetic and MD5Transform's `(uint32*)ctx->in` 16-word cast both
// require. The __alpha special case was a 1990s-era guess at which
// platforms had a 32-bit long; it's no longer a reliable signal on
// modern 64-bit systems, Alpha or otherwise. Confirmed real: on this
// platform sizeof(uint32) was 8, and a standalone repro of
// MD5Init/MD5Update/MD5Final crashed under ASan with a stack-buffer-
// overflow in MD5Transform (the 16-word cast over ctx->in reads/writes
// 128 bytes into a 64-byte buffer once each "word" is 8 bytes instead
// of 4). Fixed by using the real fixed-width type. See
// docs/BUG_CATALOG.md#srcmd5hxx.
typedef uint32_t uint32;

#ifdef __cplusplus
extern "C" {
#endif

struct MD5Context {
	uint32 buf[4];
	uint32 bits[2];
	unsigned char in[64];
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, unsigned char const *buf,
	       unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(uint32 buf[4], uint32 const in[16]);

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#ifdef __cplusplus
}
#endif

#endif /* !MD5_H */
