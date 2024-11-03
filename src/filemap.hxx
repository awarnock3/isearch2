#ifndef FILEMAP_HXX
#define FILEMAP_HXX

struct _table{
  GPTYPE GpStart;
  GPTYPE LocalStart;
  GPTYPE LocalEnd;
  STRING Path;
};
class FILEMAP{

 public:

  FILEMAP(const PIDBOBJ p);
  GPTYPE GetNameByGlobal(GPTYPE gp, PSTRING s,INT *size, INT *LS);
GPTYPE GetKeyByGlobal(GPTYPE gp);
  ~FILEMAP();

 private:
  struct _table *Items;
  INT MdtCount;
  PMDT mdt;
  PIDBOBJ Parent;

};
#endif
