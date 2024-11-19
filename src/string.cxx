/* $Id: string.cxx,v 1.41 2001/02/23 16:57:49 cnidr Exp $ */
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1994. 

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby granted
without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included i
this distribution must remain intact. 

2. Users of this software agree to make their best efforts (a) to retur
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
File:		string.cxx
Version:	1.02
$Revision: 1.41 $
Description:	Class STRING
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <ctype.h>
#include <sys/stat.h>

#ifdef UNIX
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#endif

#include "common.hxx"
#include "string.hxx"

#ifdef METRICS

INT STRING::NumTimesConstructed=0;
INT STRING::NumTimesCopied=0;
INT STRING::NumTimesExpanded=0;
INT STRING::NumTimesDeleted=0;
INT STRING::TotalStringLength=0;
INT STRING::TotalNumStrings=0;
INT STRING::NumNullStrings=0;
INT STRING::TotalStringExpns=0;


INT 
STRING::GetNumTimesConstructed(void)  {
  return NumTimesConstructed;
}


INT 
STRING::GetNumTimesCopied(void)  {
  return NumTimesCopied;
}


INT 
STRING::GetNumTimesExpanded(void)  {
  return NumTimesExpanded;
}


INT 
STRING::GetNumTimesDeleted(void)  {
  return NumTimesDeleted;
}


INT 
STRING::GetTotalNumStrings(void)  {
  return TotalNumStrings;
}


STRINGINDEX 
STRING::GetTotalStringLength(void)  {
  return TotalStringLength;
}


INT 
STRING::GetNumNullStrings(void)  {
  return NumNullStrings;
}


INT 
STRING::GetTotalStringExpns(void) {
  return TotalStringExpns;
}


time_t 
STRING::GetHowLong(void)  {
  //not implemented yet.
  return -1;
}


DOUBLE 
STRING::GetAvgCopiedStrLen(void)  {
  return (DOUBLE) TotalStringLength / NumTimesCopied;
}


DOUBLE 
STRING::GetAvgTotalStrLen(void)  {
  return (DOUBLE) TotalStringLength / TotalNumStrings;
}


DOUBLE STRING::GetAvgTotalStringExpns(void) {
  return (DOUBLE) TotalStringExpns / NumTimesExpanded;
}


INT 
STRING::GetTotalTimesShrunk(void)  {
  return NumTimesCopied - NumTimesExpanded;
}

void 
STRING::WriteMetrics(FILE *fp)  {
  fprintf(fp, "STRING's were used %d times\n", TotalNumStrings);
  fprintf(fp, "The STRING constructors have been called %d times.\n",
	  NumTimesConstructed);
  fprintf(fp, "%d NULL STRING's have been created.\n", NumNullStrings);
  fprintf(fp, "%d STRING's have been copied.\n", NumTimesCopied);
  fprintf(fp, "%d STRING buffers have been deleted\n", NumTimesDeleted);
  fprintf(fp, "STRING's have been expanded %d times\n", NumTimesExpanded);
  fprintf(fp, "The average expansion is %4f characters\n", GetAvgTotalStringExpns());
  fprintf(fp, "STRING's have been shrunk %d times\n", GetTotalTimesShrunk());
  fprintf(fp, "Average Copied String Length %4f\n", GetAvgCopiedStrLen());
  fprintf(fp, "Average Total String Length %4f\n", GetAvgTotalStrLen());
}


void 
STRING::PrintMetrics(void)  {
  WriteMetrics(stdout);
}


#endif


// magic values, based on experimentation 
// see http://ficus.cnidr.org:8080/metrics.html
STRINGINDEX STRING::InitialBufferLength = 20;
STRINGINDEX STRING::BufferLengthIncr = 12;
GDT_BOOLEAN STRING::DoubleBufferOnCopy = GDT_FALSE;

/*
  //eventually we'll be smarter. for now, we're conservative.
  INT STRING::InitialBufferLength = 0;
  INT STRING::BufferLengthIncr = 0;
  GDT_BOOLEAN STRING::DoubleBufferOnCopy = GDT_FALSE;
*/


void 
STRING::SetMinInitBufLen(STRINGINDEX InitBufLen) 
{
  InitialBufferLength = InitBufLen;
}

	
void 
STRING::SetBufLenIncr(STRINGINDEX BufLenIncr) 
{
  BufferLengthIncr = BufLenIncr;
}


void 
STRING::SetDoDoubleBufLen(GDT_BOOLEAN DoDoubling) 
{
  DoubleBufferOnCopy = DoDoubling;
}


//this is where allocation and copying policy lives
void 
STRING::StrBuffAlloc(STRINGINDEX BufferSizeRequest) 
{

#ifdef METRICS
  if (BufferSizeRequest > Length) {
    NumTimesExpanded++;
    TotalStringExpns += (BufferSizeRequest - Length);
  }
#endif

  if ((!Buffer)                                // added test for Buffer
      || (BufferSizeRequest > BufferSize)) {
    if (Buffer) {
      delete [] Buffer;
    }

    if (BufferSize != 0) {

#ifdef METRICS
      NumTimesDeleted++;
#endif

      if (DoubleBufferOnCopy) 
	BufferSize =  BufferSizeRequest * 2;
      else 
	BufferSize = (BufferSizeRequest + BufferLengthIncr);

    } else {
      BufferSize = BufferSizeRequest > InitialBufferLength ?
	BufferSizeRequest : InitialBufferLength;
    }

    Buffer = new UCHR[BufferSize];
  }
}


void 
STRING::Copy(const UCHR *CString, STRINGINDEX CLength) 
{
  if ( !CString )
    CLength = 0;
  /*
  // Are we copying part of a string into itself?
  if ((Buffer)                             // Added test for Buffer
      && (CString >= Buffer) 
      && (CString < (Buffer + Length))) {
    EraseBefore(CString - Buffer + 1);
    EraseAfter(CLength);
    return;
  }
  */

  StrBuffAlloc(CLength + 1);

  if (CLength > 0) {
    memcpy(Buffer, CString, CLength);
#ifdef METRICS
    NumTimesCopied++;
#endif
  }
#ifdef METRICS
  else {
    NumNullStrings++;
  }
#endif

  Length = CLength;
  Buffer[Length] = '\0';

#ifdef METRICS
  TotalNumStrings++;
  TotalStringLength += Length;
#endif
}


STRING::STRING() 
{
  //  BufferSize = 1;
  BufferSize = InitialBufferLength;
  Buffer = new UCHR[BufferSize];
  Length = 0;
  Buffer[Length] = '\0';
#ifdef METRICS
  NumTimesConstructed++;
  TotalNumStrings++;
#endif
}


STRING::STRING(const STRING& OtherString) 
{
  Buffer = (UCHR*)NULL;
  Length = BufferSize = 0; 
  Copy(OtherString.Buffer, OtherString.Length);		
#ifdef METRICS
  NumTimesConstructed++;
#endif
}


STRING::STRING(const CHR* CString) 
{
  Buffer = (UCHR*)NULL;
  Length = BufferSize = 0; 
  Copy((UCHR *)CString, strlen(CString));
#ifdef METRICS
  NumTimesConstructed++;
#endif
}


STRING::STRING(const UCHR* CString) 
{
  Buffer = (UCHR*)NULL;
  Length = BufferSize = 0; 
  Copy(CString, strlen((CHR *)CString));
#ifdef METRICS
  NumTimesConstructed++;
#endif
}


STRING::STRING(const CHR* NewBuffer, const STRINGINDEX BufferLength) 
{
  Buffer = (UCHR*)NULL;
  Length = BufferSize = 0; 
  Copy((UCHR *)NewBuffer, BufferLength);
#ifdef METRICS
  NumTimesConstructed++;
#endif
}


STRING::STRING(const UCHR* NewBuffer, const STRINGINDEX BufferLength) 
{
  Buffer = (UCHR*)NULL;
  Length = BufferSize = 0; 
  Copy(NewBuffer, BufferLength);
#ifdef METRICS
  NumTimesConstructed++;
#endif
}


STRING::STRING(const INT IntValue) 
{
  Buffer = (UCHR*)NULL;
  Length = BufferSize = 0;
  CHR s[256];
  sprintf(s, "%i", IntValue);
  Copy((UCHR *)s, strlen(s));
#ifdef METRICS
  NumTimesConstructed++;
#endif
}


STRING& STRING::operator=(const CHR* CString) 
{
  Copy((UCHR *)CString, strlen(CString));
  return *this;
}


STRING& STRING::operator=(const GDT_BOOLEAN BoolValue) 
{
  CHR s[256];
  sprintf(s, "%i", (INT)BoolValue);
  *this = s;
  return *this;
}


STRING& STRING::operator=(const INT IntValue) {
  CHR s[256];
  sprintf(s, "%i", IntValue);
  *this = s;
  return *this;
}


STRING& STRING::operator=(const LONG LongValue) {
  CHR s[256];
  sprintf(s, "%li", LongValue);
  *this = s;
  return *this;
}


STRING& STRING::operator=(const DOUBLE DoubleValue) {
  CHR s[256];
  sprintf(s, "%f", DoubleValue);
  *this = s;
  return *this;
}


STRING& STRING::operator=(const STRING& OtherString) {
  //if he is me, and me is he, oh gee!
  if (&OtherString == this) 
    return *this;
  Copy(OtherString.Buffer, OtherString.Length);
#ifdef METRICS
  TotalNumStrings++;
#endif
  return *this;
}


void 
STRING::Set(const UCHR* NewBuffer, const STRINGINDEX BufferLength) {
  Copy(NewBuffer, BufferLength);
}


STRING::operator const char *() const {
  return (const char *)Buffer;
}


STRING::operator const unsigned char *() const {
  return (const unsigned char *)Buffer;
}


STRING& STRING::operator+=(const UCHR Character) {
  Cat(Character);
  return *this;
}


STRING& STRING::operator+=(const CHR* CString) {
  Cat(CString);
  return *this;
}


STRING& STRING::operator+=(const STRING& OtherString) {
  Cat(OtherString);
  return *this;
}


INT STRING::operator==(const STRING& OtherString) const {
  return Equals(OtherString);
}


INT STRING::operator==(const CHR* CString) const {
  return Equals(CString);
}


INT STRING::operator!=(const STRING& OtherString) const {
  return !(Equals(OtherString));
}


INT STRING::operator!=(const CHR* CString) const {
  return !(Equals(CString));
}


INT STRING::operator^=(const STRING& OtherString) const {
  return CaseEquals(OtherString);
}


INT STRING::operator^=(const CHR* CString) const {
  return CaseEquals(CString);
}


INT 
STRING::Equals(const STRING& OtherString) const {
  if (Length != OtherString.Length) {
    return 0;
  }
  return ( memcmp(Buffer, OtherString.Buffer, Length) == 0 );
}


INT 
STRING::Equals(const CHR* CString) const {
  if (Length != (STRINGINDEX)strlen(CString)) {
    return 0;
  }
  return ( memcmp(Buffer, CString, Length) == 0 );
}

/*
INT STRING::GreaterThan(const STRING& OtherString) const {
	STRINGINDEX SmallerLength;
	if (Length > OtherString.Length) {
		SmallerLength = OtherString.Length;
	} else {
		SmallerLength = Length;
	}
	STRINGINDEX x;
	for (x=0; x<SmallerLength; x++) {
		if (Buffer[x]
	}

}
*/


INT 
STRING::CaseEquals(const STRING& OtherString) const {
  if (Length != OtherString.Length) {
    return 0;
  }
  STRINGINDEX x;
  for (x=0; x<Length; x++) {
    if (toupper(Buffer[x]) != toupper(OtherString.Buffer[x])) {
      return 0;
    }
  }
  return 1;
}


INT 
STRING::CaseEquals(const CHR* CString) const {
  const CHR *p1 = CString;
  const UCHR *p2 = Buffer;
  INT Match = 1;
  STRINGINDEX x;
  for (x = 0; ( (x < Length) && *p1 ); x++) {
    if ( (toupper(*p1) - toupper(*p2)) != 0) {
      Match = 0;
      break;
    }
    else {
      p1++; p2++;
    }
  }
  if (Match) {
    if ( (x == Length) && !*p1)
      return 1;
  }
  
  return 0;
}


void 
STRING::Print() const {
//  cout.write(Buffer, Length);
  printf("%s", Buffer);
}


void 
STRING::Print(PFILE FilePointer) const {
  STRINGINDEX x;
  for (x=0; x<Length; x++)
    fprintf(FilePointer, "%c", Buffer[x]);
}


// can this be const STRING& ?
ostream& operator<<(ostream& os, const STRING& str) {
  os.write((const char *) str.Buffer, str.Length);
  return os;
}


istream& operator>>(istream& is, STRING& str) {
  CHR buf[256];
  //  is >> buf;
  is.getline(buf,255);
  str = buf;
  return is;
}


INT 
STRING::GetInt() const {
  return atoi(*this);
}


LONG
STRING::GetLong() const {
  return atol(*this);
}


DOUBLE 
STRING::GetFloat() const {
  return atof(*this);
}


GDT_BOOLEAN 
STRING::FGet(PFILE FilePointer, const STRINGINDEX MaxCharacters) {
  CHR* pc = new CHR[MaxCharacters+2];
  CHR* p;
  GDT_BOOLEAN Ok;
  if (fgets(pc, MaxCharacters+1, FilePointer)) {
    p = pc + strlen(pc) - 1;
    while ( (p >= pc) && ( (*p == '\n') || (*p == '\r') ) ) {
      *(p--) = '\0';
    }
    *this = pc;
    Ok = GDT_TRUE;
  } else {
    *this = "";
    Ok = GDT_FALSE;
  }
  delete [] pc;
  return Ok;
}


GDT_BOOLEAN 
STRING::FGetMultiLine(PFILE FilePointer, const STRINGINDEX MaxCharacters) {
  // Gets a string from multiple lines, using Unix-like \ at the end of
  // the line for a continuation character
  CHR* pc = new CHR[MaxCharacters+2];
  CHR* p;
  GDT_BOOLEAN Ok=GDT_FALSE;
  STRING Buf="";

  while (fgets(pc, MaxCharacters+1, FilePointer)) {
    Ok=GDT_TRUE;
    p = pc + strlen(pc) - 1;
    // Get to the last useful character in the buffer we just read
    while ( (p >= pc) && ( (*p == '\n') || (*p == '\r') ) ) {
      *(p--) = '\0';
    }

    // If the last character is a backslash, we have to store this line
    // into Buf and go get another one.  Otherwise, we just got the last
    // line.
    if (p >= pc && *p == '\\') {
      *(p--) = '\0';
      Buf.Cat(pc);
    } else {
      Buf.Cat(pc);
      break;
    }

  }

  if (Ok)
    *this = Buf;
  else
    *this="";

  delete [] pc;
  return Ok;
}


STRINGINDEX 
STRING::GetLength() const {
  return Length;
}


UCHR 
STRING::GetChr(STRINGINDEX Index) const {
  if ( (Index > 0) && (Index <= Length) )
    return Buffer[Index-1];
  else
    return 0;	// generate ER
}


void 
STRING::SetChr(const STRINGINDEX Index, const UCHR NewChr) {
  if (Index > 0) {
    if (Index > Length) {
      STRINGINDEX x;
      for (x=1; x<(Index-Length); x++) {
	Cat(' ');
      }
      Cat(NewChr);
    } else {
      Buffer[Index-1] = NewChr;
    }
  }
}


void 
STRING::Cat(const UCHR Character) 
{
  if (BufferSize >= Length+2) {
    Buffer[Length] = Character;

#ifdef METRICS
    NumTimesExpanded++;		//this is usually done in StrBuffAlloc
    TotalStringExpns++;		//ditto
#endif

  } else {
    UCHR *Temp = Buffer;
    Buffer = (UCHR*)NULL;
    StrBuffAlloc(Length + 2);
    if (Length>0)
      memcpy(Buffer, Temp, Length);
    Buffer[Length] = Character;
    if (Temp)
      delete [] Temp;
  }
  Length++;
  Buffer[Length] = '\0';
#ifdef METRICS
  TotalStringLength++;
  TotalNumStrings++;
#endif
}


void 
STRING::Cat(const CHR* CString) {
  Cat(CString, strlen(CString));
}


void 
STRING::Cat(const CHR* CString, STRINGINDEX CLength) {
  if (CLength == 0) {
    return;
  }

  if (BufferSize >= (CLength + Length + 1)) {
    memcpy(Buffer + Length, CString, CLength);

#ifdef METRICS
    NumTimesExpanded++;		//this is usually done in StrBuffAlloc
    TotalStringExpns += CLength; //ditto
#endif

  } else {
    UCHR *Temp = Buffer;
    Buffer = (UCHR*)NULL;
    StrBuffAlloc(Length + CLength + 1);
    if (Length>0)
      memcpy(Buffer, Temp, Length);
    memcpy(Buffer + Length, CString, CLength);
    if (Temp)
      delete [] Temp;
  }
  Length += CLength;
  Buffer[Length] = '\0';
#ifdef METRICS
  TotalStringLength += Length;
  TotalNumStrings++;
#endif
}


void 
STRING::Cat(const STRING& OtherString) {
  if (OtherString.Length == 0)
    return;
  Cat((CHR *)OtherString.Buffer, OtherString.Length);
}


void 
STRING::Insert(const STRINGINDEX InsertionPoint, const STRING& OtherString) {
  STRINGINDEX StringLength = OtherString.Length;
  if (StringLength == 0) {
    return;
  }
  if (Length == 0)  {
    StrBuffAlloc(StringLength + 1);
    memcpy(Buffer, OtherString.Buffer, StringLength);
  }
  else if (BufferSize >= (StringLength + Length + 1)) {
    INT RemnantSize = Length - InsertionPoint + 1;
    UCHR *EndFirstBit = Buffer + InsertionPoint - 1;
    UCHR* Remnant = new UCHR[RemnantSize];
    memcpy(Remnant, EndFirstBit, RemnantSize);
    memcpy(EndFirstBit, OtherString.Buffer, StringLength);
    memcpy(EndFirstBit + StringLength, Remnant, RemnantSize);
    delete [] Remnant;
#ifdef METRICS
    NumTimesExpanded++;
    TotalStringExpns += StringLength;
#endif
  }
  else {
    UCHR* Temp = Buffer;
    Buffer = (UCHR*)NULL;
    StrBuffAlloc(Length + StringLength + 1);
    
    //index of the rest of the string
    INT ToCopy = InsertionPoint - 1;
    
    //pointer to the rest of the string
    UCHR *EndBit = Buffer + InsertionPoint - 1;
    
    //how many characters remai
    INT RemnantSize = Length - InsertionPoint + 1;
    memcpy(Buffer, Temp, ToCopy);
    memcpy(Buffer+ToCopy, OtherString.Buffer, StringLength);
    memcpy(EndBit + StringLength, Temp+ToCopy, RemnantSize);
    delete [] Temp;
  }
  Length += StringLength;
  Buffer[Length] = '\0';
#ifdef METRICS
  NumTimesCopied++;
  TotalNumStrings++;
  TotalStringLength += Length;
#endif
}


STRINGINDEX 
STRING::Search(const CHR* CString) const {
  char *p;
  p = strstr((char *)Buffer, CString);
  return p ? (p - (char *)Buffer + 1) : 0;
}

  
STRINGINDEX 
STRING::Search(const UCHR Character) const {
  char *p;
  p = strchr((char *)Buffer, Character);
  return p ? (p - (char *)Buffer + 1) : 0;
}


STRINGINDEX 
STRING::SearchReverse(const CHR* CString) const {
  STRINGINDEX x;
  STRINGINDEX n = (STRINGINDEX) strlen(CString);
  if (n > Length)
    return 0;
  if (n == 0)
    return 0;			// Generate warning ER?
  x = Length - n + 1;
  if (x > 0)
    do {
      x--;
      if (strncmp(CString, (char *)Buffer + x, n) == 0)
	return (x + 1);
    } while (x > 0);
  return 0;
}


STRINGINDEX 
STRING::SearchReverse(const UCHR Character) const {
  STRINGINDEX x;
  if (Length == 0)
    return 0;
  x = Length;
  if (x > 0)
    do {
      x--;
      if (Buffer[x] == Character)
	return (x+1);
    } while (x > 0);
  return 0;
}


INT 
STRING::Replace(const CHR* CStringSearch, const CHR* CStringReplace) {
  STRING NewString, S;
  STRINGINDEX Position;
  INT4 CSLen = strlen(CStringSearch);
  INT Count = 0;
  while ( (Position=Search(CStringSearch)) != 0) {
    Count++;
    S = *this;
    S.EraseAfter(Position-1);
    NewString += S;
    NewString += CStringReplace;
    EraseBefore(Position + CSLen);
  }
  NewString += *this;
  *this = NewString;
  return Count;
}


INT 
STRING::Replace(const CHR* CStringSearch, const STRING& CStringReplace) {
  STRING NewString, S;
  STRINGINDEX Position;
  INT4 CSLen = strlen(CStringSearch);
  INT Count = 0;
  while ( (Position=Search(CStringSearch)) != 0) {
    Count++;
    S = *this;
    S.EraseAfter(Position-1);
    NewString += S;
    NewString += CStringReplace;
    EraseBefore(Position + CSLen);
  }
  NewString += *this;
  *this = NewString;
  return Count;
}

void 
STRING::EraseBefore(const STRINGINDEX Index) {
  if (Index <= 1) {
    return;
  }
  if (Index > Length) {
    Length = 0;
    Buffer[Length] = '\0';
    return;
  }

  Length -= Index - 1;
  memmove(Buffer, Buffer + Index - 1, Length);
  Buffer[Length] = '\0';
#ifdef METRICS
  TotalNumStrings++;
  TotalStringLength += Length;
  NumTimesCopied++;
#endif
}
/*
void 
STRING::EraseBefore(const STRINGINDEX Index) {
  if (Index <= 1) {
    return;
  }
  if (Index > Length) {
    Length = 0;
    Buffer[Length] = '\0';
    return;
  }
  UCHR* Temp = new UCHR[BufferSize];
  INT4 CharsLeft = Length - Index + 1;
  memcpy(Temp, Buffer + Index - 1, CharsLeft);
  delete [] Buffer;
  Buffer = Temp;
  Length = CharsLeft;
  Buffer[Length] = '\0';
#ifdef METRICS
  TotalNumStrings++;
  TotalStringLength += Length;
  NumTimesCopied++;
#endif
}
*/

void 
STRING::EraseAfter(const STRINGINDEX Index) {
  if (Index > Length)
    return;
  Length = Index;
  Buffer[Length] = '\0';
}


void 
STRING::UpperCase() {
  STRINGINDEX x;
  for (x=0; x<Length; x++) {
    Buffer[x] = toupper(Buffer[x]);
  }
}


void 
STRING::GetCString(CHR* CStringBuffer, const INT BufferSize) const {
  STRINGINDEX ShortLength = BufferSize - 1;
  if (Length < ShortLength)
    ShortLength = Length;
  if (Buffer)
    memcpy(CStringBuffer, Buffer, ShortLength);
  CStringBuffer[ShortLength] = '\0';
}


CHR* 
STRING::NewCString() const {
  return ((CHR*)NewUCString());
}


UCHR* 
STRING::NewUCString() const {
  UCHR* p = new UCHR[Length+1];
  if (Buffer) {
    memcpy(p, Buffer, Length);
  }
  p[Length] = '\0';
  return p;
}


void 
STRING::WriteFile(const STRING& FileName) const {
  PFILE fp;
  fp = fopen(FileName, "wb");
  if (!fp) {
    perror(FileName);
    fflush(stdout); 
    fflush(stderr);
    return;
  }
  else {
    if (Buffer) {
      fwrite((char*)Buffer, 1, Length, fp);
    }
    fclose(fp);
  }
}


GDT_BOOLEAN
STRING::ReadFile(const STRING& FileName) 
{
  PFILE fp;
  struct stat status;

  if (Buffer)
    delete [] Buffer;

  // See if we have a legitimate file
  if (stat(FileName,&status) == 0) {
    Length = GetFileSize(FileName);
    if (Length >= 0) {
      BufferSize = Length + 1;
      Buffer = new UCHR[BufferSize];
      if (Buffer) {                    // make sure the allocation succeeded
        if ( (Length > 0) && (fp = fopen(FileName, "rb")) ) {
	  // Zero length is ok, except we read nothing
	       fread((char*)Buffer, 1, Length, fp);
	  fclose(fp);
	}
      Buffer[Length] = '\0';
      }
    } else {
      Length = 0;
    }
    return GDT_TRUE;
  }
  return GDT_FALSE;
}


GDT_BOOLEAN
STRING::ReadFile(const CHR* FileName) 
{
  PFILE fp;
  struct stat status;

  if (Buffer)
    delete [] Buffer;

  // See if we have a legitimate file
  if (stat(FileName,&status) == 0) {
    Length = GetFileSize(FileName);
    if (Length >= 0) {
      BufferSize = Length + 1;
      Buffer = new UCHR[BufferSize];
      if (Buffer) {                    // make sure the allocation succeeded
        if ( (Length > 0) && (fp = fopen(FileName, "rb")) ) { 
	  // Zero length is ok, except we read nothing
	  fread((char*)Buffer, 1, Length, fp);
	  fclose(fp);
	}
      Buffer[Length] = '\0';
      }
    } else {
      Length = 0;
    }
    return GDT_TRUE;
  } 
  return GDT_FALSE;
}


GDT_BOOLEAN 
STRING::IsNumber() {
  STRINGINDEX x;
  for (x=0; x<Length; x++) {
    if (!((isdigit(Buffer[x])) 
	|| (Buffer[x] == '.')
	|| (Buffer[x] == '-')
	|| (Buffer[x] == '+')))
	return(GDT_FALSE);
  }
  return(GDT_TRUE);
}


GDT_BOOLEAN 
STRING::IsPrint() {
  STRINGINDEX x;
  for (x=0; x<Length; x++) {
    if (!isprint(Buffer[x]))
	return(GDT_FALSE);
  }
  return(GDT_TRUE);
}


void 
STRING::MakePrintable() {
  STRINGINDEX x;
  for (x=0; x<Length; x++) {
    if (!isprint(Buffer[x]))
	Buffer[x] = ' ';
  }
  return;
}


char *translate[] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //   0 -  7
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //   8 - 15
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  16
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  24
  NULL, NULL, "&quot;", NULL, NULL, NULL, "&amp;", "&apos;", // 32
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  40
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  48
  NULL, NULL, NULL, NULL, "&lt;", NULL, "&gt;", NULL, // 56
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  64
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  72
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  80
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  88
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  96
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 104
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 112
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 120 - 127
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 128
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 136
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 144
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 152
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 160
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 168
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 176
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 184
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 192
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 200
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 208
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 216
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 224
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 232
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 240
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL  // 248 - 255
};


void 
STRING::XmlCleanup() {
  CHR *newstr;
  // Before calling transcode, we might want to change all of the 
  // entities to their character equivalents so we only change 
  // legitimate & characters and not the ones that delimit the
  // entities, for example:
  Replace("&amp;","&");
  Replace("&gt;",">");
  Replace("&lt;","<");
  Replace("&quot;","\"");
  Replace("&apos;","\'");

  newstr=transcode((CHR*)Buffer, translate);
  Length = BufferSize = 0; 
  Copy((UCHR*)newstr, strlen(newstr));		

  delete [] newstr;

  return;
}

/*
 *  trancode - given a buffer and a translation array
 *             return a string buffer wherein characters in the old buffer 
 *                are mapped according to the translation array
 *             NOTE: the buffer returned must be freed by the caller
 *                   the buffer is 6 times the original string length
 *             By John HLB Tyler (Credit please?)
 */
char *transcode (char *buffer, char **transarray)
{
  char *obuf=buffer;            // Beginning of old buffer
  char *obufscan=obuf;          // Scanning point of old buffer
  long lennbuf=strlen(obuf)*6;  // Maximum length of new string
  char *nbuf;                   // Pointer to start of new string buffer
  char *ipnt;                   // Insertion point into new buffer
  char *maxipnt;                // End of new buffer
  char *rscan;                  // Pointer to a character in the replacement

  char entity[7];               // Buffer to hold the numeric entity

  nbuf = new char [lennbuf];    // Pointer to start of new string buffer
  ipnt=nbuf;
  maxipnt=nbuf+lennbuf-1;       // End of new buffer

  while ((*obufscan != '\0') && (ipnt<maxipnt)) {

    // Is there a replacement for the *obufscan?
    if (transarray[(unsigned char)*obufscan]) {
      //Yes
      //      fprintf(stderr,"1. %d\n",(unsigned char)*obufscan);
      for (rscan=transarray[(unsigned char)*obufscan];
	   *rscan != '\0' && ipnt<maxipnt; 
	   rscan++,ipnt++) {
	// copy the replacement to the insertion point
	*ipnt=*rscan;
      }
    } else {
      // No, so just copy the old buffer character, but watch out
      // for non-printable characters and "entify" those
      if ((unsigned char)*obufscan < 32) {
	//	fprintf(stderr,"2. %d\n",(unsigned char)*obufscan);
	*ipnt++ = ' ';
      } else if ((unsigned char)*obufscan < 128) {
	*ipnt++ = *obufscan;
      } else {
	//	fprintf(stderr,"3. %d\n",(unsigned char)*obufscan);
	sprintf(entity,"&#%d;",(unsigned char)*obufscan);
	for (rscan=entity;
	   *rscan != '\0' && ipnt<maxipnt; 
	   rscan++,ipnt++) {
	  *ipnt = *rscan;
	}
	//	*ipnt++ = ' ';	
      }
    }
    obufscan++;
  }
  *ipnt='\0'; // terminate the new string

  return (nbuf);
}



void 
STRING::Trim() {
  while (Length && isspace(Buffer[Length - 1]))
    Length--;
  Buffer[Length] = '\0';
  return;
}


void 
STRING::TrimLeading() {
  STRINGINDEX x=0;
  while (isspace(Buffer[x])) {
    x++;
  }
  if (x>0)
    EraseBefore(x+1);
  return;
}


INT
STRING::Cmp(const STRING& OtherString) {
  return(strcmp((const CHR*)Buffer,(const CHR*)OtherString.Buffer));
}


STRING::~STRING() {
  //  if (BufferSize)
  if (Buffer != (UCHR*)NULL)
    delete [] Buffer;
}


INT 
StrUnlink(const STRING& FileName) {
  return remove(FileName);
}


INT 
StrCaseCmp(const CHR* s1, const CHR* s2) {
#ifdef UNIX
  return strcasecmp(s1,s2);
#else
  return stricmp(s1,s2);
#endif
}


INT 
StrCaseCmp(const UCHR* s1, const UCHR* s2) {
  return StrCaseCmp((CHR*)s1, (CHR*)s2);
}


INT 
StrNCaseCmp(const CHR* s1, const CHR* s2, const size_t n) {
#ifdef UNIX
  return strncasecmp(s1,s2,n);
#else
  return strnicmp(s1,s2,n);
#endif
}


INT 
StrNCaseCmp(const UCHR* s1, const UCHR* s2, const size_t n) {
  return StrNCaseCmp((CHR*)s1, (CHR*)s2, n);
}


