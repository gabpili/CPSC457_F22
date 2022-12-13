/// (c) 2022, Pavol Federl, pfederl@ucalgary.ca
/// Do not distribute this file.

#include "memsim.h"
#include <cassert>
#include <iostream>
#include <list>
#include <algorithm>
#include <set>
#include <unordered_map>

using namespace std;

struct Partition { 
  long tag;
  int64_t size;
  int64_t addr; 
};

typedef std::list<Partition>::iterator PartitionRef;
typedef std::set<PartitionRef>::iterator TreeRef;

struct scmp { 
  bool operator()(const PartitionRef & c1, const PartitionRef & c2) const { 
    if (c1->size == c2->size) {
      return c1->addr < c2->addr; 
    }
    else {
      return c1->size > c2->size; 
    }
  } 
};

struct Simulator {
  std::list<Partition> partitions;
  int64_t page_size;
  int64_t total_requested_pages = 0;

  unordered_map<long, list<PartitionRef>> tagged_blocks;  // quick access to all tagged partitions
  set<PartitionRef, scmp> free_blocks;                    // sorted partitions by size/address

  Simulator(int64_t page)
  {
    page_size = page;
  }

  int64_t get_size(int64_t size) {
    int64_t new_size;
    int64_t mod = size % page_size;
    int64_t req_pages;

    // the requested size is a multiple of the page size
    if (mod == 0) {
      req_pages = size / page_size;         // the requested pages is the division of page_size
    }
    else {
      req_pages = size / page_size + 1;      // round up to one extra page if there's a remainder
    }
    
    total_requested_pages += req_pages;      // add to the total requested pages
    new_size = req_pages * page_size;        // compute new size
    
    return new_size;
  }

  void split(list<Partition>::iterator best_part, long tag, int64_t size) {
    Partition p2;   // create new partition for the free partition

    // calculate the second half before we replace the values of the first half
    p2.tag = -1;
    p2.size = best_part->size - size;         // subtract the current partition size with the requested size
    p2.addr = best_part->addr + size;         // re-compute the address based on the requested size
  
    best_part->tag = tag;                     // replace the tag to allocate it
    best_part->size = size;                   // replace the current size with the requested size

    partitions.insert(next(best_part), p2);   // insert next to the first half
    free_blocks.insert(next(best_part));      // save the free block
  }

  void allocate(long tag, int64_t size)
  {
    //printf("alloc %ld  %ld\n", tag, size);
    PartitionRef best_part;
    bool found = false;

    // create the first partition
    if (partitions.empty()) {
      Partition first_partition;

      int64_t computed_size = get_size(size);   // calculate the size
      first_partition.tag = -1;                 // mark it as free
      first_partition.addr = 0;                 // mark the first partition with 0 addr
      first_partition.size = computed_size;     // assign the computed size to the partition

      partitions.push_back(first_partition);    // push the newly created partition in the list of partitions
      free_blocks.insert(partitions.begin());   // insert the pointer to the free partition into the set
    }
  
    if (!free_blocks.empty()) {
      TreeRef part_ptr = free_blocks.begin();
      best_part = *part_ptr;
      if (best_part->size >= size) {           // the partition is fits the requested size
        found = true;                          // since the partition is suitable, mark as found
        free_blocks.erase(part_ptr);           // delete free block since it'll be occupied later
      }
    }

    // the largest partition wasn't found or the partition wasn't big enough
    if (!found) {
      Partition new_partition;
      new_partition.tag = -1;

      // check if the last partition is free
      // - size is computed based on the (last free partition) + (requested size - last free partition)
      if (partitions.back().tag < 0) {
        new_partition.size = partitions.back().size + get_size(size - partitions.back().size);
        new_partition.addr = partitions.back().addr;

        free_blocks.erase(prev(partitions.end()));    // remove the partition from free_blocks
        partitions.pop_back();                        // delete the last partition
        partitions.push_back(new_partition);          // add new partition to the end of the list
      } 
      else {
        // just create a new partition, compute the address by adding the previous address + previous size and the
        new_partition.size = get_size(size);
        new_partition.addr = partitions.back().addr + partitions.back().size;
        partitions.push_back(new_partition);
      }

      free_blocks.insert(prev(partitions.end()));    // add the newly created partition to the list of free_blocks 
      
      TreeRef tree_ptr = free_blocks.begin();        // the best partition is the first partition
      best_part = *tree_ptr;                         // de-reference it to get a pointer to the partition
      free_blocks.erase(tree_ptr);                   // erase the partition since it'll be occupied 

    }

    // since the best_partition was either found or created, allocate it
    if (best_part->size > size) {
      Partition temp = *best_part;                // we want to replace the values, use a temporary variable 
      temp.tag = tag;                             // replace the tag
      temp.size = best_part->size;                // replace the size, will just end up being the same size
      temp.addr = best_part->addr;                // replace the addr, will just end up being the same
      *best_part = temp;

      split(best_part, tag, size);                // split the partition
      tagged_blocks[tag].push_back(best_part);    // add the allocated partition to tagged_blocks
    } 
    else if (best_part->size == size) {           // the best partition fits the requested size, don't split
      Partition temp = *best_part;
      temp.tag = tag;
      temp.size = best_part->size;
      temp.addr = best_part->addr;

      *best_part = temp;
      tagged_blocks[tag].push_back(best_part);
    } 
  }

  void deallocate(long tag)
  {
    //printf("dealloc %ld\n", tag);
    list<PartitionRef> partitions_with_tag = tagged_blocks[tag];
    for (PartitionRef current : partitions_with_tag) {
      current->tag = -1;

      // merge the current partitions with the left
      if (current != partitions.begin() && prev(current)->tag == -1) {
        free_blocks.erase(prev(current));     // erase the previous partition that's free from the tree
  
        current->size += prev(current)->size; // combine the sizes with the previous size
        current->addr = prev(current)->addr;  // replace the addr with the previous addr

        partitions.erase(prev(current));      // then delete the partition
      }

      // merge current partition with the right
      if (current != prev(partitions.end()) && next(current)->tag == -1) {
        free_blocks.erase(next(current));       // erase the next partition that's free from the tree

        current->size += next(current)->size;   // combine the sizes with the next size
        partitions.erase(next(current));        // then delete the partition
      }

      // do a check to see if it's actually free
      if (current->tag == -1) {                
        free_blocks.insert(current);
      }
    }
    
    tagged_blocks[tag].clear();                // erase all the pointers to the partitions with the given tag
  }

  MemSimResult getStats()
  {
    MemSimResult result;
  
    // if there are any free blocks
    if (free_blocks.size() > 0) {
      TreeRef partition_ptr = free_blocks.begin();                  // the first block is the biggest
      PartitionRef max_free_parition = *partition_ptr;              // de-reference the pointer
      result.max_free_partition_size = max_free_parition->size;     // de-reference the max size
      result.max_free_partition_address = max_free_parition->addr;  // de-reference the max addr
    }
    else{
      // there aren't any free blocks, assign the values to 0
      result.max_free_partition_size = 0;
      result.max_free_partition_address = 0;
    }
    result.n_pages_requested = total_requested_pages;
    return result;
  }

  void check_consistency()
  {
    // mem_sim() calls this after every request to make sure all data structures
    // are consistent. Since this will probablly slow down your code, you should
    // disable comment out the call below before submitting your code for grading.
    check_consistency_internal();
  }

  void check_consistency_internal()
  {
    // you do not need to implement this method at all - this is just my suggestion
    // to help you with debugging

    // here are some suggestions for consistency checks (see appendix also):

    // int tagged_size = 0;
    // for (auto blocks = tagged_blocks.begin(); blocks != tagged_blocks.end(); ++blocks) {
    //   tagged_size += tagged_blocks[blocks->first].size();
    // }

    // int size1 = tagged_size + free_blocks.size();
    // int size2 = partitions.size();

    // printf("----- tagged_blocks (%ld) + free_blocks (%ld) size: %ld   partitions_size: %ld -----\n", 
    // tagged_size, free_blocks.size(), size1, size2);
    //print_partitions();
    //assert(size1==size2);

    // make sure the sum of all partition sizes in your linked list is
    // the same as number of page requests * page_size

    // make sure your addresses are correct

    // make sure the number of all partitions in your tag data structure +
    // number of partitions in your free blocks is the same as the size
    // of the linked list

    // make sure that every free partition is in free blocks

    // make sure that every partition in free_blocks is actually free

    // make sure that none of the partition sizes or addresses are < 1

  }

    void print_tagged() {
    for (auto blocks = tagged_blocks.begin(); blocks != tagged_blocks.end(); ++blocks) {
      cout << tagged_blocks[blocks->first].size() << endl;
      printf("Tag[%ld]: {", blocks->first);
      for (PartitionRef part : tagged_blocks[blocks->first]) {
        cout << part->addr << " ";    
      }
      printf("}\n");
    }
  }

  void print_partitions() {
    vector<int> tags;
    vector<int64_t> sizes;
    vector<int64_t> addrs;
    for (auto i : partitions) {
      tags.push_back(i.tag);
      sizes.push_back(i.size);
      addrs.push_back(i.addr);
    }    
    //------------- tag row -------------------------------------//
    cout << "------------------------------------------" << endl;
    cout << "Tag:   |  ";
    for (unsigned long int i=0; i < tags.size(); i++) {
      cout << tags[i] << "  |  "; 
    }
    cout << endl;
    //------------- size row -------------------------------------//
    cout << "Size:  |  ";
    for (unsigned long int i=0; i < sizes.size(); i++) {
      cout << sizes[i] << "  |  "; 
    }
    cout << endl;
    //------------- addr row -------------------------------------//
    cout << "Addr:  |  ";
    for (unsigned long int i=0; i < addrs.size(); i++) {
      cout << addrs[i] << "  |  "; 
    }
    cout << endl;
    cout << "------------------------------------------" << endl;
  }

};

MemSimResult mem_sim(int64_t page_size, const std::vector<Request> & requests)
{
  // if you decide to use the simulator class above, you probably do not need
  // to modify below at all
  Simulator sim(page_size);
  for (const auto & req : requests) {
    if (req.tag < 0) {
      sim.deallocate(-req.tag);
    } else {
      sim.allocate(req.tag, req.size);
    }
    sim.check_consistency();
  }
  // sim.print_partitions();
  // sim.print_tagged();

  return sim.getStats();
}
