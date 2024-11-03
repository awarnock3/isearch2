// $Id: thesaurus.cxx,v 1.2 1999/04/22 18:19:53 cnidr Exp $
/************************************************************************
Copyright Notice

Copyright (c) A/WWW Enterprises, 1999.
************************************************************************/

/*@@@
File:		thesaurus.hxx
Version:	$Revision: 1.2 $
Description:	Class THESAURUS - Thesaurus and synonyms
Author:		Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
@@@*/

#include "thesaurus.hxx"

extern INT ParentSortCmp(const void* x, const void* y);
extern INT ParentSearchCmp(const void* x, const void* y);
extern INT EntrySortCmp(const void* x, const void* y);
extern INT EntrySearchCmp(const void* x, const void* y);

/////////////////////////////////////////////////////////////////
// Class: TH_PARENT
/////////////////////////////////////////////////////////////////
// Here are the methods for the parent terms
TH_PARENT::TH_PARENT() {
}


TH_PARENT::~TH_PARENT() {
}


INT4
TH_PARENT::GetGlobalStart() {
  return(GlobalStart);
}


void
TH_PARENT::SetGlobalStart(INT4 x) {
  GlobalStart = x;
}


void 
TH_PARENT::GetString(STRING* TheTerm) {
  *TheTerm = Term;
}


void
TH_PARENT::SetString(const STRING& NewTerm) {
  Term = NewTerm;
}


void
TH_PARENT::Copy(const TH_PARENT& OtherValue) {
}


TH_PARENT&
TH_PARENT::operator=(const TH_PARENT& OtherValue) {
  GlobalStart = OtherValue.GlobalStart;
  Term = OtherValue.Term;
  return *this;
}


/////////////////////////////////////////////////////////////////
// Class: TH_PARENT_LIST
/////////////////////////////////////////////////////////////////


INT
ParentSortCmp(const void* x, const void* y) 
{
  STRING s1,s2;
  INT val;

  ((TH_PARENT*)x)->GetString(&s1);
  ((TH_PARENT*)y)->GetString(&s2);
  val = s1.Cmp(s2);
  return(val);
}


INT
ParentSearchCmp(const void* x, const void* y) 
{
  STRING s1,s2;
  INT val;

  s1 = (CHR*)x;
  ((TH_PARENT*)y)->GetString(&s2);
  val = s1.Cmp(s2);
  return(val);
}


//  Here are the methods for handling lists of parent terms
TH_PARENT_LIST::TH_PARENT_LIST() {
  table = new TH_PARENT[100];
  Count = 0;
  MaxEntries = 100;
}


void
TH_PARENT_LIST::AddEntry(const TH_PARENT& NewParent) {
  table[Count] = NewParent;
  Count++;
}


void
TH_PARENT_LIST::GetEntry(const INT4 index, TH_PARENT* TheParent) {
  if (index <= Count) {
    *TheParent = table[index];
  }
}


TH_PARENT*
TH_PARENT_LIST::GetEntry(const INT4 index) {
  if (index <= Count) {
    return(&table[index]);
  }
  return((TH_PARENT*)NULL);
}


INT4
TH_PARENT_LIST::GetCount() {
  return(Count);
}


void
TH_PARENT_LIST::Dump(PFILE fp) {
  INT4 ptr;
  STRING str;
  CHR *ch;

  if (fp) {
    for (INT4 i=0;i<Count;i++) {
      ptr = table[i].GetGlobalStart();
      table[i].GetString(&str);
      ch = str.NewCString();
      fprintf(fp,"[%d]\t%s\n",ptr,ch);
      delete [] ch;
    }
  }
}


void
TH_PARENT_LIST::WriteTable(PFILE fp) {
  INT4 ptr;
  STRING str;

  if (fp) {
    fwrite((CHR*)&Count,1,sizeof(INT4),fp);
    
    for (INT4 i=0;i<Count;i++) {
      ptr = table[i].GetGlobalStart();
      //      table[i].GetString(&str);
      fwrite((CHR*)&ptr,1,sizeof(INT4),fp);
    }
  }
}


void
TH_PARENT_LIST::LoadTable(PFILE fp) {
  INT4 ptr,x;
  STRING str;

  if (fp) {
    x = fread((CHR*)&Count,1,sizeof(INT4),fp);
    if (x == 0)
      return;

    for (INT4 i=0;i<Count;i++) {
      x = fread((CHR*)&ptr,1,sizeof(INT4),fp);
      if (x > 0) {
	table[i].SetGlobalStart(ptr);
	table[i].SetString("");
      } else {
	table[i].SetGlobalStart(0);
      }
    }
  } else {
    Count = 0;
  }
}


void 
TH_PARENT_LIST::Sort() {
  qsort((void *)table, Count, sizeof(TH_PARENT),ParentSortCmp);
}


void*
TH_PARENT_LIST::Search(const void* term) {
  void *ptr;
  ptr = bsearch(term, (void *)table, Count, sizeof(TH_PARENT),ParentSearchCmp);
  return(ptr);
}


TH_PARENT_LIST::~TH_PARENT_LIST() {
  delete [] table;
}


/////////////////////////////////////////////////////////////////
// Class: TH_ENTRY
/////////////////////////////////////////////////////////////////
// Here are the methods for the child terms
TH_ENTRY::TH_ENTRY() {
}


void 
TH_ENTRY::SetGlobalStart(INT4 x) {
  GlobalStart=x;
}


INT4 
TH_ENTRY::GetGlobalStart() {
  return(GlobalStart);
}


void
TH_ENTRY::SetString(const STRING& NewTerm) {
  Term = NewTerm;
}


void
TH_ENTRY::GetString(STRING* TheTerm) {
  *TheTerm = Term;
}



void 
TH_ENTRY::SetParentPtr(INT4 x) {
  ParentPtr=x;
}

  
INT4 
TH_ENTRY::GetParentPtr() {
  return(ParentPtr);
}


TH_ENTRY::~TH_ENTRY() {
}



/////////////////////////////////////////////////////////////////
// Class: TH_ENTRY_LIST
/////////////////////////////////////////////////////////////////

INT
EntrySortCmp(const void* x, const void* y) 
{
  STRING s1,s2;
  INT val;

  ((TH_ENTRY*)x)->GetString(&s1);
  ((TH_ENTRY*)y)->GetString(&s2);
  val = s1.Cmp(s2);
  return(val);
}


INT
EntrySearchCmp(const void* x, const void* y) 
{
  STRING s1,s2;
  INT val;

  s1 = (CHR*)x;
  ((TH_ENTRY*)y)->GetString(&s2);
  val = s1.Cmp(s2);
  return(val);
}


//  Here are the methods for handling lists of parent terms
TH_ENTRY_LIST::TH_ENTRY_LIST() {
  table = new TH_ENTRY[100];
  Count = 0;
  MaxEntries = 100;
}


void
TH_ENTRY_LIST::AddEntry(const TH_ENTRY& NewChild) {
  table[Count] = NewChild;
  Count++;
}


void
TH_ENTRY_LIST::GetEntry(const INT4 index, TH_ENTRY* TheChild) {
  if (index <= Count) {
    *TheChild = table[index];
  }
}


TH_ENTRY*
TH_ENTRY_LIST::GetEntry(const INT4 index) {
  if (index <= Count) {
    return(&table[index]);
  }
  return((TH_ENTRY*)NULL);
}


INT4
TH_ENTRY_LIST::GetCount() {
  return(Count);
}


void
TH_ENTRY_LIST::Dump(PFILE fp) {
  INT4 ptr1, ptr2;
  STRING str;
  CHR *ch;

  if (fp) {
    for (INT4 i=0;i<Count;i++) {
      ptr1 = table[i].GetGlobalStart();
      ptr2 = table[i].GetParentPtr();
      table[i].GetString(&str);
      ch = str.NewCString();
      fprintf(fp,"[%d]\t%s, child of %d\n",ptr1,ch,ptr2);
      delete [] ch;
    }
  }
}


void
TH_ENTRY_LIST::WriteTable(PFILE fp) {
  INT4 ptr1,ptr2;
  STRING str;

  if (fp) {
    fwrite((CHR*)&Count,1,sizeof(INT4),fp);
    
    for (INT4 i=0;i<Count;i++) {
      ptr1 = table[i].GetGlobalStart();
      ptr2 = table[i].GetParentPtr();
      fwrite((CHR*)&ptr1,1,sizeof(INT4),fp);
      fwrite((CHR*)&ptr2,1,sizeof(INT4),fp);
    }
  }
}


void
TH_ENTRY_LIST::LoadTable(PFILE fp) {
  INT4 ptr,x;
  STRING str;

  if (fp) {
    x = fread((CHR*)&Count,1,sizeof(INT4),fp);
    if (x == 0)
      return;

    for (INT4 i=0;i<Count;i++) {
      x = fread((CHR*)&ptr,1,sizeof(INT4),fp);
      if (x > 0) {
	table[i].SetGlobalStart(ptr);
	table[i].SetString("");
      } else {
	table[i].SetGlobalStart(0);
      }

      x = fread((CHR*)&ptr,1,sizeof(INT4),fp);
      if (x > 0) {
	table[i].SetParentPtr(ptr);
      } else {
	table[i].SetParentPtr(0);
      }
    }
  } else {
    Count = 0;
  }
}


void 
TH_ENTRY_LIST::Sort() {
  qsort((void *)table, Count, sizeof(TH_ENTRY),EntrySortCmp);
}


void*
TH_ENTRY_LIST::Search(const void* term) {
  void *ptr;
  ptr = bsearch(term, (void *)table, Count, sizeof(TH_ENTRY),EntrySearchCmp);
  return(ptr);
}


TH_ENTRY_LIST::~TH_ENTRY_LIST() {
  delete [] table;
}




/////////////////////////////////////////////////////////////////
// Class: THESAURUS
/////////////////////////////////////////////////////////////////
// This is the search-time constructor
const CHR* DbExtDbSynonyms     = ".syn";
const CHR* DbExtDbSynParents   = ".spx";
const CHR* DbExtDbSynChildren  = ".scx";


// This is the search-time constructor
THESAURUS::THESAURUS(const STRING& DbPathName, const STRING& DbFileName){
  STRING Fn;
  Fn = DbPathName;
  Fn.Cat("/");
  Fn.Cat(DbFileName);

  SetFileName(Fn);

  LoadParents();
  LoadChildren();
  //  cout << "Parents-----------" << endl;
  //  Parents.Dump(stdout);
  //  cout << "Children----------" << endl;
  //  Children.Dump(stdout);
}


// This is the index-time constructor.  It parses the input file and
// creates the synonym table and the indexes for parents and children.
THESAURUS::THESAURUS(const STRING& SourceFileName, const STRING& DbPathName,
		     const STRING& DbFileName) {
  STRING sBuf;
  STRING SynStringFileName, SynParentFileName, SynChildFileName;
  STRING ParentString, ChildString, ThisChild;
  STRLIST ChildrenList;

  STRINGINDEX eq_sign, num_sign;
  CHR *pBuf, *b;
  SIZE_T nChildren;
  SIZE_T ParentGP,ChildOffset;
  FILE *Fp;

  TH_PARENT TheParent;
  TH_ENTRY  TheChild;
  
  // Create the file names for the thesaurus files
  // -- dbname.syn holds the actual text synonyms
  // -- dbname.spx holds the index of parent terms
  // -- dbname.scx holds the index of child terms
  SynStringFileName = DbPathName;
  SynStringFileName.Cat("/");
  SynStringFileName.Cat(DbFileName);

  SetFileName(SynStringFileName);

  // Read in the user-specified file
  sBuf.ReadFile(SourceFileName);

  // Dump it into a character buffer and parse it on newlines
  b = sBuf.NewCString();
  pBuf = strtok(b,"\n");

  // Now, pBuf points to one synonym definition
  ParentGP = 0;

  Fp = OpenSynonymFile("wb");
  if (!Fp)
    return;

  do {
    // Skip leading blanks
    while (*pBuf == ' ')
      pBuf++;

    // Skip comments
    if (*pBuf != '#') {
      //      cout << "Input string ->" << pBuf << "<-\n";

      // Now split the line into parent and children 
      // Make the parent first, trimmed and upper case
      ParentString = pBuf;
      eq_sign = ParentString.Search('=');
      ParentString.EraseAfter(eq_sign-1);
      ParentString.UpperCase();

      // Store the information into a TH_PARENT object
      TheParent.SetString(ParentString);
      TheParent.SetGlobalStart(ParentGP);

      // Put the object into the list of Parent objects
      Parents.AddEntry(TheParent);

      // Make the child string - we have to clean up the terms
      // individually, so it will be kinda slow
      ChildString = pBuf;
      ChildString.EraseBefore(eq_sign+1); // Get rid of the parent
      num_sign = ChildString.Search('#');
      ChildString.EraseAfter(num_sign-1); // Get rid of trailing comments
      ChildString.UpperCase();            // Make it upper case

      // Clean the children by loading into a STRLIST, then looping over
      // each child in the list, trimming off leading and trailing junk
      //
      // Once we have each child, we need to make a TH_ENTRY object out
      // of it so we can store it into the children index

      ChildOffset = ParentGP + ParentString.GetLength() + 1;
      ChildrenList.Split('+',ChildString);
      nChildren = ChildrenList.GetTotalEntries();
      for (SIZE_T x=1;x<=nChildren;x++) {
	ChildrenList.GetEntry(x,&ThisChild);
	ThisChild.Trim();
	ThisChild.TrimLeading();
	ChildrenList.SetEntry(x,ThisChild);

	TheChild.SetString(ThisChild);
	TheChild.SetParentPtr(ParentGP);
	TheChild.SetGlobalStart(ChildOffset);
	//	cout << "Term " << ThisChild << " [" << ChildOffset 
	  //	     << "] is a child of " << ParentString << " [" 
	  //	     << ParentGP << "]" << endl;
	Children.AddEntry(TheChild);
	ChildOffset += ThisChild.GetLength()+1;
      }

      // Make a new string for the synonym file
      ChildrenList.Join("+",&ChildString);
      ParentString += "=";
      ParentString += ChildString;
      //      cout << "Output string->" << ParentString << "<- [" 
		// << ParentGP << "]" << endl;
      ParentString += "\n";

      // Write the entry out to the synonym file
      ParentString.Print(Fp);

      // Update to point to the start of the next line in the file
      ParentGP += ParentString.GetLength();
    }
  } while ( (pBuf = strtok((CHR*)NULL,"\n")) );

  delete [] b;
  fclose(Fp);

  // Write out the index of the parents
  Parents.Sort();

  Fp = OpenParentsFile("wb");
  if (Fp) {
    Parents.WriteTable(Fp);
    fclose(Fp);
  }

  // Write out the index of the children
       //  Children.Dump(stdout);
  Children.Sort();
  //  cout << "------------" << endl;
  //  Children.Dump(stdout);
  Fp = OpenChildrenFile("wb");
  if (Fp) {
    Children.WriteTable(Fp);
    fclose(Fp);
  }
}


void
THESAURUS::SetFileName(const STRING& Fn) {
  BaseFileName = Fn;
}


void
THESAURUS::GetFileName(STRING* Fn) {
  *Fn = BaseFileName;
}


FILE*
THESAURUS::OpenSynonymFile(const char *mode) {
  STRING Fn;
  FILE* Fp;
  Fn = BaseFileName;
  Fn.Cat(DbExtDbSynonyms);
  Fp = fopen(Fn,mode);
  return(Fp);
}


FILE*
THESAURUS::OpenParentsFile(const char *mode) {
  STRING Fn;
  Fn = BaseFileName;
  Fn.Cat(DbExtDbSynParents);
  return(fopen(Fn,mode));
}


FILE*
THESAURUS::OpenChildrenFile(const char *mode) {
  STRING Fn;
  Fn = BaseFileName;
  Fn.Cat(DbExtDbSynChildren);
  return(fopen(Fn,mode));
}


void
THESAURUS::GetIndirectString(PFILE fp, const INT4 ptr, STRING* term) {
  CHR buf[MAX_SYN_LENGTH+1],*b;

  // Offset into the synonym table and read the row
  fseek(fp,ptr,SEEK_SET);
  fgets(buf,MAX_SYN_LENGTH,fp);
  
  // Get the parent from before the = sign
  b = strtok(buf,"=+\n");
  *term = b;
}


void
THESAURUS::LoadParents() {
  FILE *fp;
  INT4 TheCount,ThePtr;
  TH_PARENT *TheParent;
  STRING b;

  // Load up the starting pointers into the parent list
  fp = OpenParentsFile("rb");
  if (!fp)
    return;
  Parents.LoadTable(fp);
  fclose(fp);

  fp = OpenSynonymFile("rb");
  if (!fp)
    return;
  TheCount = Parents.GetCount();
  for (INT4 i=0;i<TheCount;i++) {
    TheParent = Parents.GetEntry(i); // Get ptr to the entry
    ThePtr = TheParent->GetGlobalStart() ; // get the offset
    GetIndirectString(fp,ThePtr,&b);

    // Save the string
    TheParent->SetString(b);
  } // Done loading parents
}


void
THESAURUS::LoadChildren() {
  FILE *fp;
  INT4 TheCount,ThePtr;
  TH_ENTRY *TheEntry;
  STRING b;

  // Load up the starting pointers into the parent list
  fp = OpenChildrenFile("rb");
  if (!fp)
    return;
  Children.LoadTable(fp);
  fclose(fp);

  //  Children.Dump(stdout);

  fp = OpenSynonymFile("rb");
  if (!fp)
    return;
  TheCount = Children.GetCount();
  for (INT4 i=0;i<TheCount;i++) {
    TheEntry = Children.GetEntry(i); // Get ptr to the entry
    ThePtr = TheEntry->GetGlobalStart() ; // get the offset
    GetIndirectString(fp,ThePtr,&b);

    // Save the string
    TheEntry->SetString(b);
  } // Done loading children
}


GDT_BOOLEAN 
THESAURUS::MatchParent(const STRING& ParentTerm, INT4 *ptr) {
  GDT_BOOLEAN matched=GDT_FALSE;
  STRING TheTerm;
  TH_PARENT *p;
  CHR *s;

  TheTerm = ParentTerm;
  TheTerm.UpperCase();
  s = TheTerm.NewCString();
  p = (TH_PARENT*)Parents.Search(s);
  if (p) {
    *ptr = p->GetGlobalStart();
    matched=GDT_TRUE;
  }
  delete [] s;
  return(matched);
}


// Given the term, go get a list of child terms.  Always return at least
// the original term in the list, so it is always safe to use the 
// returned value
void
THESAURUS::GetChildren(const STRING& ParentTerm, STRLIST* Children) {
  INT4 ptr;
  FILE *fp;
  CHR buf[MAX_SYN_LENGTH+1];
  STRING TheEntry;

  if (MatchParent(ParentTerm,&ptr)) {
    fp = OpenSynonymFile("rb");
    if (!fp)
      return;
    fseek(fp,ptr,SEEK_SET);
    fgets(buf,MAX_SYN_LENGTH,fp);
    TheEntry = buf;
    TheEntry.Replace("=","+");
    Children->Split('+',TheEntry);
  } else {
    Children->Clear();
    Children->AddEntry(ParentTerm);
  }
}


// Tell the caller if there is a match in the list of children terms
GDT_BOOLEAN 
THESAURUS::MatchChild(const STRING& Term, INT4 *ptr) {
  GDT_BOOLEAN matched=GDT_FALSE;
  STRING TheTerm;
  TH_ENTRY *p;
  CHR *s;

  TheTerm = Term;
  TheTerm.UpperCase();
  s = TheTerm.NewCString();
  p = (TH_ENTRY*)Children.Search(s);
  if (p) {
    *ptr = p->GetParentPtr();
    matched=GDT_TRUE;
  }
  delete [] s;
  return(matched);
}


// Given the term, look for the parent term.  If no parent term is found,
// return the original term, so it is always safe to use the returned value
void
THESAURUS::GetParent(const STRING& ChildTerm, STRING* TheParent) {
  INT4 ptr;
  FILE *fp;
  STRING TheEntry;

  if (MatchChild(ChildTerm,&ptr)) {
    fp = OpenSynonymFile("rb");
    if (!fp)
      return;
    GetIndirectString(fp,ptr,TheParent);
  } else {
    *TheParent=ChildTerm;
  }
}


THESAURUS::~THESAURUS() {
}

#ifdef MAIN
int
main(int argc, char** argv) {
  STRING Flag;
  INT x=0;
  STRING SynonymFileName;
  STRING DbPath,DbName;
  GDT_BOOLEAN HaveSynonyms=GDT_FALSE;
  INT LastUsed = 0;
  STRLIST Children;

  STRING ParentTerm,ChildTerm;
  ParentTerm = "spatial";
  ChildTerm = "terrestrial";

  DbPath = "/tmp";
  DbName = "test";

  while (x < argc) {
    if (argv[x][0] == '-') {
      Flag = argv[x];
      if (Flag.Equals("-syn")) {
        if (++x >= argc) {
          fprintf(stderr,
                  "ERROR: No synonym file name specified after -syn.\n\n");
          EXIT_ERROR;
        }
        SynonymFileName = argv[x];
        HaveSynonyms = GDT_TRUE;
        LastUsed = x;
      }
    }
    x++;
  }

  if (HaveSynonyms) {
    THESAURUS *MyThesaurus;
  
    // Build a new thesaurus
    MyThesaurus = new THESAURUS(SynonymFileName,DbPath,DbName);
    delete MyThesaurus;

    // Load an existing thesaurus
    cout << "-------" << endl;
    MyThesaurus = new THESAURUS(DbPath,DbName);

    // Look for the children matching a parent term
    MyThesaurus->GetChildren(ParentTerm,&Children);
    cout << "Children for " << ParentTerm << ":" << endl;
    Children.Dump(stdout);
    cout << "-------" << endl;
    MyThesaurus->GetParent(ChildTerm,&ParentTerm);
    cout << "Parent of " << ChildTerm << " is " << ParentTerm << endl;
    // Clean up
    delete MyThesaurus;

  }
  EXIT_ZERO;
}

#endif
