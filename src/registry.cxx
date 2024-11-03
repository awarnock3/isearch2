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
File:		registry.cxx
Version:	1.00
Description:	Class REGISTRY - Structured Profile Registry
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <fstream.h>
#include <stdlib.h>

#include "registry.hxx"


REGISTRY::REGISTRY(const STRING& Title) {
  Data = Title;
  Next = 0;
  Child = 0;
}


REGISTRY::REGISTRY(const CHR *Title) {
  Data = Title;
  Next = 0;
  Child = 0;
}


REGISTRY& 
REGISTRY::operator=(const REGISTRY& OtherRegistry) {
  COUT << "WARNING: REGISTRY::operator=() not yet implemented!" << endl;
  Data = OtherRegistry.Data;
  Next = OtherRegistry.Next;
  Child = OtherRegistry.Child;
  return *this;
}


REGISTRY* 
REGISTRY::clone() const {
  REGISTRY* r = new REGISTRY(Data);
  if (Next) {
    r->Next = Next->clone();
  } else {
    r->Next = 0;
  }
  if (Child) {
    r->Child = Child->clone();
  } else {
    r->Child = 0;
  }
  return r;
}


void 
REGISTRY::PrintSgml(FILE* fp, const STRLIST& Position) {
  REGISTRY* node = FindNode(Position);
  if (node) {
    REGISTRY* p = node->Child;
    while (p != 0) {
      p->PrintSgml(fp);
      p = p->Next;
    }
  }
}


void 
REGISTRY::PrintSgml(FILE* fp) {
  if (Child) {
    fprintf(fp, "\n  <");
    Data.Print(fp);
    fprintf(fp, ">");
    Child->PrintSgml(fp);
    fprintf(fp, "</");
    Data.Print(fp);
    fprintf(fp, ">\n");
  } else {
    Data.Print(fp);
  }
  if (Next) {
    Next->PrintSgml(fp);
  }
}


REGISTRY* 
REGISTRY::FindNode(const STRING& Position) {
  PREGISTRY p = Child;
  if (p) {
    // Search for a matching node:
    while (p) {
      if (p->Data ^= Position) {
	return p;
      }
      p = p->Next;
    }
    return 0;
  } else {
    return 0;
  }
}


REGISTRY* 
REGISTRY::FindNode(const STRLIST& Position) {
  INT x;
  INT y = Position.GetTotalEntries();
  if (!y) {
    return this;
  }
  STRING S;
  PREGISTRY TempNode = this;
  for (x=1; x<=y; x++) {
    Position.GetEntry(x, &S);
    TempNode = TempNode->FindNode(S);
    if (!TempNode) {
      return 0;
    }
  }
  return TempNode;
}


REGISTRY* 
REGISTRY::GetNode(const STRING& Position) {
  PREGISTRY p = Child;
  if (p) {
    PREGISTRY op = 0;
    // Search for a matching node:
    while (p) {
      if (p->Data ^= Position) {
	return p;
      }
      op = p;
      p = p->Next;
    }
    // End up with op pointing to the last node.
    // Now create new node:
    p = new REGISTRY(Position);
    op->Next = p;
    return p;
  } else {
    // Create single node:
    p = new REGISTRY(Position);
    Child = p;
    return p;
  }
}


REGISTRY* 
REGISTRY::GetNode(const STRLIST& Position) {
  INT x;
  INT y = Position.GetTotalEntries();
  if (!y) {
    return this;
  }
  STRING S;
  PREGISTRY TempNode = this;
  for (x=1; x<=y; x++) {
    Position.GetEntry(x, &S);
    TempNode = TempNode->GetNode(S);
  }
  return TempNode;
}


void 
REGISTRY::SetData(const STRLIST& Value) {
  DeleteChildren();
  AddData(Value);
}


void 
REGISTRY::AddData(const STRLIST& Value) {
  PREGISTRY p;
  INT y = Value.GetTotalEntries();
  if (y == 0) {
    return;
  }
  STRING S;
  Value.GetEntry(1, &S);
  PREGISTRY NewNode = new REGISTRY(S);
  if (Child) {
    p = Child;
    while (p->Next) {
      p = p->Next;
    }
    p->Next = NewNode;
  } else {
    Child = NewNode;
  }
  p = NewNode;
  // Now add the rest
  INT x;
  for (x=2; x<=y; x++) {
    Value.GetEntry(x, &S);
    NewNode = new REGISTRY(S);
    p->Next = NewNode;
    p = NewNode;
  }
}


void 
REGISTRY::SetData(const STRLIST& Position, const STRLIST& Value) {
  PREGISTRY Node = GetNode(Position);
  Node->SetData(Value);
}


void 
REGISTRY::AddData(const STRLIST& Position, const STRLIST& Value) {
  PREGISTRY Node = GetNode(Position);
  Node->AddData(Value);
}


void 
REGISTRY::GetData(STRLIST *StrlistBuffer) {
  StrlistBuffer->Clear();
  PREGISTRY p = Child;
  while (p) {
    StrlistBuffer->AddEntry(p->Data);
    p = p->Next;
  }
}


void 
REGISTRY::GetData(const STRLIST& Position, STRLIST *StrlistBuffer) {
  PREGISTRY Node = FindNode(Position);
  if (Node) {
    Node->GetData(StrlistBuffer);
  } else {
    StrlistBuffer->Clear();
  }
}


void 
REGISTRY::ProfileGetString(const STRING& Section, const STRING& Entry,
			   const STRING& Default, STRING *StringBuffer) {
  *StringBuffer = Default;
  STRLIST Position;
  Position.AddEntry(Section);
  Position.AddEntry(Entry);
  PREGISTRY Node = FindNode(Position);
  if (Node) {
    STRLIST Strlist;
    Node->GetData(&Strlist);
    INT y = Strlist.GetTotalEntries();
    if (y) {
      if (y == 1) {
	Strlist.GetEntry(1, StringBuffer);
      } else {
	Strlist.Join(",", StringBuffer);
      }
    }
  }
}


void 
REGISTRY::ProfileWriteString(const STRING& Section, const STRING& Entry,
			     const STRING& StringData) {
  STRLIST Position, Value;
  Position.AddEntry(Section);
  Position.AddEntry(Entry);
  Value.Split(",", StringData);
  SetData(Position, Value);
}

/*
void REGISTRY::Print(ostream& os, const INT Level) const {
	INT x;
	for (x=0; x<Level; x++) {
		os << ' ';
	}
	//os << '+' << Data << endl; // core dump
	os << "+";
	os << Data;
	os << endl;
	PREGISTRY p = Child;
	while (p) {
		p->Print(os, Level + 1);
		p = p->Next;
	}
}
*/

#ifdef UNIX

//#if defined(SGI_CC) or defined(_HP_)
#ifdef SGI_CC
void 
REGISTRY::fprint(FILE* fp, const INT level) const 
#else
void 
REGISTRY::fprint(FILE* fp, const INT level = 0) const 
#endif

#else
void 
REGISTRY::fprint(FILE* fp, const INT level) const 
#endif
{
	INT x;
	for (x=0; x < level; x++) {
		fprintf(fp, " ");
	}
	fprintf(fp, "+");
	Data.Print(fp);
	fprintf(fp, "\n");
	REGISTRY* p = Child;
	while (p) {
		p->fprint(fp, level + 1);
		p = p->Next;
	}
}


void 
REGISTRY::SaveToFile(const STRING& FileName, const STRLIST&
			  Position) {
	REGISTRY* node = FindNode(Position);
	if (node) {
		CHR* fn = FileName.NewCString();
		FILE* fp = fopen(fn, "w");
		delete [] fn;
		if (fp) {
			STRING s;
			node->fprint(fp);
			fclose(fp);
		}
	}
	/*
	  PCHR Fn = FileName.NewCString();
	  ofstream Ofs(Fn);
	  delete [] Fn;
	  PREGISTRY Node = FindNode(Position);
	  if (Node) {
	  Ofs << *Node;
	  }
	*/
}


void 
REGISTRY::ProfileLoadFromFile(const STRING& FileName, 
			      const STRLIST& Position) {
  PREGISTRY Node = GetNode(Position);
  Node->DeleteChildren();
  Node->ProfileAddFromFile(FileName);
}


void 
REGISTRY::ProfileAddFromFile(const STRING& FileName, 
			     const STRLIST& Position) {
  PREGISTRY Node = GetNode(Position);
  Node->ProfileAddFromFile(FileName);
}


void 
REGISTRY::ProfileAddFromFile(const STRING& FileName) {
  PFILE Fp = fopen(FileName, "rb");
  //don't need to die if not found. -jem.
  if (Fp) {
    STRLIST Position, Value;
    STRING S, T;
    INT x;
    //		while (S.FGet(Fp, 4000)) {
    while (S.FGetMultiLine(Fp, 4000)) {
      if (S.GetChr(1) == '[') {
	S.EraseBefore(2);
	x = S.Search(']');
	S.EraseAfter(x-1);
	Position.SetEntry(1, S);
      } else {
	if ((S.GetChr(1) != '#') && (S.GetChr(1) != ';')) {
	  x = S.Search('=');
	  if (x) {
	    T = S;
	    S.EraseAfter(x-1);
	    T.EraseBefore(x+1);
	    Position.SetEntry(2, S);
	    Value.Clear();
	    if (T.Search(',')) {
	      Value.Split(",", T);
	    } else {
	      Value.AddEntry(T);
	    }
	    SetData(Position, Value);
	  }
	}
      }
    }
    fclose(Fp);
  }
}


void 
REGISTRY::ProfileWrite(ostream& os, const STRING& FileName, 
		       const STRLIST& Position) {
  REGISTRY *Node = Child;
  while (Node) {
    Node->ProfilePrint(os,0);
    Node=Node->Next;
  }
}


void 
REGISTRY::ProfilePrint(ostream& os, const INT Level) const {
  //  INT x;
  if (Level==0)
    os << "[";
  os << Data;
  if (Level==0)
    os << "]";
  if (Level==1)
    os << "=";
  else {
    if ((Level>1) && (Next)) 
      os << ",";
    else
      os << endl;
  }
  PREGISTRY p = Child;
  while (p) {
    p->ProfilePrint(os, Level + 1);
    p = p->Next;
  }
}


void 
REGISTRY::LoadFromFile(const STRING& FileName, const STRLIST& Position) {
  PREGISTRY Node = GetNode(Position);
  Node->LoadFromFile(FileName);
}


void 
REGISTRY::LoadFromFile(const STRING& FileName) {
  DeleteChildren();
  AddFromFile(FileName);
}


void 
REGISTRY::AddFromFile(const STRING& FileName, const STRLIST& Position) {
  PREGISTRY Node = GetNode(Position);
  Node->AddFromFile(FileName);
}


void 
REGISTRY::AddFromFile(const STRING& FileName) {
  PFILE Fp = fopen(FileName, "rb");
  //don't need to die if not found. -jem.
  if (Fp) {
    STRING S;
    STRLIST Position, Value;
    STRING NewData;
    INT x;
    while (S.FGet(Fp, 4000)) {
      x = S.Search('+');
      if (x) {
	NewData = S;
	NewData.EraseBefore(x+1);
	if (x <= (Position.GetTotalEntries()+1)) {
	  Position.EraseAfter(x-1);
	  Value.SetEntry(1, NewData);
	  AddData(Position, Value);
	  Position.AddEntry(NewData);
	}
      }
    }
    fclose(Fp);
  }
}

/*
// can this be const?
ostream& operator<<(ostream& os, const REGISTRY& Registry) {
  Registry.Print(os, 0);
  return os;
}
*/

void 
REGISTRY::DeleteChildren() {
  if (Child) {
    delete Child;
    Child = 0;
  }
}


REGISTRY::~REGISTRY() {
  if (Next) {
    delete Next;
  }
  if (Child) {
    delete Child;
  }
}


REGISTRY* 
parseMetaDefaults(const STRING& filename) {
  REGISTRY* metadef = new REGISTRY("meta");
  FILE* fp = fopen(filename, "r");
  int eof = 0;
  int tokenReady = 0;
  int tagging = 0;
  char token[1024];
  char* tokenEnd;
  STRLIST path;
  STRLIST* data;
  if (fp) {
    char c;
    while ( ! eof ) {
      // get token
	   tagging = 0;
      tokenEnd = token;
      tokenReady = 0;
      while ( ! tokenReady ) {
	c = fgetc(fp);
	if (c == EOF) {
	  tokenReady = 1;
	  eof = 1;
	  break;
	}
	if (tagging) {
	  if (c == '>') {
	    tokenReady = 1;
	  } else {
	    *(tokenEnd++) = c;
	  }
	} else {
	  if (c == '<') {
	    if (tokenEnd == token) {
	      tagging = 1;
	      *(tokenEnd++) = c;
	    } else {
	      ungetc(c, fp);
	      tokenReady = 1;
	    }
	  } else {
	    *(tokenEnd++) = c;
	  }
	}
      }
      *tokenEnd = '\0';
      if (token[0] == '<') {
	switch (token[1]) {
	case '/':
	  path.EraseAfter(path.GetTotalEntries() - 1);
	  break;
	case '?':
	case '!':
	  break;
	default:
	  path.AddEntry(token + 1);
	}
      } else {
	if (path.GetTotalEntries() > 0) {
	  data = new STRLIST();
	  trimWhitespace(token);
	  data->AddEntry(token);
	  metadef->AddData(path, *data);
	}
      }
    }
    delete data;
    fclose(fp);
  }
  return metadef;
}

