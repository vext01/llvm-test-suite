#include <assert.h>
#include <err.h>

#include <iostream>

#include "StackMapParser.h"

using namespace std;

template <typename T> T StackMapParser::read(char **SMData) {
  T Ret = *reinterpret_cast<T *>(*SMData);
  *SMData += sizeof(T);
  return Ret;
}

void StackMapParser::alignTo(char **SMData, size_t To) {
  *SMData += reinterpret_cast<uintptr_t>(*SMData) % To;
}

StackMapParser::StackMapParser(char *SMData) {
  // Header.
  uint8_t Version = read<uint8_t>(&SMData);
  if (Version != SMVersion)
    errx(EXIT_FAILURE, "expected stackmap version '%d', got version '%d'",
         SMVersion, Version);
  read<uint8_t>(&SMData);  // Skip reserved.
  read<uint16_t>(&SMData); // Skip reserved.

  // Multiplicities of tables.
  uint32_t NumFunctions = read<uint32_t>(&SMData);
  uint32_t NumConstants = read<uint32_t>(&SMData);
  uint32_t NumRecords = read<uint32_t>(&SMData);

  // Functions.
  for (uint32_t I = 0; I < NumFunctions; I++)
    Funcs.push_back({read<uint64_t>(&SMData), read<uint64_t>(&SMData),
                     read<uint64_t>(&SMData)});

  // Constants.
  for (uint64_t I = 0; I < NumConstants; I++)
    Consts.push_back(read<uint64_t>(&SMData));

  // Records.
  for (uint64_t I = 0; I < NumRecords; I++) {
    uint64_t ID = read<uint64_t>(&SMData);
    uint32_t InstrOff = read<uint32_t>(&SMData);
    read<uint16_t>(&SMData); // skip reserved.
    uint16_t NumLocs = read<uint16_t>(&SMData);

    vector<SMLoc> Locs;
    for (uint16_t J = 0; J < NumLocs; J++) {
      uint8_t Where = read<uint8_t>(&SMData);
      read<uint8_t>(&SMData); // Skip reserved.
      uint16_t Size = read<uint16_t>(&SMData);
      uint16_t RegNum = read<uint16_t>(&SMData);
      read<uint16_t>(&SMData); // Skip reserved.
      int32_t OffsetOrSmallConst = read<int32_t>(&SMData);
      Locs.push_back({StorageClass(Where), Size, RegNum, OffsetOrSmallConst});
    }

    alignTo(&SMData, 8);     // Pad if necessary.
    read<uint16_t>(&SMData); // Skip reserved.

    // Liveouts.
    uint16_t NumLiveOuts = read<uint16_t>(&SMData);
    vector<SMLiveOut> LiveOuts;
    for (uint64_t I = 0; I < NumLiveOuts; I++) {
      uint16_t RegNum = read<uint16_t>(&SMData);
      read<uint8_t>(&SMData); // Skip reserved.
      uint8_t Size = read<uint8_t>(&SMData);
      LiveOuts.push_back({RegNum, Size});
    }

    alignTo(&SMData, 8); // Pad if necessary.

    Recs.insert({ID, {InstrOff, Locs, LiveOuts}});
  }
}

SMRec *StackMapParser::getRecord(uint64_t ID) {
  map<uint64_t, SMRec>::iterator It = Recs.find(ID);
  if (It == Recs.end())
    return nullptr;
  return &It->second;
}

string getStorageClassName(StorageClass C) {
  switch (C) {
  case Reg:
    return "Register";
  case Direct:
    return "Direct";
  case Indirect:
    return "Indirect";
  case Const:
    return "Constant";
  case ConstIndex:
    return "ConstIndex";
  default:
    errx(EXIT_FAILURE, "unreachable");
  }
}
