// ======================================================================
// You must modify this file and then submit it for grading to D2L.
// ======================================================================
//
// count_pi() calculates the number of pixels that fall into a circle
// using the algorithm explained here:
//
// https://en.wikipedia.org/wiki/Approximations_of_%CF%80
//
// count_pixels() takes 2 paramters:
//  r         =  the radius of the circle
//  n_threads =  the number of threads you should create
//
// Currently the function ignores the n_threads parameter. Your job is to
// parallelize the function so that it uses n_threads threads to do
// the computation.

#include "calcpi.h"
#include <iostream>
#include <pthread.h>  

using namespace std;

struct Task{
    int r;
    double start_x;
    double end_x;
    uint64_t partial_count;
};

// each thread will run this method and find where they will start_x given the 
// some contents of this function were based on code from lines 198-208:
// - https://github.com/colinauyeung/CPSC457-F22-Notes/blob/master/Week6/threads/workdivison.cpp
void * thread_task(void * args){
  struct Task * in = ((struct Task *) args);
  
  int r = in -> r;
  double rsq = double(r) * r;

  double start_x = in -> start_x; 
  double end_x = in -> end_x;

  uint64_t partial_count = 0;
  for(double x = start_x+1; x <= end_x ; x ++){
    for(double y = 0 ; y <= r ; y ++){
      if( x*x + y*y <= rsq) {
        partial_count ++;
      }
    }
  }
  // keep the partial_count for the particular thread in the struct
  in->partial_count = partial_count;

  pthread_exit(NULL);
}

// contents of this function were based on code from lines 330-360:
// - https://github.com/colinauyeung/CPSC457-F22-Notes/blob/master/Week6/threads/workdivison.cpp
uint64_t count_pixels(int r, int n_threads) {
  // ============== setting up thread ============== //
  pthread_t thread_pool[n_threads];
  Task tasks[n_threads];

  int div = r / n_threads;
  int mod = r % n_threads;
  int lastend = 0;

  // set up which threads are doing what
  for(int i = 0; i < n_threads; i++){
    tasks[i].r = r;
    tasks[i].partial_count = 0;
    tasks[i].start_x = lastend;

    if(i < mod){
        tasks[i].end_x = tasks[i].start_x + div + 1;
    }
    else{
        tasks[i].end_x = tasks[i].start_x + div;
    }
    //Make sure to update where the last element is...
    lastend = tasks[i].end_x;
  }

  // create threads and run on the work they are assigned to
  // - each thread counts pixels for the x-range assigned to it and updates its partial count
  for(int i = 0; i < n_threads; i++){
      pthread_create(&thread_pool[i], NULL, thread_task, (void *) &tasks[i]);
  }

  // join threads
  for(int i = 0; i< n_threads; i++){
      pthread_join(thread_pool[i], NULL);    
  }

  // add up all partial_counts to get final result
  uint64_t count = 0;
  for(int i = 0; i< n_threads; i++){
      count = count + tasks[i].partial_count;
  }
  // ================================================ //

  return count * 4 + 1;
}
