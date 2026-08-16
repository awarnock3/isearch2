// Tests for src/md5sum.cxx (standalone md5sum CLI reference
// implementation). Linked against the real implementation via
// TEST_ENGINE_SRCS in the top-level Makefile, not reimplemented here.
//
// Note: main() is #if 0'd out in the source itself -- this file has
// never actually been wired into any build target (not in the
// Makefile, no other file calls into it). It's exercised here purely
// at the function level, the same way any other cataloged file is.

#include "catch_amalgamated.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

int hex_digit(int c);
int get_md5_line(FILE *fp, unsigned char *digest, char *file);
int mdfile(FILE *fp, unsigned char *digest);
int do_check(FILE *chkf);
void print_digest(unsigned char *p);

extern int verbose;
extern int bin_mode;
extern char *progname;

TEST_CASE("hex_digit converts 0-9/a-f and rejects everything else", "[md5sum]") {
	REQUIRE(hex_digit('0') == 0);
	REQUIRE(hex_digit('9') == 9);
	REQUIRE(hex_digit('a') == 10);
	REQUIRE(hex_digit('f') == 15);
	REQUIRE(hex_digit('A') == -1); // uppercase deliberately not accepted
	REQUIRE(hex_digit('g') == -1);
	REQUIRE(hex_digit(' ') == -1);
}

TEST_CASE("mdfile computes the RFC 1321 digest of a file's contents", "[md5sum]") {
	char tmpl[] = "/tmp/isearch2_test_md5sum_mdfile_XXXXXX";
	int fd = mkstemp(tmpl);
	REQUIRE(fd != -1);
	FILE *fp = fdopen(fd, "w+");
	REQUIRE(fp != nullptr);
	fputs("abc", fp);
	rewind(fp);

	unsigned char digest[16];
	REQUIRE(mdfile(fp, digest) == 0);

	char hex[33];
	for (int i = 0; i < 16; i++) {
		snprintf(hex + i * 2, 3, "%02x", digest[i]);
	}
	REQUIRE(std::string(hex) == "900150983cd24fb0d6963f7d28e17f72");

	fclose(fp);
	remove(tmpl);
}

namespace {
char* WriteTempFile(const char* prefix, const char* contents) {
	static char tmpl[256];
	snprintf(tmpl, sizeof(tmpl), "/tmp/isearch2_test_md5sum_%s_XXXXXX", prefix);
	int fd = mkstemp(tmpl);
	REQUIRE(fd != -1);
	FILE* fp = fdopen(fd, "w");
	REQUIRE(fp != nullptr);
	fputs(contents, fp);
	fclose(fp);
	return tmpl;
}
} // namespace

TEST_CASE("get_md5_line parses a well-formed checksum line", "[md5sum]") {
	char* path = WriteTempFile("line", "900150983cd24fb0d6963f7d28e17f72  hello.txt\n");
	FILE* fp = fopen(path, "r");
	REQUIRE(fp != nullptr);

	unsigned char digest[16];
	char filename[256];
	int rc = get_md5_line(fp, digest, filename);
	REQUIRE(rc == 1); // text-mode attribute char
	REQUIRE(std::string(filename) == "hello.txt");
	REQUIRE(digest[0] == 0x90);
	REQUIRE(digest[15] == 0x72);

	fclose(fp);
	remove(path);
}

TEST_CASE("get_md5_line rejects a line that isn't a checksum line", "[md5sum]") {
	char* path = WriteTempFile("badline", "not a checksum line at all\n");
	FILE* fp = fopen(path, "r");
	REQUIRE(fp != nullptr);

	unsigned char digest[16];
	char filename[256];
	REQUIRE(get_md5_line(fp, digest, filename) == 0);

	fclose(fp);
	remove(path);
}

TEST_CASE("get_md5_line returns -1 at end of file", "[md5sum]") {
	char* path = WriteTempFile("empty", "");
	FILE* fp = fopen(path, "r");
	REQUIRE(fp != nullptr);

	unsigned char digest[16];
	char filename[256];
	REQUIRE(get_md5_line(fp, digest, filename) == -1);

	fclose(fp);
	remove(path);
}

TEST_CASE("do_check verifies a matching file against its checksum line", "[md5sum]") {
	char* datapath = WriteTempFile("data", "abc");
	char line[512];
	snprintf(line, sizeof(line), "900150983cd24fb0d6963f7d28e17f72  %s\n", datapath);
	char* chkpath = WriteTempFile("chk", line);

	FILE* chkf = fopen(chkpath, "r");
	REQUIRE(chkf != nullptr);
	int saved_verbose = verbose;
	verbose = 0;
	REQUIRE(do_check(chkf) == 0);
	verbose = saved_verbose;

	fclose(chkf);
	remove(datapath);
	remove(chkpath);
}

TEST_CASE("do_check reports a mismatch against a tampered checksum", "[md5sum]") {
	char* datapath = WriteTempFile("data2", "abc");
	char line[512];
	snprintf(line, sizeof(line), "00000000000000000000000000000000  %s\n", datapath);
	char* chkpath = WriteTempFile("chk2", line);

	FILE* chkf = fopen(chkpath, "r");
	REQUIRE(chkf != nullptr);
	int saved_verbose = verbose;
	char* saved_progname = progname;
	verbose = 0;
	progname = const_cast<char*>("test_md5sum");
	REQUIRE(do_check(chkf) == 1);
	verbose = saved_verbose;
	progname = saved_progname;

	fclose(chkf);
	remove(datapath);
	remove(chkpath);
}
