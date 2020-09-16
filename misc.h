#ifndef MISC_H
#define MISC_H

#include <cstdint>

#define PAGE_SIZE_4KB

#ifdef PAGE_SIZE_4KB
const uint64_t PAGE_MASK = 0xFFFFFFFFFFFFF000;
const uint32_t PAGE_SHIFT = 12;
#elif defined PAGE_SIZE_2MB
const uint64_t PAGE_MASK = 0xFFFFFFFFFFE00000;
const uint32_t PAGE_SHIFT = 21;
#else
#error "Bad PAGE_SIZE"
#endif

enum cacheState {MOD,OWN,EXC,SHA,INV};

struct cacheLine{
   uint64_t tag;
   cacheState state;
   cacheLine(){tag = 0; state = INV;}
   bool operator<(const cacheLine& rhs) const
   { return tag < rhs.tag; }
   bool operator==(const cacheLine& rhs) const
   { return tag == rhs.tag; }
};

#endif
