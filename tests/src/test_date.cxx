// Tests for src/date.hxx / src/date.cxx (classes SRCH_DATE and
// DATERANGE).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "date.hxx"

TEST_CASE("SRCH_DATE default-constructs to an invalid/error state", "[date]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcdatecxx): d_date/d_prec used
	// to be indeterminate.
	SRCH_DATE d;
	REQUIRE(d.GetValue() == DATE_ERROR);
	REQUIRE(d.GetPrecision() == BAD_DATE);
	REQUIRE(d.IsValidDate() == GDT_FALSE);
}

TEST_CASE("SRCH_DATE precision is derived from digit count", "[date]") {
	SRCH_DATE year(2026.0);
	SRCH_DATE month(202608.0);
	SRCH_DATE day(20260807.0);
	REQUIRE(year.GetPrecision() == YEAR_PREC);
	REQUIRE(month.GetPrecision() == MONTH_PREC);
	REQUIRE(day.GetPrecision() == DAY_PREC);
}

TEST_CASE("SRCH_DATE TrimToMonth/TrimToYear reduce precision", "[date]") {
	SRCH_DATE day(20260807.0);
	REQUIRE(day.TrimToMonth() == GDT_TRUE);
	REQUIRE(day.GetPrecision() == MONTH_PREC);
	REQUIRE(day.GetValue() == 202608.0);

	SRCH_DATE day2(20260807.0);
	REQUIRE(day2.TrimToYear() == GDT_TRUE);
	REQUIRE(day2.GetPrecision() == YEAR_PREC);
	REQUIRE(day2.GetValue() == 2026.0);
}

TEST_CASE("SRCH_DATE PromoteToDayStart/End fill in month and day", "[date]") {
	SRCH_DATE year(2026.0);
	REQUIRE(year.PromoteToDayStart() == GDT_TRUE);
	REQUIRE(year.GetValue() == 20260101.0);

	SRCH_DATE year2(2026.0);
	REQUIRE(year2.PromoteToDayEnd() == GDT_TRUE);
	REQUIRE(year2.GetValue() == 20261231.0);
}

TEST_CASE("SRCH_DATE IsBefore/Equals/IsAfter compare at matching precision", "[date]") {
	SRCH_DATE a(2020.0);
	SRCH_DATE b(2025.0);
	REQUIRE(a.IsBefore(b) == GDT_TRUE);
	REQUIRE(b.IsAfter(a) == GDT_TRUE);
	REQUIRE(a.Equals(a) == GDT_TRUE);
}

TEST_CASE("SRCH_DATE comparisons trim the more precise date first", "[date]") {
	// A day-precision date compared against a year-precision date should
	// compare at year precision.
	SRCH_DATE day(20260315.0);
	SRCH_DATE year(2026.0);
	REQUIRE(day.Equals(year) == GDT_TRUE);
}

TEST_CASE("SRCH_DATE GetTodaysDate produces a valid day-precision date", "[date]") {
	SRCH_DATE d;
	d.GetTodaysDate();
	REQUIRE(d.IsValidDate() == GDT_TRUE);
	REQUIRE(d.GetPrecision() == DAY_PREC);
}

TEST_CASE("SRCH_DATE operator= copies value and precision", "[date]") {
	SRCH_DATE a(20260807.0);
	SRCH_DATE b;
	b = a;
	REQUIRE(b.GetValue() == 20260807.0);
	REQUIRE(b.GetPrecision() == DAY_PREC);
}

TEST_CASE("DATERANGE default-constructs with both ends invalid", "[date]") {
	DATERANGE r;
	REQUIRE(r.GetStart().IsValidDate() == GDT_FALSE);
	REQUIRE(r.GetEnd().IsValidDate() == GDT_FALSE);
}

TEST_CASE("DATERANGE single-date constructor sets both start and end", "[date]") {
	DATERANGE r(SRCH_DATE(2026.0));
	REQUIRE(r.GetStart().GetValue() == 2026.0);
	REQUIRE(r.GetEnd().GetValue() == 2026.0);
}

TEST_CASE("DATERANGE string constructor splits on space", "[date]") {
	DATERANGE r(STRING("2020 2025"));
	REQUIRE(r.GetStart().GetValue() == 2020.0);
	REQUIRE(r.GetEnd().GetValue() == 2025.0);
}

TEST_CASE("DATERANGE string constructor splits on slash when no space", "[date]") {
	DATERANGE r(STRING("2020/2025"));
	REQUIRE(r.GetStart().GetValue() == 2020.0);
	REQUIRE(r.GetEnd().GetValue() == 2025.0);
}

TEST_CASE("DATERANGE string constructor sets an error range with no delimiter", "[date]") {
	DATERANGE r(STRING("notarange"));
	REQUIRE(r.GetStart().IsValidDate() == GDT_FALSE);
	REQUIRE(r.GetEnd().IsValidDate() == GDT_FALSE);
}

TEST_CASE("DATERANGE Contains is true for a date inside the range", "[date]") {
	// BUGFIX #3 (see docs/BUG_CATALOG.md#srcdatecxx): Contains() used to
	// have its BEFORE/AFTER comparison backwards, so it returned
	// GDT_FALSE for almost every TestDate, including ones squarely
	// inside the range.
	DATERANGE r(SRCH_DATE(2020.0), SRCH_DATE(2025.0));
	REQUIRE(r.Contains(SRCH_DATE(2022.0)) == GDT_TRUE);
}

TEST_CASE("DATERANGE Contains is false for a date before or after the range", "[date]") {
	DATERANGE r(SRCH_DATE(2020.0), SRCH_DATE(2025.0));
	REQUIRE(r.Contains(SRCH_DATE(2015.0)) == GDT_FALSE);
	REQUIRE(r.Contains(SRCH_DATE(2030.0)) == GDT_FALSE);
}

TEST_CASE("DATERANGE Contains is true at the exact boundaries", "[date]") {
	DATERANGE r(SRCH_DATE(2020.0), SRCH_DATE(2025.0));
	REQUIRE(r.Contains(SRCH_DATE(2020.0)) == GDT_TRUE);
	REQUIRE(r.Contains(SRCH_DATE(2025.0)) == GDT_TRUE);
}
