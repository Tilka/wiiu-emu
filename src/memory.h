#pragma once
#include <cassert>
#include <vector>
#include "bitutils.h"
#include "types.h"

union PageEntry
{
   struct
   {
      uint32_t base : 20;           // First page in region
      uint32_t count : 20;          // Number of pages in region (only valid in base page)
      uint32_t allocated : 1;       // Is page allocated?
      uint32_t : 22;
   };

   uint64_t value = 0;
};

enum class MemoryType
{
   SystemData,
   Application,
   Foreground,
   MEM1
};

struct MemoryView
{
   MemoryView() :
      address(0)
   {
   }

   MemoryView(MemoryType type, uint32_t start, uint32_t end, uint32_t pageSize) :
      type(type), start(start), end(end), pageSize(pageSize), address(0)
   {
   }

   MemoryType type;
   uint32_t start;
   uint32_t end;
   uint8_t *address;
   uint32_t pageSize;
   std::vector<PageEntry> pageTable;
};

class Memory
{
public:
   ~Memory();

   bool initialise();
   bool valid(ppcaddr_t address);
   bool alloc(ppcaddr_t address, size_t size);
   ppcaddr_t alloc(MemoryType type, size_t size);
   bool free(ppcaddr_t address);

   size_t base() const
   {
      return (size_t)mBase;
   }

   // Translate WiiU virtual address to host address
   template<typename Type = uint8_t>
   Type *translate(ppcaddr_t address) const
   {
      if (!address) {
         return nullptr;
      } else {
         return reinterpret_cast<Type*>(mBase + address);
      }
   }

   template<typename Type>
   Type *translatePtr(Type *ptr) const
   {
      return reinterpret_cast<Type*>(translate(reinterpret_cast<uint32_t>(ptr)));
   }

   // Translate host address to WiiU virtual address
   ppcaddr_t untranslate(const void *ptr) const
   {
      if (!ptr) {
         return 0;
      }

      auto sptr = reinterpret_cast<size_t>(ptr);
      auto sbase = reinterpret_cast<size_t>(mBase);
      assert(sptr > sbase);
      assert(sptr <= sbase + 0xFFFFFFFF);
      return static_cast<ppcaddr_t>(sptr - sbase);
   }

   // Read Type from virtual address
   template<typename Type>
   Type read(ppcaddr_t address) const
   {
      return byte_swap(readNoSwap<Type>(address));
   }

   // Read Type from virtual address with no endian byte_swap
   template<typename Type>
   Type readNoSwap(ppcaddr_t address) const
   {
      return *reinterpret_cast<Type*>(translate(address));
   }

   // Write Type to virtual address
   template<typename Type>
   void write(ppcaddr_t address, Type value) const
   {
      writeNoSwap(address, byte_swap(value));
   }

   // Write Type to virtual address with no endian byte_swap
   template<typename Type>
   void writeNoSwap(ppcaddr_t address, Type value) const
   {
      *reinterpret_cast<Type*>(translate(address)) = value;
   }

private:
   MemoryView *getView(MemoryType type);
   MemoryView *getView(uint32_t address);
   bool tryMapViews(uint8_t *base);
   void unmapViews();

   uint8_t *mBase = nullptr;
   void *mFile = NULL;
   std::vector<MemoryView> mViews;
};

extern Memory gMemory;
