#ifndef CACHE_H
#define CACHE_H

#include <vector>
#include <set>
#include <list>
#include <set>
#include <unordered_map>
#include "misc.h"

class Cache{
public:
   virtual ~Cache(){}
   virtual cacheState findTag(uint64_t set,
                                 uint64_t tag) const = 0;
   virtual void changeState(uint64_t set,
                              uint64_t tag, cacheState state) = 0;
   virtual void updateLRU(uint64_t set, uint64_t tag) = 0;
   virtual bool checkWriteback(uint64_t set,
      uint64_t& tag) const = 0;
   virtual void insertLine(uint64_t set,
                              uint64_t tag, cacheState state) = 0;
};

class SetCache : public Cache
{
   std::vector<std::set<cacheLine> > sets;
   std::vector<std::list<uint64_t> > lruLists;
   std::vector<std::unordered_map<uint64_t,
   std::list<uint64_t>::iterator> > lruMaps;
   // set a flag to control which policy (LRU or random) to use
   // set boolean false when LRUPolicy is used
   bool LRUPolicy = false;
public:
   SetCache(unsigned int num_lines, unsigned int assoc);
   cacheState findTag(uint64_t set, uint64_t tag) const;
   void changeState(uint64_t set, uint64_t tag,
                     cacheState state);
   void updateLRU(uint64_t set, uint64_t tag);
   bool checkWriteback(uint64_t set, uint64_t& tag) const;
   void insertLine(uint64_t set, uint64_t tag,
                     cacheState state);
};

#endif

