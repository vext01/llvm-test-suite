#ifndef TEST_SUITE_STACKMAP_PARSER_H
#define TEST_SUITE_STACKMAP_PARSER_H

#include <inttypes.h>
#include <unistd.h>

#include <map>
#include <vector>

using namespace std;

const uint8_t SMVersion = 3;

enum StorageClass {
  Reg = 1,
  Direct = 2,
  Indirect = 3,
  Const = 4,
  ConstIndex = 5
};

struct SMFunc {
  uint64_t Addr;
  uint64_t Size;
  uint64_t RecCt;
};

struct SMLoc {
  StorageClass Where;
  uint16_t Size;
  uint16_t RegNum;
  int32_t OffsetOrSmallConst;
};

struct SMLiveOut {
  uint16_t RegNo;
  uint8_t Size;
};

struct SMRec {
  uint32_t InstrOff;
  vector<SMLoc> Locs;
  vector<SMLiveOut> LiveOuts;
};

// A simple (copying) Stackmap parser.
// https://llvm.org/docs/StackMaps.html#stack-map-format
class StackMapParser {
private:
  vector<SMFunc> Funcs;
  vector<uint64_t> Consts;
  map<uint64_t, SMRec> Recs;

  template <typename T> T read(char **SMData);
  void alignTo(char **SMData, size_t To);

public:
  StackMapParser(char *SMData);
  SMRec *getRecord(uint64_t ID);
};

string getStorageClassName(StorageClass C);

#endif // TEST_SUITE_STACKMAP_PARSER_H
