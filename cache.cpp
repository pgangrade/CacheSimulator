
#include <cassert>
#include <cmath>
#include <iostream>
#include <utility>
#include "misc.h"
#include "cache.h"

using namespace std;

SetCache::SetCache(unsigned int num_lines, unsigned int assoc)
{
   assert(num_lines % assoc == 0);
   // The set bits of the address will be used as an index
   // into sets. Each set is a set containing "assoc" items
   sets.resize(num_lines / assoc);
   lruLists.resize(num_lines / assoc);
   lruMaps.resize(num_lines / assoc);
   for(unsigned int i=0; i < sets.size(); i++) {
      for(unsigned int j=0; j < assoc; ++j) {
         cacheLine temp;
         temp.tag = j;
         temp.state = INV;  
         sets[i].insert(temp);
         lruLists[i].push_front(j);
         lruMaps[i].insert(make_pair(j, lruLists[i].begin()));
      }
   }
}

/* FIXME invalid vs not found */
// Given the set and tag, return the cache lines state
cacheState SetCache::findTag(uint64_t set,
                              uint64_t tag) const
{ 
  // create a temporary cache line 
	cacheLine tempCache;
  tempCache.tag = tag;
  std::set<cacheLine>::const_iterator iterator = sets[set].find(tempCache);

//iterate over the set till the set is empty to find if the cache line is in the set
  if(iterator != sets[set].end()) {
    cacheLine *target;
    target = (cacheLine*)&*iterator;
    cout << "Hello" << target << endl;
    //retrun cache line state 
    // default state is EXC
    return target->state; 

  }
// retrun invalid if the cache line is not found in the set
 return INV;
}

/* FIXME invalid vs not found */
// Changes the cache line specificed by "set" and "tag" to "state"
void SetCache::changeState(uint64_t set, uint64_t tag,
                              cacheState state)
{
   cacheLine temp;
   temp.tag = tag;
   std::set<cacheLine>::const_iterator it = sets[set].find(temp);

   if(it != sets[set].end()) {
      cacheLine *target;
      target = (cacheLine*)&*it;
      target->state = state;
   }
}

// A complete LRU is mantained for each set, using a separate
// list and map. The front of the list is considered most recently used.
void SetCache::updateLRU(uint64_t set, uint64_t tag)
{
// create a list of tags
   std::list<uint64_t>* tagList = &lruLists[set];  // lruLists[set] contains tags in the LRU

   // check if the tag is in the list 
   // if the tag is not in the list remove the tag and push the tag in the front the list

   if (findTag(set, tag) != INV) {
      tagList->remove(tag);  // remove from current position and move to front 
      tagList->push_front(tag);
      //cout << tagList ;
   }
   // otherwise remove the LRU tag
   else {
      tagList->pop_back();
      // add this tag to the front of the list
      tagList->push_front(tag);
      //cout << "update LRU";
   }

}

// Called if a new cache line is to be inserted. Checks if
// the least recently used line needs to be written back to
// main memory.
bool SetCache::checkWriteback(uint64_t set,
                                 uint64_t& tag) const
{
   cacheLine evict, temp;
   tag = lruLists[set].back();
   temp.tag = tag;
   evict = *sets[set].find(temp);

   return (evict.state == MOD || evict.state == OWN);
}

// FIXME: invalid vs not found
// Insert a new cache line by popping the least //recently used line
// and pushing the new line to the front not back (most recently //used)
void SetCache::insertLine(uint64_t set, uint64_t tag,
                           cacheState state)
{
  std::set<cacheLine>::const_iterator iterator;

  if(LRUPolicy)
  {
    uint64_t lruTag = lruLists[set].back();

    cacheLine oldLine;
    oldLine.tag = lruTag;
    iterator = sets[set].find(oldLine);
    // update the LRU
    updateLRU(set, tag);
  }
   // use Random replacement policy 
  else
  {
int assoc = lruLists[set].size();
int randNum = rand() % assoc; // rand() to generate random numbers
      // get the beginning of the cache set
      iterator = sets[set].begin();
      // iterate to get the random line we chose earlier
      for (int i = 0; i < randNum; i++) 
      iterator++;
   }


   // if the cacheLine is in the set (it always should be, this just checks)
   if (iterator != sets[set].end()) {
      // delete this cache line
      sets[set].erase(iterator);
   }

   // create the cache line with the given information
   cacheLine newLine;
   newLine.tag = tag;
   newLine.state = state;
   // insert the new line into the cache
   sets[set].insert(newLine);
  }



