#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <string>
#include "system.h"

using namespace std;

int main()
{
   // tid_map is used to inform the simulator how
   // thread ids map to NUMA/cache domains. Using
   // the tid as an index gives the NUMA domain.
   unsigned int arr_map[] = {0};
   vector<unsigned int> tid_map(arr_map, arr_map +
         sizeof(arr_map) / sizeof(unsigned int));
   NullPrefetch prefetch;
   // The constructor parameters are:
   // the tid_map, the cache line size in bytes,
   // number of cache lines, the associativity,
   // the prefetcher object,
   // whether to count compulsory misses,
   // whether to do virtual to physical translation,
   // and number of caches/domains
   // WARNING: counting compulsory misses doubles execution time
   MultiCacheSystem sys(tid_map, 64, 1024, 64, &prefetch, true, false, 1);
   char rw;
   uint64_t address;
   unsigned long long lines = 0;
   ifstream infile;
   // This code works with the output from the
   // ManualExamples/pinatrace pin tool
   infile.open("pinatrace.out", ifstream::in);
   assert(infile.is_open());

   string line;
   while(getline(infile,line))
   {
      stringstream ss(line);
      ss >> rw;
      assert(rw == 'R' || rw == 'W');
      ss >> hex >> address;
      if(address != 0) {
         // By default the pinatrace tool doesn't record the tid,
         // so we make up a tid to stress the MultiCache functionality
         sys.memAccess(address, rw, 0);
      }

      ++lines;
   }
   


   cout << "Accesses: " << lines << endl;
   cout << "Hits: " << sys.stats.hits << endl;
   cout << "Misses: " << lines - sys.stats.hits << endl;
   cout << "Local reads: " << sys.stats.local_reads << endl;
   cout << "Local writes: " << sys.stats.local_writes << endl;
   cout << "Remote reads: " << sys.stats.remote_reads << endl;
   cout << "Remote writes: " << sys.stats.remote_writes << endl;
   cout << "Other-cache reads: " << sys.stats.othercache_reads << endl;
   //cout << "Compulsory Misses: " << sys.stats.compulsory << endl;

   infile.close();

   // calculate the hit ratio 
   // and write in ratio.txt
    float hit_ratio = sys.stats.hits / lines;
    cout << "Hit Ratio: " << hit_ratio << endl;

   return 0;
}
