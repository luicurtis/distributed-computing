#include <pthread.h>
#include <stdlib.h>

#include <iomanip>
#include <stdexcept>

#include "core/circular_queue.h"
#include "core/utils.h"

class Producer {
 public:
  // Define Producer Class here
  int id;
  int item_start_val;  // starting item value for the producer
  int increment_val;   // increment value for the items
  double time_taken;
  long np;  // # of items to produce
  long num_type[3];
  long val_type[3];
  int remainder;  // use to determine if producer 0 needs to create extra items

  CircularQueue *buffer;
  pthread_mutex_t *buffer_mut;
  pthread_mutex_t *active_producer_count_mut;
  pthread_mutex_t *active_consumer_count_mut;
  pthread_cond_t *buffer_full;
  pthread_cond_t *buffer_empty;
  int *active_producer_count;
  int *active_consumer_count;

  Producer();
  ~Producer();
};

class Consumer {
 public:
  // Define Producer Class here
  double time_taken;
  long num_type[3];
  long val_type[3];

  CircularQueue *buffer;
  pthread_mutex_t *buffer_mut;
  pthread_mutex_t *active_producer_count_mut;
  pthread_mutex_t *active_consumer_count_mut;
  pthread_cond_t *buffer_full;
  pthread_cond_t *buffer_empty;
  int *active_producer_count;
  int *active_consumer_count;

  Consumer();
  ~Consumer();
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
  pthread_mutex_t buffer_mut;
  pthread_mutex_t producer_count_mut;
  pthread_mutex_t consumer_count_mut;
  pthread_cond_t buffer_full;
  pthread_cond_t buffer_empty;

 public:
  // The following 6 methods should be defined in the implementation file
  // (solution.cpp)
  ProducerConsumerProblem(long _n_items, int _n_producers, int _n_consumers,
                          long _queue_size);
  ~ProducerConsumerProblem();
  void startProducers();
  void startConsumers();
  void joinProducers();
  void joinConsumers();
  void printStats();
};
