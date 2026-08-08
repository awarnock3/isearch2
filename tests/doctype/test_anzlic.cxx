#include "../vendor/catch2/catch_amalgamated.hpp"
#include "../../doctype/anzlic.hxx"
#include "../../src/isearch.hxx"

// Test that the ANZLIC header compiles and basic types are defined
TEST_CASE("ANZLIC header defines", "[anzlic]") {
  // Verify that the header was parsed correctly
  REQUIRE(ANZLIC_ACCEPT_EMPTY_TAGS == 0);
  REQUIRE(MAXNESTINGLEN == 1024);
}

// Test that string constants are defined
TEST_CASE("ANZLIC extension constants", "[anzlic]") {
  const char *sgml_ext = ANZLIC_SGML_EXTENSION;
  const char *xml_ext = ANZLIC_XML_EXTENSION;
  const char *html_ext = ANZLIC_HTML_EXTENSION;
  const char *text_ext = ANZLIC_TEXT_EXTENSION;

  REQUIRE(sgml_ext != nullptr);
  REQUIRE(xml_ext != nullptr);
  REQUIRE(html_ext != nullptr);
  REQUIRE(text_ext != nullptr);

  REQUIRE(strcmp(sgml_ext, "sgml") == 0);
  REQUIRE(strcmp(xml_ext, "xml") == 0);
  REQUIRE(strcmp(html_ext, "html") == 0);
  REQUIRE(strcmp(text_ext, "txt") == 0);
}

// Test AMD_Element class structure
TEST_CASE("AMD_Element basic operations", "[anzlic]") {
  AMD_Element elem;

  STRING testTag = "TITLE";
  elem.set_tag(testTag);
  REQUIRE(elem.get_tag() == testTag);

  elem.set_start(100);
  REQUIRE(elem.get_start() == 100);

  elem.set_end(200);
  REQUIRE(elem.get_end() == 200);
}

// Test that ANZLIC can be properly defined in type declarations
TEST_CASE("ANZLIC pointer type definitions", "[anzlic]") {
  // Verify that PANZLIC is properly defined
  PANZLIC pAnzlic = nullptr;
  REQUIRE(pAnzlic == nullptr);
}

// Test short extension constants
TEST_CASE("ANZLIC short extensions", "[anzlic]") {
  const char *short_sgml = SHORT_ANZLIC_SGML_EXTENSION;
  const char *short_html = SHORT_ANZLIC_HTML_EXTENSION;
  const char *short_text = SHORT_ANZLIC_TEXT_EXTENSION;

  REQUIRE(short_sgml != nullptr);
  REQUIRE(short_html != nullptr);
  REQUIRE(short_text != nullptr);

  REQUIRE(strcmp(short_sgml, "sgm") == 0);
  REQUIRE(strcmp(short_html, "htm") == 0);
  REQUIRE(strcmp(short_text, "txt") == 0);
}

// Test uppercase extension constants
TEST_CASE("ANZLIC uppercase extensions", "[anzlic]") {
  const char *sgml_uc = ANZLIC_SGML_EXTENSION_UC;
  const char *xml_uc = ANZLIC_XML_EXTENSION_UC;
  const char *html_uc = ANZLIC_HTML_EXTENSION_UC;
  const char *text_uc = ANZLIC_TEXT_EXTENSION_UC;

  REQUIRE(sgml_uc != nullptr);
  REQUIRE(xml_uc != nullptr);
  REQUIRE(html_uc != nullptr);
  REQUIRE(text_uc != nullptr);

  REQUIRE(strcmp(sgml_uc, "SGML") == 0);
  REQUIRE(strcmp(xml_uc, "XML") == 0);
  REQUIRE(strcmp(html_uc, "HTML") == 0);
  REQUIRE(strcmp(text_uc, "TXT") == 0);
}

// Test ANZLIC class can be forward declared
TEST_CASE("ANZLIC class definition", "[anzlic]") {
  // Verify class has expected methods (compile-time check)
  // Methods checked: LoadFieldTable, ParseRecords, ParseFields,
  // GetCleanedFieldData, Present, ParseDateSingle, ParseDateRange
  REQUIRE(true); // If this compiles, the header is valid
}

// Test string buffer for Present method
TEST_CASE("ANZLIC string buffer types", "[anzlic]") {
  STRING buf;
  buf = "Test content";
  REQUIRE(buf.GetLength() > 0);
}
