/* $Id: Iindex.cxx,v 1.33 2001/03/06 04:14:16 cnidr Exp $ */
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
File:		Iindex.cxx
Version:	1.03
$Revision: 1.33 $
Description:	Command-line indexer
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#if defined(_MSDOS) || defined(_WIN32)
#include <direct.h>
#define NO_MMAP
#else
#include <unistd.h>
#endif

#ifndef NO_MMAP
#include <sys/types.h>
#include <sys/mman.h>
#endif

#include "isearch.hxx"

#include "common.hxx"
#include "dtreg.hxx"
#include "rcache.hxx"
#include "index.hxx"
#include "fprec.hxx"
#include "fpt.hxx"
#include "registry.hxx"
#include "idb.hxx"
#include "thesaurus.hxx"

class IDBC : public IDB {
public:
  IDBC(const STRING& NewPathName, const STRING& NewFileName, 
       const STRLIST& NewDocTypeOptions) 
    : IDB(NewPathName, NewFileName, NewDocTypeOptions) { };

protected:
  void IndexingStatus(const INT StatusMessage, const STRING *FileName,
		      const INT Count) const 
    {
      switch (StatusMessage) {
      case (IndexingStatusParsingFiles):
	printf("   Parsing files ...\n");
	break;
      case (IndexingStatusParsingDocument):
	printf("   Parsing ");
	FileName->Print();
	//	printf(" ...\n");
	break;
      case (IndexingStatusIndexing):
	printf("   Indexing %i words ...\n", Count);
	break;
      case (IndexingStatusMerging):
	printf("   Merging index ...\n");
	break;
      case (IndexingStatusKeySet):
	printf(", key=");
	FileName->Print();
	printf("\n");
	break;
      }
    };
};

typedef IDBC* PIDBC;

STRING Separator;
STRING DocumentType;
UINT4 MemoryUsage = 1;


void 
AddFile(PIDB IdbPtr, STRING& PathName, STRING& FileName) {
  RECORD Record;
  Record.SetPathName(PathName);
  Record.SetFileName(FileName);
  Record.SetDocumentType(DocumentType);
  if (Separator.Equals("")) {
    Record.SetRecordStart(0);
    Record.SetRecordEnd(0);
    IdbPtr->AddRecord(Record);
  } else {
#ifndef NO_MMAP
    STRING Fn;
    Record.GetFullFileName(&Fn);
    FILE* Fp = fopen(Fn, "rb");
    if (!Fp) {
      return;
    }
    LONG FileSize = GetFileSize(Fp);
    INT FileDesc = fileno(Fp);
    CHR* Buffer;
    
    Buffer = (CHR*) mmap((caddr_t)0, FileSize, PROT_READ, MAP_PRIVATE,
			 FileDesc, 0);
    Record.SetRecordStart(0);
    CHR* Position = Buffer;
    CHR* Found=(CHR*)NULL;
    GDT_BOOLEAN Done;
    CHR* EndOfBuffer = Buffer + FileSize;
    CHR* Sep = Separator.NewCString();
    CHR SepChar = Sep[0];
    GPTYPE Offset;
    SIZE_T SepLength = Separator.GetLength();
    
    do {
      Done = GDT_FALSE;
      while (Done == GDT_FALSE) {
	while ( (Position < EndOfBuffer) &&
		(*Position != SepChar) ) {
	  Position++;
	}
	if (Position >= EndOfBuffer) {
	  Done = GDT_TRUE;
	  Found = 0;
	} else {
	  if ((Position + SepLength) <= EndOfBuffer) {
	    if (strncmp(Sep, Position, SepLength) == 0) {
	      Found = Position;
	      Done = GDT_TRUE;
	    }
	  }
	}
	if (Done == GDT_FALSE) {
	  Position++;
	}
      }
      if (Found) {
	Offset = (GPTYPE)((UINT4)Found - (UINT4)Buffer);
	/* the separator marks the beginning of the next 
	   record. (offset - 1), then marks the end of 
	   the current record. we must make sure that the
	   end of the current record is past the beginning 
	   of the current record.
	   
	   Don't do this for the start of the file where
	   Offset = 0, 'cause Offset is unsigned and you'll get
	   in big trouble when you compute Offset-1!
	*/
	if ( Offset == 0 )
	  Position++;
	else if ( (Offset - 1) > Record.GetRecordStart()) {
	  Record.SetRecordEnd(Offset -1);
	  IdbPtr->AddRecord(Record);
	  Record.SetRecordStart(Offset);
	  Position = Found + strlen(Sep);
	} else {
	  Position++;
	}
      }
    } while (Found);
    if ((FileSize - 1) > Record.GetRecordStart()) {
      //      Record.SetRecordEnd(FileSize);  // I think this is right - aw3
      Record.SetRecordEnd(FileSize-1);
      IdbPtr->AddRecord(Record);
    }
    delete [] Sep;
    fclose(Fp);
#else
    GPTYPE Start = 0;
    GPTYPE Position = 0;
    GPTYPE SavePosition = 0;
    GPTYPE RecordEnd;
    GPTYPE C;
    STRING Fn;
    CHR Ch, Sch;
    STRINGINDEX Slen = Separator.GetLength();
    PCHR Buffer = new CHR[Slen+1];
    Sch = Separator.GetChr(1);
    Record.GetFullFileName(&Fn);
    PFILE Fp = fopen(Fn, "rb");
    if (!Fp) {
      return;
    }
    while (fread(&Ch, 1, 1, Fp) == 1) {
      SavePosition = Position;
      Position++;
      if (Ch == Sch) {
	*Buffer = Ch;
	C = fread(Buffer + 1, 1, Slen - 1, Fp);
	Position += C;
	Buffer[C+1] = '\0';
	if (Separator.Equals(Buffer)) {
	  Record.SetRecordStart(Start);
	  if (SavePosition == 0) {
	    RecordEnd = 0;
	  } else {
	    RecordEnd = SavePosition - 1;
	  }
	  if (RecordEnd > Start) {
	    Record.SetRecordEnd(RecordEnd);
	    IdbPtr->AddRecord(Record);
	    Start = SavePosition;
	  }
	} else {
	  // Rewind, so we won''t skip over
	  // the beginning of a separator string.
	  // (thanks to Jae W. Chang for this fix)
	  Position -= C;
	  //	  fseek(Fp, Position, 0);
	  fseek(Fp, Position, SEEK_SET);
	}
      }
    }
    Record.SetRecordStart(Start);
    if (SavePosition == 0) {
      RecordEnd = 0;
    } else {
      RecordEnd = Position - 1;
    }
    if (RecordEnd > Start) {
      Record.SetRecordEnd(RecordEnd);
      IdbPtr->AddRecord(Record);
    }
    fclose(Fp);
    delete [] Buffer;
#endif
  }
}



int 
main(int argc, char** argv) {
  fprintf(stderr,"Iindex v%s\n", IsearchVersion);
  if (argc < 2) {
    fprintf(stderr,"Copyright (c) 1995-2001 MCNC/CNIDR and A/WWW Enterprises\n");
    fprintf(stderr,"-d (X)  # Use (X) as the root name for database files.\n");
    //cout << "       [-a]     // Add to existing database, instead of replacing it." << endl;
    fprintf(stderr,"-V  # Print the version number.\n");
    fprintf(stderr,"-m (X)  # Load (X) megabytes of data at a time for indexing\n");
    fprintf(stderr,"        # (default=1).\n");
    fprintf(stderr,"-s (X)  # Treat (X) as a separator for multiple documents within\n");
    fprintf(stderr,"        # a single file.\n");
    fprintf(stderr,"-t (X)  # Index as files of document type (X).\n");
    fprintf(stderr,"-f (X)  # Read list of file names to be indexed from file (X).\n");
    fprintf(stderr,"-r      # Recursively descend subdirectories.\n");
    fprintf(stderr,"-meta (X)  # Read default metadata from XML file (X)\n");
    fprintf(stderr,"           # (default=./meta.xml).\n");
    fprintf(stderr,"-gils   # Create GILS metadata for the index.\n");
    fprintf(stderr,"-merge  # Merge subindexes now.\n");
    fprintf(stderr,"-syn (X)# File (X) contains term synonyms.\n");
    fprintf(stderr,"-o (X)  # Document type specific option.\n");
    fprintf(stderr,"(X) (Y) (...)  # Index files (X), (Y), etc.\n");
    fprintf(stderr,"Examples:  Iindex -d POETRY *.doc *.txt\n");
    fprintf(stderr,"           Iindex -d WEBPAGES -t SGMLTAG *.html\n");
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
    return 0;
  }

  if (!setlocale(LC_CTYPE,"")) {
    fprintf(stderr,"Warning: Failed to set the locale!\n");
  }

  STRLIST DocTypeOptions;
  CHR Cwd[256];
  getcwd(Cwd, 255);
  STRING Flag;
  STRING DBName;
  STRING MetaFn;
  MetaFn = "./meta.xml";
  STRING FileList;
  INT DebugFlag = 0;
  INT Recursive = 0;
  INT AppendDb = 0;
  GDT_BOOLEAN Merge=GDT_FALSE;
  GDT_BOOLEAN GILS=GDT_FALSE;
  INT x = 0;
  INT LastUsed = 0;
  STRING SynonymFileName;
  GDT_BOOLEAN Synonyms=GDT_FALSE;
	
  while (x < argc) {
    if (argv[x][0] == '-') {
      Flag = argv[x];
      if (Flag.Equals("-o")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No option specified after -o.\n\n");
	  RETURN_ZERO;
	}
	STRING S;
	S = argv[x];
	DocTypeOptions.AddEntry(S);
	LastUsed = x;
      }
      if (Flag.Equals("-d")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No database name specified after -d.\n\n");
	  RETURN_ZERO;
	}
	DBName = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-f")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No file name specified after -f.\n\n");
	  RETURN_ZERO;
	}
	FileList = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-t")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No document type name specified after -dt.\n\n");
	  RETURN_ZERO;
	}
	DocumentType = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-s")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No separator string specified after -s.\n\n");
	  RETURN_ZERO;
	}
	Separator = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-merge")) {
	Merge=GDT_TRUE;
	LastUsed = x;
      }
      if (Flag.Equals("-gils")) {
	GILS=GDT_TRUE;
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
      if (Flag.Equals("-m")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No memory usage specified after -m.\n\n");
	  RETURN_ZERO;
	}
	MemoryUsage = strtol(argv[x], (PCHR*)NULL, 10);
	LastUsed = x;
      }
			
      if (Flag.Equals("-a")) {
	AppendDb = 1;
	LastUsed = x;
      }
			
      if (Flag.Equals("-r")) {
	Recursive = 1;
	LastUsed = x;
      }

      if (Flag.Equals("-V")) {
	RETURN_ZERO;
      }
			
      if (Flag.Equals("-debug")) {
	DebugFlag = 1;
	LastUsed = x;
      }
			
      if (Flag.Equals("-erase")) {
	fprintf(stderr,"Please use Iutil for erasing databases; it is no longer supported in Iindex.\n");
	RETURN_ZERO;
      }
      if (Flag.Equals("-syn")) {
        if (++x >= argc) {
          fprintf(stderr,
                  "ERROR: No synonym file name specified after -syn.\n\n");
          RETURN_ERROR;
        }
        SynonymFileName = argv[x];
        Synonyms = GDT_TRUE;
        LastUsed = x;
      }
    }
    x++;
  }
  
  if (DBName.Equals("")) {
    DBName = IsearchDefaultDbName;
    //		fprintf(stderr, "ERROR: No database name specified!\n");
    //		RETURN_ZERO;
  }
  
  x = LastUsed + 1;
  
  INT NumFiles = argc - x;
  INT z = x;
  
  if ( (FileList.Equals("")) && (NumFiles == 0) ) {
    fprintf(stderr,"ERROR: No files specified for indexing!\n");
    RETURN_ZERO;
  }
  
  if ( (!FileList.Equals("")) && (NumFiles != 0) ) {
    fprintf(stderr,"ERROR: Unable to handle -f and file names at the same time.\n");
    RETURN_ZERO;
  }
	
  //	RECLIST reclist;
  //	RECORD record;
  STRING PathName, FileName;
	
  printf("Building document list ...\n");
	
  PIDBC pdb;
  STRING DBPathName, DBFileName;
  
  DBPathName = DBName;
  DBFileName = DBName;
  RemovePath(&DBFileName);
  RemoveFileName(&DBPathName);
	
  if (!AppendDb) {
    STRING KillFile;
    PCHR cKillFile;
		
    KillFile = DBName;
    KillFile.Cat(".dbi");
    StrUnlink(KillFile);
		
    KillFile = DBName;
    KillFile.Cat(".dfd");
    StrUnlink(KillFile);

    KillFile = DBName;
    KillFile.Cat(".mdg");
    StrUnlink(KillFile);

    KillFile = DBName;
    KillFile.Cat(".mdk");
    StrUnlink(KillFile);

    KillFile = DBName;
    KillFile.Cat(".mdt");
    StrUnlink(KillFile);

    KillFile = DBName;
    KillFile.Cat(".num");
    StrUnlink(KillFile);
		
    KillFile = DBName;
    KillFile.Cat(".syn");
    StrUnlink(KillFile);
		
    KillFile = DBName;
    KillFile.Cat(".spx");
    StrUnlink(KillFile);
		
    KillFile = DBName;
    KillFile.Cat(".scx");
    StrUnlink(KillFile);
		
#if defined(_MSDOS) || defined(_WIN32)
    /*
     * For the moment you are out of luck if you have more then
     * 99 fields in an existing database.  You'll have to remove
     * the old database by hand before building the new database.
     */
    KillFile = "del ";
    KillFile.Cat(DBName);
    KillFile.Cat(".0*");
    cKillFile = KillFile.NewCString();
    system(cKillFile);
    delete cKillFile;
    KillFile = "del ";
    KillFile.Cat(DBName);
    KillFile.Cat(".inx*");
    cKillFile = KillFile.NewCString();
    system(cKillFile);
    delete cKillFile;
#else
    KillFile = "rm -f ";
    KillFile.Cat(DBName);
    KillFile.Cat(".[0-9]*");
    cKillFile = KillFile.NewCString();
    system(cKillFile);
    delete [] cKillFile;
    KillFile = "rm -f ";
    KillFile.Cat(DBName);
    KillFile.Cat(".inx*");
    cKillFile = KillFile.NewCString();
    system(cKillFile);
    delete [] cKillFile;
#endif
  }

  pdb = new IDBC(DBPathName, DBFileName, DocTypeOptions);
  
  if (!pdb->ValidateDocType(DocumentType)) {
    fprintf(stderr,"ERROR: Unknown document type specified.\n");
    delete pdb;
    RETURN_ZERO;
  }
	
  if (DebugFlag) {
    pdb->DebugModeOn();
  }

  if (!AppendDb) {
    pdb->KillAll();
  } else {
    if (!pdb->IsDbCompatible()) {
      fprintf(stderr,"The specified database is not compatible with this version of Iindex.\n");
      fprintf(stderr,"You cannot append to a database created with a different version.\n");
      delete pdb;
      RETURN_ZERO;
    }
    FILE *fc;
    STRING CheckFile;
    CheckFile = DBName;
    CheckFile.Cat(".num");
    fc=pdb->ffopen(CheckFile,"r");
    if (fc==NULL) {
      STRING dbn;
      CheckFile=DBName;
      CheckFile.Cat(".inx.1");
      dbn=DBName;
      dbn.Cat(".inx");
      rename(dbn,CheckFile);
      CheckFile = DBName;
      CheckFile.Cat(".num");
      fc=pdb->ffopen(CheckFile,"w");
      fprintf(fc,"%d\n",1);
      // IndexAppend patch - begin
      fclose(fc);
      
      delete pdb;
      pdb = new IDBC(DBPathName, DBFileName, DocTypeOptions);
      if (DebugFlag) {
	pdb->DebugModeOn();
      }
      
    } else {
      fclose(fc);
      // IndexAppend patch - end 
    }
    // IndexAppend patch
    //    fclose(fc);
  }
	
  /* Set Global Document Type to match -t if there isn''t already one */
  if (DocumentType.GetLength() >0) {
    STRING  GlobalDoctype;
    STRLIST slDocTypeOptions;
    pdb->GetGlobalDocType(&GlobalDoctype);
    if (GlobalDoctype == "") {
      pdb->SetGlobalDocType(DocumentType);
    }
    pdb->GetDocTypeOptions(&slDocTypeOptions);
    if (slDocTypeOptions.GetTotalEntries() > 0) {
      pdb->SetDocTypeOptions();
    }
  }
	
  STRING AppTemp;
  CHR pAppTemp[80];
  pdb->ComposeDbFn(&AppTemp, ".ii~");
  AppTemp.GetCString(pAppTemp, 80);
	
  if ( FileList.Equals("") ) {
    INT y;
    STRING TheFile;
    for (z=0; z<NumFiles; z++) {
      TheFile = argv[z+x];
      if ( (!(pdb->IsSystemFile(TheFile))) &&
	   (strcmp(argv[z+x], "core") != 0) ) {
	y = chdir(argv[z+x]);
       if ((y == -1) 
	    && (TheFile.Search("*") == 0) 
	    && (TheFile.Search("?") == 0)) { 
          //  do not add the file if it contains a wildcard!!
	  PathName = argv[z+x];
	  FileName = argv[z+x];
	  RemovePath(&FileName);
	  RemoveFileName(&PathName);
	  AddFile(pdb, PathName, FileName);
	} else {
	  chdir(Cwd);
         if (Recursive 
	      || (TheFile.Search("*") > 0) 
	      || (TheFile.Search("?") > 0)) { 
            // Check to see if we still have a wildcard
	    CHR tempbuf[256];
	    if (Recursive) {
	      // This is a recursive search for files.
				
#if defined(_MSDOS) || defined (_WIN32)
	    /* Use the DOS dir command to generate the file list */
	    sprintf(tempbuf, "dir /s /b %s > %s",
		    argv[z+x], pAppTemp);
#elif defined (ULTRIX)
	    /* Ultrix doesn''t seem to like -follow. */
	    sprintf(tempbuf, "find %s -name \"*\" -print > %s",
		    argv[z+x], pAppTemp);
#else
	    sprintf(tempbuf, "find %s -name \"*\" -follow -print > %s",
		    argv[z+x], pAppTemp);
#endif
	    } else { 
	      // This must be a non recursive wildcard search 
#if defined(_MSDOS) || defined (_WIN32)
	      /* Use the DOS dir command to generate the file list */
	      sprintf(tempbuf, "dir /b %s > %s",
		      argv[z+x], pAppTemp);
#elif defined (ULTRIX)
	      /* Ultrix doesn''t seem to like -follow. */
	      sprintf(tempbuf, "find %s -name \"*\" -print > %s",
		      argv[z+x], pAppTemp);
#else
	      sprintf(tempbuf, "find %s -name \"*\" -print > %s",
		      argv[z+x], pAppTemp);
#endif
	    }
	    system(tempbuf);
						
	    
	    PFILE fp = fopen(AppTemp, "r");
	    CHR s[1024], t[1024];
	    STRING v;
	    if (!fp) {
	      fprintf(stderr,"ERROR: Can't generate file list (-r).\n");
	      delete pdb;
	      RETURN_ZERO;
	    }
	    while (fgets(s, 1023, fp) != NULL) {
	      if (s[strlen(s)-1] == '\n') {
		s[strlen(s)-1] = '\0';
	      }
	      v = s;
	      RemovePath(&v);
	      v.GetCString(t, 1024);
	      TheFile = t;
	      if ( (!(pdb->IsSystemFile(TheFile))) &&
		   (strcmp(t, pAppTemp) != 0) &&
		   (strcmp(t, "core") != 0) &&
		   (strlen(t) > 0) ) {
		y = chdir(s);
		if (y == -1) {
		  PathName = s;
		  FileName = s;
		  RemovePath(&FileName);
		  RemoveFileName(&PathName);
		  AddFile(pdb, PathName, FileName);
		} else {
		  chdir(Cwd);
		}
	      }
	    }
	    fclose(fp);
	    StrUnlink(AppTemp);
	  }
	}
      }
    }
  }
	
  if ( !FileList.Equals("") ) {
    CHR s[1024];
    PFILE fp=(PFILE)NULL;
    if (FileList.Equals("-")) {
      while (fgets(s, 1023, stdin) != NULL) {
	if (s[strlen(s)-1] == '\n') {
	  s[strlen(s)-1] = '\0';
	}
	PathName = s;
	FileName = s;
	RemovePath(&FileName);
	RemoveFileName(&PathName);
	AddFile(pdb, PathName, FileName);
      }
      if(fp)
	fclose(fp);
    } else {
      fp = fopen(FileList, "r");
      if (!fp) {
	fprintf(stderr,"ERROR: Can't find file list (-f).\n");
	delete pdb;
	RETURN_ZERO;
      }
      while (fgets(s, 1023, fp) != NULL) {
	if (s[strlen(s)-1] == '\n') {
	  s[strlen(s)-1] = '\0';
	}
	PathName = s;
	FileName = s;
	RemovePath(&FileName);
	RemoveFileName(&PathName);
	AddFile(pdb, PathName, FileName);
      }
      fclose(fp);
    }
  }
	
  if (AppendDb) {
    printf("Adding to database ");
  } else {
    printf("Building database ");
  }
  DBName.Print();
  printf(":\n");
  
  if (MemoryUsage > 0) {
#ifndef MEMTEST
    pdb->SetIndexingMemory(MemoryUsage*1024*1024);
#else
    pdb->SetIndexingMemory(MemoryUsage*8*8);
#endif
  }

  pdb->SetDbState(IsearchDbStateBusy);

  pdb->SetMergeStatus(Merge);
  pdb->Index();
  if (pdb->GetDbState() == IsearchDbStateInvalid) {
    fprintf(stderr,"Error: Could not find any files to index\n");
    RETURN_ZERO;
  }

  pdb->FlushFiles();

  if (GILS) {
    printf("Creating GILS metadata...\n");
    printf("Any default values in ");
    MetaFn.Print();
    printf(" will be included.\n");
    // parse defaults file
    REGISTRY* metadef = parseMetaDefaults(MetaFn);
    STRING GilsBuffer,GilsFile;

    // MakeDbGilsRec(pdb, PathName, FileName, &GilsBuffer);
    time_t today;
    struct tm    *t;
    CHR *date=(CHR*)NULL;
    STRING DbName;

    pdb->GetDbFileStem(&DbName);
    
    today = time((time_t *)NULL);
    t = localtime(&today);
    if ((date = (CHR *)malloc(9))) {
      strftime(date,9,"%Y%m%d",t);
    }

    GilsFile = DBName;
    GilsFile.Cat(".gils");
    FILE* fp = fopen(GilsFile, "wb");

    if (fp) {
      fprintf(fp, "<?XML VERSION=\"1.0\" ENCODING=\"UTF-8\" ?>\n");
      fprintf(fp, "<!DOCTYPE Locator SYSTEM \"xml.dtd\" >\n");

      fprintf(fp,"<Locator>\n");

      fprintf(fp,"  <Title>");
      fprintf(fp,DbName);
      fprintf(fp,"</Title>\n");
      fprintf(fp,"  <Methodology>");
      fprintf(fp,"Automatically generated by CNIDR Iindex");
      fprintf(fp,"</Methodology>\n");
      
      fprintf(fp,"  <Control-Identifier>");
      fprintf(fp,DbName);
  //  fprintf(fp,PathName);
  //  fprintf(fp,FileName);
      fprintf(fp,"</Control-Identifier>\n");
      fprintf(fp,"  <Language-of-Record>");
      fprintf(fp,"ENG");
      fprintf(fp,"</Language-of-Record>\n");
      
      fprintf(fp,"  <Date-of-Last-Modification>");
      fprintf(fp,date);
      fprintf(fp,"</Date-of-Last-Modification>\n");

      STRLIST position;
      metadef->PrintSgml(fp, position);
    }

    //    GilsBuffer.WriteFile(GilsFile);

    pdb->WriteCentroid(fp);
    fprintf(fp, "</Locator>\n");
    fclose(fp);
     
  }

  // Do we need to create a thesaurus?
  if (Synonyms) {
    THESAURUS *MyThesaurus;
    MyThesaurus = new THESAURUS(SynonymFileName,DBPathName, DBFileName);
    delete MyThesaurus;
  }

  pdb->SetDbState(IsearchDbStateReady);

  delete pdb;
  printf("Database files saved to disk.\n");
  
  RETURN_ZERO;
}
