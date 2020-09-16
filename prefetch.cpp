#include "prefetch.h"
#include "system.h"

int NullPrefetch::prefetchMiss(uint64_t address,
	unsigned int tid,
	System* sys)
{
   return 0;
}
int NullPrefetch::prefetchHit(uint64_t address,
	unsigned int tid ,
	System* sys)
{
   return 0;
}

