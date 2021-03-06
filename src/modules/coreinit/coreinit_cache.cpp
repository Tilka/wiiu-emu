#include "coreinit.h"
#include "coreinit_cache.h"
#include "util.h"

void
DCInvalidateRange(void *addr, uint32_t size)
{
   // TODO: DCInvalidateRange
}

void
DCFlushRange(void *addr, uint32_t size)
{
   // TODO: DCFlushRange
}

void
DCStoreRange(void *addr, uint32_t size)
{
   // TODO: DCStoreRange
}

void
DCFlushRangeNoSync(void *addr, uint32_t size)
{
   // TODO: DCFlushRangeNoSync
}

void
DCStoreRangeNoSync(void *addr, uint32_t size)
{
   // TODO: DCStoreRangeNoSync
}

void
DCZeroRange(void *addr, uint32_t size)
{
   // TODO: DCZeroRange check align direction is correct!
   size = alignDown(size, 32);
   addr = alignUp(addr, 32);
   memset(addr, 0, size);
}

void
DCTouchRange(void *addr, uint32_t size)
{
   // TODO: DCTouchRange
}

void
CoreInit::registerCacheFunctions()
{
   RegisterKernelFunction(DCInvalidateRange);
   RegisterKernelFunction(DCFlushRange);
   RegisterKernelFunction(DCStoreRange);
   RegisterKernelFunction(DCFlushRangeNoSync);
   RegisterKernelFunction(DCStoreRangeNoSync);
   RegisterKernelFunction(DCZeroRange);
   RegisterKernelFunction(DCTouchRange);
}