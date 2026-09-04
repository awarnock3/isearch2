/* $Id: Iutil.cxx,v 1.26 2001/03/06 04:14:16 cnidr Exp $ */
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1994-2001. 

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
File:		Iutil.cxx
Version:	1.02
$Revision: 1.26 $
Description:	Command-line utilities for Isearch databases
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#define GILS_DIRNAME "gils.out"

#include <stdlib.h>
#include <fcntl.h>
#ifdef UNIX
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <locale.h>

#if defined(_MSDOS) || defined(_WIN32)
#include <direct.h>
#else
#include <glob.h>
#endif

#include <ctype.h>

#include "isearch.hxx"

#include "common.hxx"
#include "dtreg.hxx"
#include "rcache.hxx"
#include "index.hxx"
#include "fprec.hxx"
#include "fpt.hxx"
#include "registry.hxx"
#include "idb.hxx"

/// IDB with a silent IndexingStatus() override; Iutil doesn't index, so it has no progress to report.
class IDBC : public IDB {
public:
	IDBC(const STRING& NewPathName, const STRING& NewFileName,
	     const STRLIST& NewDocTypeOptions)
		:	IDB(NewPathName, NewFileName, NewDocTypeOptions) {};

protected:
	void IndexingStatus(const INT StatusMessage, const STRING *FileName,
			    const INT WordCount) const {};
};

typedef IDBC* PIDBC;

/**
 * @brief Removes the .mdt/.num files IDB::KillAll() leaves behind for
 * database @p db (worked around here rather than in ~IDB(), per the
 * comment below).
 * @param db Database root name/path passed to KillAll().
 */
void
cleanupAfterKillAll(const STRING& db) {
  // for some reason these files are not getting deleted
  // by KillAll() or are being written out again,
  // probably in ~IDB().
#if !defined(_MSDOS) && !defined (WIN32)
  STRING temp = db;
  temp.Cat(".mdt");
  StrUnlink(temp);
  temp = db;
  temp.Cat(".num");
  StrUnlink(temp);
#endif
}


/**
 * @brief Iutil CLI entry point: parses flags (see the usage text printed
 * below when @p argc < 2) and dispatches to the corresponding IDB
 * maintenance operation(s) against the database named by -d — optimize/
 * collapse indexes, erase, view info/fields/documents, delete/undelete
 * by key, cleanup, generate GILS metadata, or replace one database with
 * another.
 * @param argc Argument count, including the program name.
 * @param argv Argument vector; argv[1..] are the flags/values described
 * in the usage text.
 * @return 0 on completion or a handled usage/database error (via
 * RETURN_ZERO); 1 if -replace's source and destination are the same
 * database.
 */
int
main(int argc, char** argv) {
  fprintf(stderr,"Iutil v%s\n", IsearchVersion);
  if (argc < 2) {
    fprintf(stderr,"Copyright (c) 1995-2001 MCNC/CNIDR and A/WWW Enterprises\n");
    fprintf(stderr,"-d (X)       # Use (X) as the root name for database files.\n");
    fprintf(stderr,"-V           # Print the version number.\n");
    fprintf(stderr,"-vi          # View summary information about the database.\n");
    fprintf(stderr,"-vf          # View list of fields defined in the database.\n");
    fprintf(stderr,"-v           # View list of documents in the database.\n");
    fprintf(stderr,"-newpaths    # Prompt for new pathnames for files.\n");
    fprintf(stderr,"-del         # Mark individual documents (by key) to be deleted from database.\n");
    fprintf(stderr,"-undel       # Unmark documents (by key) that were marked for deletion.\n");
#ifdef DICTIONARY
    fprintf(stderr,"-dict        # Generate a search dictionary for the index.\n");
    fprintf(stderr,"-centroid    # Create a centroid document for the database.\n");
#endif
    fprintf(stderr,"-c           # Cleanup database by removing unused data (useful after -del).\n");
    //    cout << "      [-collapse] // Collapse last two index files."<<endl;
    fprintf(stderr,"-erase       # Erase the entire database.\n");
    fprintf(stderr,"-gt (X)      # Set (X) as the global document type for the database.\n");
    fprintf(stderr,"-gt0         # Clear the global document type for the database.\n");
    fprintf(stderr,"-optimize    # Optimize database indexes.\n");
    fprintf(stderr,"-m (X)       # Load (X) megabytes of data at a time for optimizing\n");
    fprintf(stderr,"-gilsdocs    # Generate GILS metadata records in XML format for each document\n");
    fprintf(stderr,"-meta (X)    # Read default metadata from XML file (X)\n");
    fprintf(stderr,"-gilsindex   # Generate GILS metadata records in XML format for the database\n");
    fprintf(stderr,"             # (default=./meta.xml).\n");
    fprintf(stderr,"-replace (X) # Replace database (X) with the database specified by -d.\n");
    fprintf(stderr,"-state       # Print the state of the database.\n");
    fprintf(stderr,"-urn         # Generate a table of URNs.\n");
    fprintf(stderr,"-o (X)       # Document type specific option.\n");
    fprintf(stderr,"Example:  Iutil -d POETRY -erase\n");
    fprintf(stderr,"Document Types Supported:");
    DTREG dtreg(0);
    STRLIST DocTypeList;
    dtreg.GetDocTypeList(&DocTypeList);
    STRING s;
    INT x;
    INT y = DocTypeList.GetTotalEntries();
    for (x=1; x<=y; x++) {
      DocTypeList.GetEntry(x, &s);
      fprintf(stderr,"\t ");
      s.Print(stderr);
    }
    fprintf(stderr,"\n");
    RETURN_ZERO;
  }

  if (!setlocale(LC_CTYPE,"")) {
    fprintf(stderr,"Warning: Failed to set the locale!\n");
  }

  STRLIST DocTypeOptions;
  STRING GlobalDoctype;
  INT SetGlobalDoctype = 0;
  CHR Cwd[256];
  if (!getcwd(Cwd, 255)) {
    strcpy(Cwd, ".");
  }
  STRING Flag;
  STRING DBName;
  STRING MetaFn;
  STRING Replace;
  MetaFn = "./meta.xml";
  STRING Temp;
  //  STRLIST WordList;
  INT DebugFlag = 0;
  INT Skip = 0;
#ifdef DICTIONARY
  INT DictGen = 0;
  INT DoCentroid = 0;
#endif
  INT EraseAll = 0;
  INT PathChange = 0;
  INT DeleteByKey = 0;
  INT UndeleteByKey = 0;
  INT Cleanup = 0;
  INT Gils = 0;
  INT GilsIndex = 0;
  INT DbState = 0;
  INT OptimizerMemory=1; // 1 MB by default
			      INT View = 0;
  INT ViewInfo = 0;
  INT ViewFields = 0;
  INT Optimize = 0;
  INT Collapse=0;
  INT Urn=0;
  //	INT Recursive = 0;
  //	INT AppendDb = 0;
  //	UINT4 MemoryUsage = 0;
  INT x = 0;
  INT LastUsed = 0;
  while (x < argc) {
    if (argv[x][0] == '-') {
      Flag = argv[x];
      if (Flag.Equals("-o")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No option specified after -o.\n");
	  RETURN_ZERO;
	}
	STRING S;
	S = argv[x];
	DocTypeOptions.AddEntry(S);
	LastUsed = x;
      }
      if(Flag.Equals("-optimize")){
	Optimize=1;
	LastUsed=x;
      }
      if(Flag.Equals("-collapse")){
	Collapse=1;
	LastUsed=x;
      }
      if (Flag.Equals("-d")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No database name specified after -d.\n");
	  RETURN_ZERO;
	}
	DBName = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-meta")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No file name specified after -meta.\n");
	  RETURN_ZERO;
	}
	MetaFn = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-replace")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No file name specified after -replace.\n");
	  RETURN_ZERO;
	}
	Replace = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-gt")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No document type specified after -gt.\n");
	  fprintf(stderr,"       Use -gt0 if you want no document type.\n");
	  RETURN_ZERO;
	}
	GlobalDoctype = argv[x];
	SetGlobalDoctype = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-m")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No memory size specified after -m.\n");
					
	  RETURN_ZERO;
	}
	OptimizerMemory = atoi(argv[x]);
	printf("%i MB Memory Selected\n", OptimizerMemory);
	//	OptimizerMemory=OptimizerMemory*1024*1024;
	LastUsed = x;
      }
      if (Flag.Equals("-gt0")) {
	GlobalDoctype = "";
	SetGlobalDoctype = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-debug")) {
	DebugFlag = 1;
	if (x+1 < argc) {
	  Temp = argv[x+1];
	  Temp.GetCString(Cwd, 256);
	  if (isdigit(Cwd[0])) {
	    Skip = Temp.GetInt();
	    x++;
	  }
	}
	LastUsed = x;
      }
#ifdef DICTIONARY
      if (Flag.Equals("-dict")) {
	DictGen = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-centroid")) {
	DoCentroid = 1;
	LastUsed = x;
      }
#endif
      if (Flag.Equals("-erase")) {
	EraseAll = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-newpaths")) {
	PathChange = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-v")) {
	View = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-vf")) {
	ViewFields = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-vi")) {
	ViewInfo = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-del")) {
	DeleteByKey = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-undel")) {
	UndeleteByKey = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-V")) {
	RETURN_ZERO;
      }
      if (Flag.Equals("-c")) {
	Cleanup = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-gilsdocs")) {
	Gils = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-gilsindex")) {
	GilsIndex = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-state")) {
	DbState = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-urn")) {
	Urn = 1;
	LastUsed = x;
      }
    }
    x++;
  }
	
  if (DBName.Equals("")) {
    DBName = IsearchDefaultDbName;
  }
	
  x = LastUsed + 1;
	
  // we need to prevent bad combinations of options, such as -erase and -del together
	
  PIDBC pdb;
  STRING DBPathName, DBFileName;
	
  if (!DBExists(DBName)) {
    fprintf(stderr,"Database ");
    DBName.Print(stderr);
    fprintf(stderr," does not exist.\n");
    RETURN_ZERO;
  }
	
  struct stat info;
  STRING IndexFile;
	
  DBPathName = DBName;
  DBFileName = DBName;
  RemovePath(&DBFileName);
  RemoveFileName(&DBPathName);
  pdb = new IDBC(DBPathName, DBFileName, DocTypeOptions);
  OptimizerMemory=OptimizerMemory*1024*1024; // in bytes
	
  if (DebugFlag) {
    pdb->DebugModeOn();
  }
  if (Optimize) {
    IndexFile = DBName;
    IndexFile.Cat(".inx.1");
    PCHR CheckName;
    CheckName = IndexFile.NewCString();
    if (stat(CheckName, &info) !=0) {
      fprintf(stderr,"Database ");
      DBName.Print(stderr);
      fprintf(stderr," does not need optimizing.\n");
      delete [] CheckName;
      // BUGFIX #1 (docs/BUG_CATALOG.md#srciutilcxx): pdb (allocated
      // above) was leaked on this early return -- every other exit
      // path in main() either deletes pdb first or hasn't allocated it
      // yet.
      delete pdb;
      RETURN_ZERO;
    } else {
      delete [] CheckName;
      pdb->MergeIndexFiles(OptimizerMemory);
    }
  }
  if (Collapse) {
    IndexFile = DBName;
    IndexFile.Cat(".inx.1");
    PCHR CheckName;
    CheckName = IndexFile.NewCString();
    if (stat(CheckName, &info) !=0) {
      fprintf(stderr,"Database ");
      DBName.Print(stderr);
      fprintf(stderr," cannot be collapsed.\n");
      delete [] CheckName;
      // BUGFIX #2 (docs/BUG_CATALOG.md#srciutilcxx): same pdb leak as
      // BUGFIX #1 above, mirrored in the -collapse branch.
      delete pdb;
      RETURN_ZERO;
    } else {
      delete [] CheckName;
      pdb->CollapseIndexFiles(OptimizerMemory);
    }
  }
  if (ViewInfo) {
    STRING S;
    INT x, y, z;
    pdb->GetDbFileStem(&S);
    printf("Database name: ");
    S.Print();
    printf("\n");
    pdb->GetGlobalDocType(&S);
    if (S == "") {
      S = "(none)";
    }
    printf("Global document type: ");
    S.Print();
    printf("\n");
    y = pdb->GetTotalRecords();
    printf("Total number of documents: %i\n", y);
    z = 0;
    for (x=1; x<=y; x++) {
      if (pdb->GetDocumentDeleted(x)) {
	z++;
      }
    }
    printf("Documents marked as deleted: %i\n", z);
  }
	
  if (DbState) {
    INT4 DbState = pdb->GetDbState();
    switch (DbState) {
    case IsearchDbStateReady:
      printf("ready\n");
      break;
    case IsearchDbStateBusy:
      printf("busy\n");
      break;
    case IsearchDbStateInvalid:
      printf("invalid\n");
      break;
    default:
      printf("unknown\n");
    }
    delete pdb;
    RETURN_ZERO;
  }

  if (!pdb->IsDbCompatible()) {
    fprintf(stderr,"The specified database is not compatible with this version of Iutil.\n");
    fprintf(stderr,"Please use matching versions of Iindex, Isearch, and Iutil.\n");
    delete pdb;
    RETURN_ZERO;
  }
  
  if (SetGlobalDoctype) {
    pdb->SetGlobalDocType(GlobalDoctype);
    if (GlobalDoctype == "") {
      printf("Global document type cleared.\n");
    } else {
      GlobalDoctype.UpperCase();
      printf("Global document type set to ");
      GlobalDoctype.Print();
      printf(".\n");
    }
  }
  
  if (EraseAll) {
    printf("Erasing database files ...\n");
    pdb->KillAll();
    delete pdb;

    cleanupAfterKillAll(DBName);

    printf("Database files erased.\n");
    RETURN_ZERO;
  }
  
  if (PathChange) {
    printf("Scanning database for file paths ...\n");
    printf("Enter new path or <Return> to leave unchanged:\n");
    INT x, y;
    RECORD Record;
    PCHR p;
    STRING OldPath, NewPath;
    STRLIST PathList;
    CHR s[512];
    y = pdb->GetTotalRecords();
    for (x=1; x<=y; x++) {
      pdb->GetDocumentInfo(x, &Record);
      Record.GetPathName(&OldPath);
      p = OldPath.NewCString();
      PathList.GetValue(p, &NewPath);
      delete [] p;
      if (NewPath == "") {
	printf("Path=[");
	OldPath.Print();
	printf("]\n");
	printf("    > ");
				//gets(s);
	if (!fgets(s,511,stdin)) {
	  s[0] = '\0';
	}
	INT slen;
	slen = strlen(s);
	if ((slen > 0) && (s[slen-1] == '\n')) {
	  s[slen-1] = '\0';  //GCMD  chop off '\n' from the end. 
        }
	if (s[0] == '\0') {
	  NewPath = OldPath;
	} else {
	  NewPath = s;
	}
	Record.SetPathName(NewPath);
	OldPath += "=";
	OldPath += NewPath;
	PathList.AddEntry(OldPath);
      } else {
	Record.SetPathName(NewPath);
      }
      pdb->SetDocumentInfo(x, Record);
    }
    printf("Done.\n");
  }
  /*  
      // Replaced with new versions from J. Wehle
      if (DeleteByKey) {
      cout << "Marking documents as deleted ..." << endl;
      INT x, z;
      INT y = 0;
      STRING S;
      z = WordList.GetTotalEntries();
      for (x=1; x<=z; x++) {
      WordList.GetEntry(x, &S);
      y += pdb->DeleteByKey(S);
      }
      cout << y << " document(s) marked as deleted." << endl;
      }
	    
      if (UndeleteByKey) {
      cout << "Removing deletion mark from documents ..."  << endl;
      INT x, z;
      INT y = 0;
      STRING S;
      z = WordList.GetTotalEntries();
      for (x=1; x<=z; x++) {
      WordList.GetEntry(x, &S);
      y += pdb->UndeleteByKey(S);
      }
      cout << "Deletion mark removed for " << y << " document(s)." << endl;
      }
  */
  if (DeleteByKey) {
    printf("Marking documents as deleted ...\n");
    INT NumWords = argc - x;
    INT y = 0;
    for (INT z = 0; z < NumWords; z++) {
      y += pdb->DeleteByKey(argv[z+x]);
    }
    printf("%i document(s) marked as deleted.\n", y);
  }
	
  if (UndeleteByKey) {
    printf("Removing deletion mark from documents ...\n");
    INT NumWords = argc - x;
    INT y = 0;
    for (INT z = 0; z < NumWords; z++) {
      y += pdb->UndeleteByKey(argv[z+x]);
    }
    printf("Deletion mark removed for %i document(s).\n", y);
  }
	
  if (Cleanup) {
    printf("Cleaning up database (removing deleted documents) ...\n");
    INT x = pdb->CleanupDb();
    printf("%i document(s) were removed.\n", x);
  }

#ifdef DICTIONARY 
  if (DictGen) {
    printf("Creating dictionary ...\n");
    pdb->CreateDictionary();
  }
   
  if (DoCentroid) {
    printf("Generating centroid document ...\n");
    pdb->CreateCentroid();
  }
#endif

  if (ViewFields) {
    printf("The following fields are defined in this database:\n");
    DFDT Dfdt;
    DFD Dfd;
    STRING S;
    pdb->GetDfdt(&Dfdt);
    INT y = Dfdt.GetTotalEntries();
    INT x;
    for (x=1; x<=y; x++) {
      Dfdt.GetEntry(x, &Dfd);
      Dfd.GetFieldName(&S);
      S.Print();
      printf("\n");
    }
  }
	
  if (View) {
    printf("DocType: [Key] (Start - End) File\n");
    printf("(* indicates deleted record)\n");
    RECORD Record;
    STRING S;
    INT y = pdb->GetTotalRecords();
    INT x;
    for (x=1; x<=y; x++) {
      pdb->GetDocumentInfo(x, &Record);
      Record.GetDocumentType(&S);
      if (S.Equals("")) {
	printf("(none)");
      } else {
	S.Print();
      }
      printf(": [");
      Record.GetKey(&S);
      S.Print();
      printf("] ");
      printf("(%i - %i) ", Record.GetRecordStart(),
	     Record.GetRecordEnd());
      Record.GetFullFileName(&S);
      S.Print();
      if (pdb->GetDocumentDeleted(x)) {
	printf(" *");
      }
      printf("\n");
    }
  }

  if (Urn) {
    RECORD Record;
    STRING S,FullDbName;
    INT y = pdb->GetTotalRecords();
    INT x;
    
    FullDbName = DBPathName;
    FullDbName.Cat(DBFileName);

    for (x=1; x<=y; x++) {
      pdb->GetDocumentInfo(x, &Record);
      FullDbName.Print();
      printf("/");
      Record.GetKey(&S);
      S.Print();
      printf("\t");
      Record.GetFullFileName(&S);
      S.Print();
      printf("\n");
    }
  }

  if (Gils) {
    printf("Generating GILS records ...\n");
    printf("Files will be placed in %s/ ...\n", GILS_DIRNAME);
    printf("Any default values in ");
    MetaFn.Print();
    printf(" will be included.\n");
    // parse defaults file
    REGISTRY* metadef = parseMetaDefaults(MetaFn);
    // BUGFIX #3 (docs/BUG_CATALOG.md#srciutilcxx): metadef and each
    // loop iteration's meta below were both leaked -- see the deletes
    // added at the end of this block and inside the loop.
    // generate gils records
#ifdef UNIX
    mkdir(GILS_DIRNAME, 0777);
#else
    mkdir(GILS_DIRNAME);
#endif
    INT t = pdb->GetTotalRecords();
    INT x;
    RECORD record;
    STRING doctype;
    DOCTYPE* dtp;
    //		STRING metadata;
    FILE* fp;
    STRING fn;
    STRING recfn;
    char* p;
    REGISTRY* meta;
    for (x = 1; x <= t; x++) {
      pdb->GetDocumentInfo(x, &record);
      record.GetDocumentType(&doctype);
      dtp = pdb->GetDocTypePtr(doctype);
      meta = dtp->GetMetadata(record, "gils", metadef);
      fn = GILS_DIRNAME;
      fn += "/";
      record.GetFileName(&recfn);
      fn += recfn;
      fn += ".xml";
      p = fn.NewCString();
      fp = fopen(p, "w");
      delete [] p;
      if (fp) {
	fprintf(fp, "<?XML VERSION=\"1.0\" ENCODING=\"UTF-8\" ?>\n");
	fprintf(fp, "<!DOCTYPE Locator SYSTEM \"xml.dtd\" >\n");
	STRLIST position;
	meta->PrintSgml(fp, position);
	fclose(fp);
      }
      delete meta;
    }
    delete metadef;
  }

  if (GilsIndex) {
    printf("Creating GILS metadata...\n");
    STRING GilsBuffer,GilsFile;
    MakeDbGilsRec(pdb, DBPathName, DBFileName,&GilsBuffer);
    GilsFile = DBName;
    GilsFile.Cat(".gils");
    GilsBuffer.WriteFile(GilsFile);
    FILE* fp = fopen(GilsFile, "a");
    // BUGFIX #4 (docs/BUG_CATALOG.md#srciutilcxx): fp was used
    // unconditionally, with no check for a failed fopen() (e.g. an
    // unwritable directory or a full disk) -- a null-pointer
    // dereference on WriteCentroid()/fprintf() below.
    if (fp) {
      pdb->WriteCentroid(fp);
      fprintf(fp, "</Locator>\n");
      fclose(fp);
    } else {
      perror(GilsFile);
    }
  }

  delete pdb;
  if ( ! Replace.Equals("") ) {
    STRING source, dest;
    source = DBName;
    dest = Replace;
    ExpandFileSpec(&source);
    ExpandFileSpec(&dest);
    if (source.Equals(dest)) {
      printf("Both databases are the same; replace aborted.\n");
      return 1;
    }
    printf("Replacing ");
    dest.Print();
    printf(" with ");
    source.Print();
    printf(" ...\n");
    // do the replacement
    // remove the database to be replaced
    STRING path, file;
    path = dest;
    file = dest;
    RemoveFileName(&path);
    RemovePath(&file);
    IDB* idb = new IDB(path, file);
    idb->KillAll();
    delete idb;
    cleanupAfterKillAll(dest);
    // move the new database
    // for now we move DB.* since there is no good way to
    // get a list of files from IDB.
    RemoveFileName(&dest);
#if !defined(_MSDOS) && !defined (WIN32)
    STRING sourcePattern = source;
    sourcePattern.Cat(".*");
    CHR *pattern = sourcePattern.NewCString();
    glob_t matches;
    memset(&matches, 0, sizeof(matches));
    const int status = glob(pattern, 0, nullptr, &matches);
    if (status == 0) {
      AddTrailingSlash(&dest);
      for (size_t i = 0; i < matches.gl_pathc; ++i) {
        STRING from = matches.gl_pathv[i];
        STRING base = from;
        RemovePath(&base);
        STRING to = dest;
        to.Cat(base);
        CHR *fromPath = from.NewCString();
        CHR *toPath = to.NewCString();
        if (rename(fromPath, toPath) != 0) {
          perror(fromPath);
        }
        delete [] fromPath;
        delete [] toPath;
      }
    } else if (status != GLOB_NOMATCH) {
      perror("glob");
    }
    globfree(&matches);
    delete [] pattern;
#endif
  }
	

  RETURN_ZERO;
}
