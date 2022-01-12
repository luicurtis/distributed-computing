#include "core/circular_queue.h"
#include "core/utils.h"
#include <iomanip>
#include <pthread.h>
#include <stdlib.h>

class Producer {
    // Define Producer Class here
};

class Consumer {
    // Define Producer Class here
};

class ProducerConsumerProblem {
  long n_items;
  int n_producers;
  int n_consumers;
  CircularQueue production_buffer;

  // Dynamic array of thread identifiers for producer and consumer threads.
  // Use these identifiers while creating the threads and joining the threads.
  pthread_t *producer_threads;
  pthread_t *consumer_threads;

  Producer *producers;
  Consumer *consumers;

  int active_producer_count;
  int active_consumer_count;

 // define any other members, mutexes, condition variables here

public:
  // The following 6 methods should be defined in the implementation file (solution.cpp)
  ProducerConsumerProblem(long _n_items, int _n_producers, int _n_consumers,
                          long _queue_size);
  ~ProducerConsumerProblem();
  void startProducers();
  void startConsumers();
  void joinProducers();
  void joinConsumers();
  void printStats();
};
