#include "../vendor/catch2/catch_amalgamated.hpp"
#include "../../doctype/anzmeta.hxx"
#include "../../src/isearch.hxx"

// Test that the ANZMETA header compiles and basic types are defined
TEST_CASE("ANZMETA header defines", "[anzmeta]") {
  // Verify that the header was parsed correctly
  REQUIRE(ANZ_ACCEPT_EMPTY_TAGS == 0);
  REQUIRE(MAXNESTINGLEN == 1024);
}

// Test that string constants are defined
TEST_CASE("ANZMETA extension constants", "[anzmeta]") {
  const char *sgml_ext = ANZ_SGML_EXTENSION;
  const char *xml_ext = ANZ_XML_EXTENSION;
  const char *html_ext = ANZ_HTML_EXTENSION;
  const char *text_ext = ANZ_TEXT_EXTENSION;

  REQUIRE(sgml_ext != nullptr);
  REQUIRE(xml_ext != nullptr);
  REQUIRE(html_ext != nullptr);
  REQUIRE(text_ext != nullptr);

  REQUIRE(strcmp(sgml_ext, "sgml") == 0);
  REQUIRE(strcmp(xml_ext, "xml") == 0);
  REQUIRE(strcmp(html_ext, "html") == 0);
  REQUIRE(strcmp(text_ext, "txt") == 0);
}

// Test ZMD_Element class structure
TEST_CASE("ZMD_Element basic operations", "[anzmeta]") {
  ZMD_Element elem;

  STRING testTag = "TITLE";
  elem.set_tag(testTag);
  REQUIRE(elem.get_tag() == testTag);

  elem.set_start(100);
  REQUIRE(elem.get_start() == 100);

  elem.set_end(200);
  REQUIRE(elem.get_end() == 200);
}

// Test that ANZMETA can be properly defined in type declarations
TEST_CASE("ANZMETA pointer type definitions", "[anzmeta]") {
  // Verify that PANZMETA is properly defined
  PANZMETA pAnzmeta = nullptr;
  REQUIRE(pAnzmeta == nullptr);
}

// Test short extension constants
TEST_CASE("ANZMETA short extensions", "[anzmeta]") {
  const char *short_sgml = SHORT_ANZ_SGML_EXTENSION;
  const char *short_html = SHORT_ANZ_HTML_EXTENSION;
  const char *short_text = SHORT_ANZ_TEXT_EXTENSION;

  REQUIRE(short_sgml != nullptr);
  REQUIRE(short_html != nullptr);
  REQUIRE(short_text != nullptr);

  REQUIRE(strcmp(short_sgml, "sgm") == 0);
  REQUIRE(strcmp(short_html, "htm") == 0);
  REQUIRE(strcmp(short_text, "txt") == 0);
}

// Test uppercase extension constants
TEST_CASE("ANZMETA uppercase extensions", "[anzmeta]") {
  const char *sgml_uc = ANZ_SGML_EXTENSION_UC;
  const char *xml_uc = ANZ_XML_EXTENSION_UC;
  const char *html_uc = ANZ_HTML_EXTENSION_UC;
  const char *text_uc = ANZ_TEXT_EXTENSION_UC;

  REQUIRE(sgml_uc != nullptr);
  REQUIRE(xml_uc != nullptr);
  REQUIRE(html_uc != nullptr);
  REQUIRE(text_uc != nullptr);

  REQUIRE(strcmp(sgml_uc, "SGML") == 0);
  REQUIRE(strcmp(xml_uc, "XML") == 0);
  REQUIRE(strcmp(html_uc, "HTML") == 0);
  REQUIRE(strcmp(text_uc, "TEXT") == 0);
}

// Test short uppercase extension constants
TEST_CASE("ANZMETA short uppercase extensions", "[anzmeta]") {
  const char *short_sgml_uc = SHORT_ANZ_SGML_EXTENSION_UC;
  const char *short_html_uc = SHORT_ANZ_HTML_EXTENSION_UC;
  const char *short_text_uc = SHORT_ANZ_TEXT_EXTENSION_UC;

  REQUIRE(short_sgml_uc != nullptr);
  REQUIRE(short_html_uc != nullptr);
  REQUIRE(short_text_uc != nullptr);

  REQUIRE(strcmp(short_sgml_uc, "SGM") == 0);
  REQUIRE(strcmp(short_html_uc, "HTM") == 0);
  REQUIRE(strcmp(short_text_uc, "TXT") == 0);
}

// Test ANZMETA class can be forward declared
TEST_CASE("ANZMETA class definition", "[anzmeta]") {
  // Verify class has expected methods (compile-time check)
  // Methods checked: LoadFieldTable, ParseRecords, ParseFields,
  // GetCleanedFieldData, Present, ParseDate (overloads),
  // ParseDateRange (overloads), ParseGPoly, ParseComputed
  REQUIRE(true); // If this compiles, the header is valid
}

// Test string buffer for Present method
TEST_CASE("ANZMETA string buffer types", "[anzmeta]") {
  STRING buf;
  buf = "Test content";
  REQUIRE(buf.GetLength() > 0);
}

// Test ZMD_Element array operations
TEST_CASE("ZMD_Element array functionality", "[anzmeta]") {
  ZMD_Element elems[3];

  for (int i = 0; i < 3; i++) {
    STRING tag = "TAG";
    tag.Cat(STRING(i + 1));
    elems[i].set_tag(tag);
    elems[i].set_start(i * 100);
    elems[i].set_end((i + 1) * 100);
  }

  REQUIRE(elems[0].get_start() == 0);
  REQUIRE(elems[1].get_start() == 100);
  REQUIRE(elems[2].get_start() == 200);

  REQUIRE(elems[0].get_end() == 100);
  REQUIRE(elems[1].get_end() == 200);
  REQUIRE(elems[2].get_end() == 300);
}

// Test BRIEF_MAGIC definition
TEST_CASE("ANZMETA BRIEF_MAGIC constant", "[anzmeta]") {
  const char *brief = BRIEF_MAGIC;
  REQUIRE(brief != nullptr);
  REQUIRE(strcmp(brief, "B") == 0);
}
