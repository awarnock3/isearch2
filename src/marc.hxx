// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef MARC_HXX
#define MARC_HXX

#include "gdt.h"
#include "marclib.hxx"
#include "string.hxx"

#ifdef __cplusplus
extern "C" {
#endif

//
// Display formats:
//
#define MARC_FORMAT_DEFAULT    0
#define MARC_FORMAT_SHORT      1
#define MARC_FORMAT_MARC       2
#define MARC_FORMAT_EVALUATION 3
#define MARC_FORMAT_TITLE      4
#define MARC_FORMAT_HTML       5

// Parses one MARC bibliographic record (via marclib.hxx's GetMARC())
// and formats it for display in one of several fixed layouts (see
// MARC_FORMAT_* above and defaultformat[]/shortformat[]/etc. in
// marc.cxx). Sole real caller: doctype/usmarc.cxx's
// USMARC::Present(), which always constructs, uses, and destroys one
// MARC object at a time -- see docs/BUG_CATALOG.md#srcmarchxx for why
// that matters (the underlying MARC_REC allocator is one process-wide
// pool, not per-object).
class MARC {
	MARC_REC	*c_rec;
	char		*c_data;
	INT4		c_len;
	INT		c_format,
			c_maxlen;
public:
	MARC(STRING & Data);
	~MARC();

	void SetDisplayFormat(INT Format) { c_format = Format; }
	void SetDisplayWidth(INT Width) { c_maxlen = Width; }
	void Print(FILE *fp=stdout);
	void Print(STRING *StringBuffer);
	void GetPrettyBuffer(STRING *Buffer);
};
#ifdef __cplusplus
	   }
#endif

#endif
