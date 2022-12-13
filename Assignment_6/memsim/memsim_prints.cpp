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
  long tag;  // long or int?
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
  int64_t largest_free_partition_size = 0;
  int64_t largest_free_partition_addr = 0;

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

    if (mod == 0) { 
      req_pages = size / page_size;
    } else {
      req_pages = size / page_size + 1;
    }
    total_requested_pages += req_pages;
    new_size = req_pages * page_size;
    return new_size;
  }

  void split(list<Partition>::iterator best_part, long tag, int64_t size) {
      
    // splitting
    Partition p1, p2;

    // calculate the second half before we replace the values of the first half
    p2.tag = -1;
    p2.size = best_part->size - size;
    p2.addr = best_part->addr + size;
  
    best_part->tag = tag;
    best_part->size = size;

    // printf("Second half:\n Tag:      %d\n Size:     %ld\n Address:  %ld\n", p2.tag, p2.size, p2.addr);
    // printf("First half:\n Tag:      %d\n Size:     %ld\n Address:  %ld\n", best_part->tag, best_part->size, best_part->addr);
    
    //tagged_blocks[tag].push_back(best_part);  
    auto res = partitions.insert(next(best_part), p2);   // insert next to the first half
    assert(res == next(best_part));
  
    // printf("Second half:\n Tag:      %d\n Size:     %ld\n Address:  %ld\n", p2.tag, p2.size, p2.addr);

    free_blocks.insert(next(best_part));      // save the free block
    // cout << "free_blocks size in split: " << free_blocks.size() << endl;

  }

  void allocate(long tag, int64_t size)
  {
    //printf("alloc %d  %d\n", tag, size);
    bool found = false;
    int64_t prev_size = -1;
    int largest_ind = -1;
    int ind=0;

    PartitionRef best_part;

    // create the first partition
    if (partitions.empty()) {
      Partition first_partition;

      int64_t computed_size = get_size(size);
      first_partition.tag = -1;
      first_partition.addr = 0;
      first_partition.size = computed_size;

      partitions.push_back(first_partition);
      assert(partitions.begin()->addr==0);
      free_blocks.insert(partitions.begin());   // insert the pointer to the free partition into the set
    }
  
    if (!free_blocks.empty()) {
      //cout << free_blocks.size() << endl;
      TreeRef it = free_blocks.begin();
      best_part = *it;
      //cout << best_part->addr << endl;
      // mark as found if the found size is fits the requested size
      if (best_part->size >= size) {    
        found = true;
        free_blocks.erase(it);        // delete free block since it'll be occupied later
      }
    }

    if (!found) {    // the largest partition wasn't found or the partition wasn't big enough
      //printf("not found, creating a NEW partition\n");
      Partition new_partition;
      new_partition.tag = -1;

      // check if the last partition is free
      // - size is computed based on the (last free partition) + (requested size - last free partition)
      if (partitions.back().tag < 0) {
        //printf("Last partition is free: Tag: %d Size: %d Addr: %d\n", partitions.back().tag, partitions.back().size, partitions.back().addr);
        new_partition.size = partitions.back().size + get_size(size - partitions.back().size);
        new_partition.addr = partitions.back().addr;

        // *** FIX ME, delete partition from the free_blocks first
        free_blocks.erase(prev(partitions.end()));
        partitions.pop_back();
         partitions.push_back(new_partition);          // push new partition at the end
        //free_blocks.erase(prev(partitions.end()));
      } 
      else {
        new_partition.size = get_size(size);
        new_partition.addr = partitions.back().addr + partitions.back().size;
         partitions.push_back(new_partition);          // push new partition at the end
      }
      //printf("Created new parition: \n Tag:      %d\n Size:     %ld\n Address:  %ld\n", 
      //new_partition.tag , new_partition.size, new_partition.addr);
  
      // partitions.push_back(new_partition);          // push new partition at the end
      
      // printf("######checking before we push######: Tag:      %d Size:     %ld Address:  %ld\n", 
      // prev(partitions.end())->tag , prev(partitions.end())->size, prev(partitions.end())->addr);

      //printf("Allocated partition with (Tag: %d Size:%d Addr:%d)\n", new_partition.tag, new_partition.size, new_partition.addr);
      // if (new_partition.size > size) {
      //   split(best_part, tag, size);
      // }
      free_blocks.insert(prev(partitions.end()));
      best_part = prev(partitions.end());

      TreeRef tree_ptr = free_blocks.begin();
      PartitionRef part_ptr = *tree_ptr;

      //cout << "**************************** " << part_ptr->size << endl; 
      free_blocks.erase(tree_ptr);

    } 
    //       printf("Best partition info:\n Requested Size: %d\n Size: %d\n Addr: %d\n", 
    // size, best_part->size, best_part->addr);
    // found best partition
    //else {
    if (best_part->size > size) {
      //free_blocks.erase(it);
      // TO-DO: find the largest empty partition from tree (using best_part pointer) and delete it
      Partition temp = *best_part;
      temp.tag = tag;
      temp.size = best_part->size;
      temp.addr = best_part->addr;
      *best_part = temp;

      //printf("Allocated partition with (Tag: %d Size:%d Addr:%d)\n", temp.tag, temp.size, temp.addr);
      split(best_part, tag, size);          // split the partition
          tagged_blocks[tag].push_back(best_part);

    } 
    else if (best_part->size == size) {    // the best partition fits the requested size, don't split
      Partition temp = *best_part;
      temp.tag = tag;
      temp.size = best_part->size;
      temp.addr = best_part->addr;

      //printf("Allocated partition with (Tag: %d Size:%d Addr:%d)\n", temp.tag, temp.size, temp.addr);
      *best_part = temp;
      tagged_blocks[tag].push_back(best_part);
    } 
  }

  void deallocate(long tag)
  {
    //printf("dealloc %d\n", tag);
    list<PartitionRef> partitions_with_tag = tagged_blocks[tag];
    for (PartitionRef current : partitions_with_tag) {
      current->tag = -1;

      // merge the current partitions with the left
      if (current != partitions.begin() && prev(current)->tag == -1) {
        // erase the previous partition that's free from the tree
        free_blocks.erase(prev(current));
        current->size += prev(current)->size;
        current->addr = prev(current)->addr;
        
        partitions.erase(prev(current));       
      }

      // merge current partition with the right
      if (current != prev(partitions.end()) && next(current)->tag == -1) {
        free_blocks.erase(next(current));
        current->size += next(current)->size;
        partitions.erase(next(current));
      }
      
      if (current->tag == -1) {     // do a check to see if it's actually free
        free_blocks.insert(current);
      }
    }
    // erase all the pointers to the partitions with the given tag
    tagged_blocks[tag].clear();
  }
  MemSimResult getStats()
  {
    MemSimResult result;
    if (free_blocks.size() > 0) {
      TreeRef partition_ptr = free_blocks.begin();
      PartitionRef max_free_parition = *partition_ptr;
      result.max_free_partition_size = max_free_parition->size;
      result.max_free_partition_address = max_free_parition->addr;
    }
    else{
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

    // printf("----- tagged_blocks (%d) + free_blocks (%d) size: %d   partitions_size: %d -----\n", 
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
      printf("Tag[%d]: {", blocks->first);
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
    for (int i=0; i < tags.size(); i++) {
      cout << tags[i] << "  |  "; 
    }
    cout << endl;
    //------------- size row -------------------------------------//
    cout << "Size:  |  ";
    for (int i=0; i < sizes.size(); i++) {
      cout << sizes[i] << "  |  "; 
    }
    cout << endl;
    //------------- addr row -------------------------------------//
    cout << "Addr:  |  ";
    for (int i=0; i < addrs.size(); i++) {
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
