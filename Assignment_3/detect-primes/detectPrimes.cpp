/// ============================================================================
/// Copyright (C) 2022 Pavol Federl (pfederl@ucalgary.ca)
/// All Rights Reserved. Do not distribute this file.
/// ============================================================================
///
/// You must modify this file and then submit it for grading to D2L.
///
/// You can delete all contents of this file and start from scratch if
/// you wish, as long as you implement the detect_primes() function as
/// defined in "detectPrimes.h".

#include "detectPrimes.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <atomic> 

using namespace std;

vector<int64_t> numbers;
vector<int64_t> result;
int numOfThreads;
int64_t num;

int myIndex = 0;
atomic<bool> isPrimeFinal = true;
atomic<bool> stop = false;
atomic<bool> done = false;
pthread_barrier_t barrier;

// returns true if n is prime, otherwise returns false
static bool is_prime(int64_t n, int64_t id) {
  if (!stop.load(memory_order_relaxed)){   
    if (n < 2) return false;
    if (n <= 3) return true;      // 2 and 3 are primes
    if (n % 2 == 0) return false; // handle multiples of 2
    if (n % 3 == 0) return false; // handle multiples of 3

    // each thread should be dealing with individual divisors
    int64_t div = 5 + id * 6;
    int64_t max = sqrt(n);
    while (div <= max) {
      if (n % div == 0) return false;
      if (n % (div + 2) == 0) return false;
      div = div + numOfThreads * 6;
    }
    // didn't find any divisors, so it must be a prime
    return true;
  }
  // thread should be cancelled, just return false
  return false;
}

struct Task{
    int64_t number;
    bool is_prime;
    int id;
};

void * thread_task(void * args){
  struct Task * in = ((struct Task *) args);
  int id = in->id;

  int flag;
  int sizenums = numbers.size();

  while(1){
    isPrimeFinal = true; // re-initialize this variable for every number

    // serial task
    flag = pthread_barrier_wait(&barrier);
    if(flag == PTHREAD_BARRIER_SERIAL_THREAD){
      stop.store(false, memory_order_relaxed);
      num = numbers[myIndex];
      if(myIndex >= sizenums){
        // no more numbers left, set flag 
        done.store(true, memory_order_relaxed);
      }
      myIndex++;  // get next number from nums[] 
    }
    pthread_barrier_wait(&barrier);

    // parallel work 
    // check if done flag has been set, this means all the numbers have been checked
    if (done.load(memory_order_relaxed)){
      pthread_exit(NULL);
    }
    else{
      // store each number and is_prime results into memory
      in->number = num;
      in->is_prime = is_prime(num, id);

      // ANDing two values to get final evaluation if number is prime.
      // i.e. if one of in->is_prime is a 0, the evaluation will be 0 
      //  and we can cancel the other threads.
      isPrimeFinal = (in->is_prime & isPrimeFinal);
      if (!isPrimeFinal) stop.store(true, memory_order_relaxed);
    }

    // serial work - push results in global array
    flag = pthread_barrier_wait(&barrier);
    if(flag == PTHREAD_BARRIER_SERIAL_THREAD){
      if (isPrimeFinal){
        result.push_back(in->number);
      }
    }
    pthread_barrier_wait(&barrier);
  }
}

vector<int64_t> detect_primes(const vector<int64_t> & nums, int n_threads) {
  pthread_barrier_init(&barrier, NULL ,n_threads);
  numOfThreads = n_threads;
  numbers = nums;

  pthread_t thread_pool[n_threads];
  Task tasks[n_threads];            // prepare memory for each thread

  // create threads and call the thread_task function
  for(int i = 0; i < n_threads; i++){
    tasks[i].id = i;
  }
  for(int i = 0; i < n_threads; i++){
    pthread_create(&thread_pool[i], NULL, thread_task, (void *) &tasks[i]);
  }
  for(int i=0; i< n_threads; i++){
    pthread_join(thread_pool[i], NULL);
  }
  return result;
}
