#ifndef PREFETCH_H
#define PREFETCH_H

#include <cstdint>

class System;

class Prefetch {
public:
   virtual int prefetchMiss(uint64_t address, unsigned int tid,
                              System* sys)=0;
   virtual int prefetchHit(uint64_t address, unsigned int tid,
                              System* sys)=0;
};

//"Prefetcher" that does nothing
class NullPrefetch : public Prefetch {
public:
   int prefetchMiss(uint64_t address, unsigned int tid,
                              System* sys);
   int prefetchHit(uint64_t address, unsigned int tid,
                              System* sys);
};

#endif
