// Tests for src/md5.hxx / src/md5.cxx (MD5 message-digest algorithm,
// RFC 1321). Linked against the real implementation via
// TEST_ENGINE_SRCS in the top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "md5.hxx"

#include <cstring>
#include <cstdio>
#include <string>

namespace {

std::string Digest(const char* input) {
	struct MD5Context ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char const*)input, (unsigned)strlen(input));
	unsigned char digest[16];
	MD5Final(digest, &ctx);
	char hex[33];
	for (int i = 0; i < 16; i++)
		snprintf(hex + i * 2, 3, "%02x", digest[i]);
	return std::string(hex, 32);
}

} // namespace

TEST_CASE("md5.hxx uses a genuinely 32-bit uint32", "[md5]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcmd5hxx): uint32 was
	// typedef'd from `unsigned long`, 64 bits wide on this (LP64)
	// platform. That's the regression this guards against directly.
	REQUIRE(sizeof(uint32) == 4);
	REQUIRE(sizeof(struct MD5Context) == 88);
}

TEST_CASE("MD5 matches RFC 1321 test vectors", "[md5]") {
	// BUGFIX #1 coverage: before the fix, the oversized uint32 broke
	// both MD5's bit-rotation math and MD5Transform's 16-word cast over
	// ctx->in, producing wrong digests (when it didn't crash outright,
	// confirmed separately under ASan with a stack-buffer-overflow).
	REQUIRE(Digest("") == "d41d8cd98f00b204e9800998ecf8427e");
	REQUIRE(Digest("a") == "0cc175b9c0f1b6a831c399e269772661");
	REQUIRE(Digest("abc") == "900150983cd24fb0d6963f7d28e17f72");
	REQUIRE(Digest("message digest") == "f96b697d7cb7938d525a2f31aaf161d0");
	REQUIRE(Digest("abcdefghijklmnopqrstuvwxyz") == "c3fcd3d76192e4007dfb496cca67e13b");
	REQUIRE(Digest("The quick brown fox jumps over the lazy dog") == "9e107d9d372bb6826bd81d3542a419d6");
}

TEST_CASE("MD5 handles input spanning multiple 64-byte blocks", "[md5]") {
	REQUIRE(Digest("12345678901234567890123456789012345678901234567890123456789012345678901234567890")
		== "57edf4a22be3c955ac49da2e2107b67a");
}

TEST_CASE("MD5Update can be called incrementally across chunks", "[md5]") {
	struct MD5Context ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char const*)"abc", 3);
	MD5Update(&ctx, (unsigned char const*)"def", 3);
	unsigned char digest[16];
	MD5Final(digest, &ctx);
	char hex[33];
	for (int i = 0; i < 16; i++)
		snprintf(hex + i * 2, 3, "%02x", digest[i]);
	REQUIRE(std::string(hex, 32) == Digest("abcdef"));
}

TEST_CASE("MD5Final scrubs the whole context, not just the pointer's own size", "[md5]") {
	// BUGFIX #2 coverage: before the fix, memset(ctx, 0, sizeof(ctx))
	// cleared only 8 bytes (sizeof of the pointer itself) of the
	// 88-byte context. Confirmed originally via a standalone repro
	// memcmp-ing the whole context against a zeroed buffer.
	struct MD5Context ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char const*)"hello", 5);
	unsigned char digest[16];
	MD5Final(digest, &ctx);

	unsigned char zero[sizeof(ctx)];
	memset(zero, 0, sizeof(zero));
	REQUIRE(memcmp(&ctx, zero, sizeof(ctx)) == 0);
}
