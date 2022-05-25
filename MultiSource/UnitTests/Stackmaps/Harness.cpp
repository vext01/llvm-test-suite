#include <inttypes.h>

#include <assert.h>
#include <elf.h>
#include <err.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "StackMapParser.h"

using namespace std;

StackMapParser *SMPRef = nullptr;
uint64_t NextID = 0;
int ExitStatus = EXIT_SUCCESS;

// Defined in the `.ll` file for the platform under test.
extern "C" void run_tests(uintptr_t *);

// Defined the the `.ll` file for the platform under test. It must be
// pointer-sized.
extern uintptr_t test_input;

void dumpMem(const char *Ptr, size_t Size) {
  // FIXME: should it be unsigned char everywhere?
  for (size_t I = 0; I < Size; I++) {
    printf("%02x", (unsigned char)Ptr[I]);
    if (I != Size - 1)
      printf(" ");
  }
  printf("\n");
}

char *getStackMapSectionAddr(char *Path, void **OutMap, off_t *OutMapSize) {
  // First map the binary into our adress space.
  struct stat St;
  if (stat(Path, &St) == -1)
    err(EXIT_FAILURE, "stat");

  int Fd = open(Path, O_RDONLY);
  if (Fd == -1)
    err(EXIT_FAILURE, "open");

  void *Map = mmap(nullptr, St.st_size, PROT_READ, MAP_SHARED, Fd, 0);
  if (Map == MAP_FAILED)
    err(EXIT_FAILURE, "mmap");

  close(Fd);

  // Report mmap result back to caller, so it can be unmapped later.
  *OutMap = Map;
  *OutMapSize = St.st_size;

  // Now find the stackmap section inside what we just mapped in.
  Elf64_Ehdr *Ehdr = static_cast<Elf64_Ehdr *>(Map);
  assert(Ehdr[0].e_ident = EI_MAG0);
  assert(Ehdr[1].e_ident = EI_MAG1);
  assert(Ehdr[2].e_ident = EI_MAG2);
  assert(Ehdr[3].e_ident = EI_MAG3);

  Elf64_Shdr *Shdrs = static_cast<Elf64_Shdr *>(
      static_cast<void *>(static_cast<char *>(Map) + Ehdr->e_shoff));

  // Find the string table for the section names.
  uint16_t StrTabShIdx = Ehdr->e_shstrndx;
  assert(StrTabShIdx != SHN_UNDEF);
  if (StrTabShIdx >= SHN_LORESERVE)
    return nullptr; // FIXME not yet implemented. See `elf(5)` manual.

  Elf64_Shdr *StrTabSh = &Shdrs[StrTabShIdx];
  assert(StrTabSh->sh_type == ELF::SHT_STRTAB);
  Elf64_Off StrTabOff = StrTabSh->sh_offset;

  void *Data = nullptr;
  for (uint16_t SI = 0; SI < Ehdr->e_shentsize; SI++) {
    Elf64_Shdr *Shdr = &Shdrs[SI];
    char *name = static_cast<char *>(Map) + StrTabOff + Shdr->sh_name;
    if (strcmp(name, ".llvm_stackmaps") == 0)
      return static_cast<char *>(Map) + Shdr->sh_offset;
  }
  return nullptr;
}

// Get a pointer to the start of the storage referenced by `Loc`.
//
// This code asssumes that a register is the size of a pointer.
char *getMemForLoc(SMLoc *Loc, uintptr_t RegVals[]) {
  switch (Loc->Where) {
  case Reg:
    // Get the saved register values off of the stack.
    return reinterpret_cast<char *>(&RegVals[Loc->RegNum]);
  case Direct:
    errx(EXIT_FAILURE, "Direct locations unimplemented");
  case Indirect:
    errx(EXIT_FAILURE, "Indirect locations unimplemented");
  case Const:
    // Get the constant directly from the stackmap record.
    return reinterpret_cast<char *>(&Loc->OffsetOrSmallConst);
  case ConstIndex:
    errx(EXIT_FAILURE, "ConstIndex locations unimplemented");
  default:
    errx(EXIT_FAILURE, "unexpected storage class: %u\n", Loc->Where);
  }
}

extern "C" void do_sm_inspect(uintptr_t RegVals[]) { //, uintptr_t RetAddr) {
  SMRec *Rec = SMPRef->getRecord(NextID);
  if (Rec == nullptr)
    errx(EXIT_FAILURE, "Failed to find stackmap record #%u\n", NextID);

  cout << "StackMap #" << NextID << "\n";
  size_t LocNum = 0;
  for (auto Loc : Rec->Locs) {
    char *GotPtr = getMemForLoc(&Loc, RegVals);

    // FIXME: This is arguably a bug in LLVM stackmaps. Small constants are
    // reported as being 8 bytes long, but the stackmap format only allows for
    // 4 bytes.
    size_t GotSize = Loc.Size;
    if (Loc.Where == Const)
      GotSize = 4;

    cout << "  Location #" << LocNum << "\n";
    cout << "    Storage: " << getStorageClassName(Loc.Where) << "\n";
    cout << "    Size:    " << GotSize << "\n";
    cout << "    Bytes:   ";
    dumpMem(GotPtr, GotSize);
    LocNum++;
  }
  NextID++;
  cout << "\n";
}

__attribute__((naked)) extern "C" void sm_inspect() {
  asm volatile(".intel_syntax noprefix\n"
               // Save register state onto the stack in reverse DWARF register
               // number order.
               "push r15\n"
               "push r14\n"
               "push r13\n"
               "push r12\n"
               "push r11\n"
               "push r10\n"
               "push r9\n"
               "push r8\n"
               "push rsp\n"
               "push rbp\n"
               "push rdi\n"
               "push rsi\n"
               "push rbx\n"
               "push rcx\n"
               "push rdx\n"
               "push rax\n"
               // Call do_sm_inspect().
               //
               // Arg #0: pointer to saved registers.
               "mov rdi, rsp\n"
               // Arg #1: return address.
               //"mov rsi, [rsp - 136]\n"
               "call do_sm_inspect\n"

               // Restore registers we saved earlier.
               "pop rax\n"
               "pop rdx\n"
               "pop rcx\n"
               "pop rbx\n"
               "pop rsi\n"
               "pop rdi\n"
               "pop rbp\n"
               "pop rsp\n"
               "pop r8\n"
               "pop r9\n"
               "pop r10\n"
               "pop r11\n"
               "pop r12\n"
               "pop r13\n"
               "pop r14\n"
               "pop r15\n"
               "ret");
}

#define NOOPT_VAL(X) asm volatile("" : "+r,m"(X) : : "memory");

int main(int argc, char *argv[]) {
  void *Map;
  off_t MapSize;
  char *SMData = getStackMapSectionAddr(argv[0], &Map, &MapSize);
  if (SMData == nullptr)
    errx(EXIT_FAILURE, "couldn't find stackmaps");

  StackMapParser SMP(SMData);
  munmap(Map, MapSize);

  SMPRef = &SMP;

  NOOPT_VAL(test_input);
  run_tests(&test_input);

  return ExitStatus;
}
