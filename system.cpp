#include <map>
#include <iostream>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include "misc.h"
#include "cache.h"
#include "system.h"

using namespace std;

System::System(std::vector<unsigned int> tid_to_domain,
            unsigned int line_size, unsigned int num_lines, unsigned int assoc,
            Prefetch* prefetcher, bool count_compulsory /*=false*/,
            bool do_addr_trans /*=false*/)
{ 
   srand(time(NULL)); // generate random numbers
   assert(num_lines % assoc == 0);

   stats.hits = stats.local_reads = stats.remote_reads =
      stats.othercache_reads = stats.local_writes =
      stats.remote_writes = stats.compulsory = 0;

   LINE_MASK = ((uint64_t) line_size)-1;
   SET_SHIFT = log2(line_size);
   SET_MASK = ((num_lines / assoc) - 1) << SET_SHIFT;
   TAG_MASK = ~(SET_MASK | LINE_MASK);

   nextPage = 0;

   countCompulsory = count_compulsory;
   doAddrTrans = do_addr_trans;
   this->tid_to_domain = tid_to_domain;
   this->prefetcher = prefetcher;
}

void System::checkCompulsory(uint64_t line)
{
   std::set<uint64_t>::iterator it;

   it = seenLines.find(line);
   if(it == seenLines.end()) {
      stats.compulsory++;
      seenLines.insert(line);
   }
}

uint64_t System::virtToPhys(uint64_t address)
{
   std::map<uint64_t, uint64_t>::iterator it;
   uint64_t virt_page = address & PAGE_MASK;
   uint64_t phys_page;
   uint64_t phys_addr = address & (~PAGE_MASK);

   it = virtToPhysMap.find(virt_page);
   if(it != virtToPhysMap.end()) {
      phys_page = it->second;
      phys_addr |= phys_page;
   }
   else {
      phys_page = nextPage << PAGE_SHIFT;
      phys_addr |= phys_page;
      virtToPhysMap.insert(std::make_pair(virt_page, phys_page));
      //nextPage += rand() % 200 + 5 ;
      ++nextPage;
   }

   return phys_addr;
}

unsigned int MultiCacheSystem::checkRemoteStates(uint64_t set,
               uint64_t tag, cacheState& state, unsigned int local)
{
   cacheState curState = INV;
   state = INV;
   unsigned int remote = 0;

   for(unsigned int i=0; i<caches.size(); i++) {
      if(i == local) {
         continue;
      }
      curState = caches[i]->findTag(set, tag);
      switch (curState)
      {
         case OWN:
            state = OWN;
            return i;
            break;
         case SHA:
            // A cache line in a shared state may be
            // in the owned state in a different cache
            // so don't return i immdiately
            state = SHA;
            remote = i;
            break;
         case EXC:
            state = EXC;
            return i;
            break;
         case MOD:
            state = MOD;
            return i;
            break;
         default:
            break;
      }
   }

   return remote;
}

void MultiCacheSystem::setRemoteStates(uint64_t set,
               uint64_t tag, cacheState state, unsigned int local)
{
   for(unsigned int i=0; i<caches.size(); i++) {
      if(i != local) {
         caches[i]->changeState(set, tag, state);
      }
   }
}

// Maintains the statistics for memory write-backs
void MultiCacheSystem::evictTraffic(uint64_t set,
               uint64_t tag, unsigned int local)
{
   uint64_t page = ((set << SET_SHIFT) | tag) & PAGE_MASK;
   unsigned int domain = pageToDomain[page];

   if(domain == local) {
      stats.local_writes++;
   }
   else {
      stats.remote_writes++;
   }
}

bool MultiCacheSystem::isLocal(uint64_t address, unsigned int local)
{
   uint64_t page = address & PAGE_MASK;
   unsigned int domain = pageToDomain[page];

   return (domain == local);
}


cacheState MultiCacheSystem::processMOESI(uint64_t set,
                  uint64_t tag, cacheState remote_state, char rw,
                  bool is_prefetch, bool local_traffic, unsigned int local,
                  unsigned int remote)
{
   cacheState new_state = INV;

   if(remote_state == INV && rw == 'R') {
      new_state = EXC;
      if(local_traffic && !is_prefetch) {
         stats.local_reads++;
      }
      else if(!is_prefetch) {
         stats.remote_reads++;
      }
   }
   else if(remote_state == INV && rw == 'W') {
      new_state = MOD;
      if(local_traffic && !is_prefetch) {
         stats.local_reads++;
      }
      else if(!is_prefetch) {
         stats.remote_reads++;
      }
   }
   else if(remote_state == SHA && rw == 'R') {
      new_state = SHA;
      if(local_traffic && !is_prefetch) {
         stats.local_reads++;
      }
      else if(!is_prefetch) {
         stats.remote_reads++;
      }
   }
   else if(remote_state == SHA && rw == 'W') {
      new_state = MOD;
      setRemoteStates(set, tag, INV, local);
      if(!is_prefetch) {
         stats.othercache_reads++;
      }
   }
   else if((remote_state == MOD || remote_state == OWN) && rw == 'R') {
      new_state = SHA;
      caches[remote]->changeState(set, tag, OWN);
      if(!is_prefetch) {
         stats.othercache_reads++;
      }
   }
   else if((remote_state == MOD || remote_state == OWN || remote_state == EXC)
            && rw == 'W') {
      new_state = MOD;
      setRemoteStates(set, tag, INV, local);
      if(!is_prefetch) {
         stats.othercache_reads++;
      }
   }
   else if(remote_state == EXC && rw == 'R') {
      new_state = SHA;
      caches[remote]->changeState(set, tag, SHA);
      if(!is_prefetch) {
         stats.othercache_reads++;
      }
   }

#ifdef DEBUG
   assert(new_state != INV);
#endif

   return new_state;
}

void MultiCacheSystem::memAccess(uint64_t address, char rw,
      unsigned int tid, bool is_prefetch /*=false*/)
{
   uint64_t set, tag;
   unsigned int local;
   bool hit;
   cacheState state;

   if(doAddrTrans) {
      address = virtToPhys(address);
   }

   local = tid_to_domain[tid];
   updatePageToDomain(address, local);

   set = (address & SET_MASK) >> SET_SHIFT;
   tag = address & TAG_MASK;
   state = caches[local]->findTag(set, tag); // 
   hit = (state != INV);

   if(countCompulsory && !is_prefetch) {
      checkCompulsory(address & LINE_MASK);
   }

   // Handle hits
   if(rw == 'W' && hit) {
      caches[local]->changeState(set, tag, MOD);
      setRemoteStates(set, tag, INV, local);
   }

   if(hit) {
      caches[local]->updateLRU(set, tag);
      if(!is_prefetch) {
         stats.hits++;
         stats.prefetched += prefetcher->prefetchHit(address, tid, this);
      }
   }
   else {
      // Now handle miss cases
      cacheState remote_state;
      cacheState new_state = INV;
      uint64_t evicted_tag;
      bool writeback, local_traffic;

      unsigned int remote = checkRemoteStates(set, tag, remote_state, local);

      writeback = caches[local]->checkWriteback(set, evicted_tag);
      if(writeback) {
         evictTraffic(set, evicted_tag, local);
      }

      local_traffic = isLocal(address, local);

      new_state = processMOESI(set, tag, remote_state, rw, is_prefetch,
                                 local_traffic, local, remote);
      caches[local]->insertLine(set, tag, new_state);
      if(!is_prefetch) {
         stats.prefetched += prefetcher->prefetchMiss(address, tid, this);
      }
   }
}

// Keeps track of which NUMA domain each memory page is in,
// using a first-touch policy
void MultiCacheSystem::updatePageToDomain(uint64_t address,
                                          unsigned int curDomain)
{
   std::map<uint64_t, unsigned int>::iterator it;
   uint64_t page = address & PAGE_MASK;

   it = pageToDomain.find(page);
   if(it == pageToDomain.end()) {
      pageToDomain[page] = curDomain;
   }
}

MultiCacheSystem::MultiCacheSystem(std::vector<unsigned int> tid_to_domain,
            unsigned int line_size, unsigned int num_lines, unsigned int assoc,
            Prefetch* prefetcher, bool count_compulsory /*=false*/,
            bool do_addr_trans /*=false*/, unsigned int num_domains) :
            System(tid_to_domain, line_size, num_lines, assoc, prefetcher,
                     count_compulsory, do_addr_trans)
{
   for(unsigned int i=0; i<num_domains; i++) {
      SetCache* temp = new SetCache(num_lines, assoc);
      caches.push_back(temp);
   }

   return;
}

MultiCacheSystem::~MultiCacheSystem()
{
   for(unsigned int i=0; i<caches.size(); i++) {
      delete caches[i];
   }
}

void SingleCacheSystem::memAccess(uint64_t address, char rw, unsigned
   int tid, bool is_prefetch /*=false*/)
{
   uint64_t set, tag;
   bool hit;
   cacheState state;

   if(doAddrTrans) {
      address = virtToPhys(address);
   }

   set = (address & SET_MASK) >> SET_SHIFT;
   tag = address & TAG_MASK;
   state = cache->findTag(set, tag);
   hit = (state != INV);

   if(countCompulsory && !is_prefetch) {
      checkCompulsory(address & LINE_MASK);
   }

   // Handle hits
   if(rw == 'W' && hit) {
      cache->changeState(set, tag, MOD);
   }

   if(hit) {
      cache->updateLRU(set, tag);
      if(!is_prefetch) {
         stats.hits++;
         stats.prefetched += prefetcher->prefetchHit(address, tid, this);
      }
      return;
   }

   cacheState new_state = INV;
   uint64_t evicted_tag;
   bool writeback = cache->checkWriteback(set, evicted_tag);

   if(writeback) {
      stats.local_writes++;
   }

   if(rw == 'R') {
      new_state = EXC;
   }
   else {
      new_state = MOD;
   }

   if(!is_prefetch) {
      stats.local_reads++;
   }

   cache->insertLine(set, tag, new_state);
   if(!is_prefetch) {
      stats.prefetched += prefetcher->prefetchMiss(address, tid, this);
   }
}

SingleCacheSystem::SingleCacheSystem(std::vector<unsigned int> tid_to_domain,
            unsigned int line_size, unsigned int num_lines, unsigned int assoc,
            Prefetch* prefetcher, bool count_compulsory /*=false*/,
            bool do_addr_trans /*=false*/) :
            System(tid_to_domain, line_size, num_lines, assoc,
               prefetcher, count_compulsory, do_addr_trans)
{
   cache = new SetCache(num_lines, assoc);
}

SingleCacheSystem::~SingleCacheSystem()
{
   delete cache;
}
