#include "solution.h"

void producerFunction(void *_arg) {
  // Parse the _arg passed to the function.
  // Enqueue `n` items into the `production_buffer`. The items produced should
  // be 0, 1, 2,..., (n-1).
  // Keep track of the number of items produced, their type 
  // and the value produced by the thread
  // The producer that was last active should ensure that all the consumers have
  // finished. NOTE: Each thread will enqueue `n` items.
  // Use mutex variables and conditional variables as necessary.
}

void consumerFunction(void *_arg) {
  // Parse the _arg passed to the function.
  // The consumer thread will consume items by dequeueing the items from the
  // `production_buffer`.
  // Keep track of the number of items consumed and their value and type
  // Once the productions is complete and the queue is also empty, the thread
  // will exit. NOTE: The number of items consumed by each thread need not be the same
  // Use mutex variables and conditional variables as necessary.
}

ProducerConsumerProblem::ProducerConsumerProblem(long _n_items,
                                                 int _n_producers,
                                                 int _n_consumers,
                                                 long _queue_size)
    : n_items(_n_items), n_producers(_n_producers), n_consumers(_n_consumers),
      production_buffer(_queue_size) {
  std::cout << "Constructor\n";
  std::cout << "Number of producers: " << n_producers << "\n";
  std::cout << "Number of consumers: " << n_consumers << "\n";

  if (n_consumers) {
    consumers = new Consumer[n_consumers];
    consumer_threads = new pthread_t[n_consumers];
  }
  if (n_producers) {
    producers = new Producer[n_producers];
    producer_threads = new pthread_t[n_producers];
  }

  // Initialize all mutex and conditional variables here.
}

ProducerConsumerProblem::~ProducerConsumerProblem() {
  std::cout << "Destructor\n";
  if (n_producers) {
    delete[] producers;
    delete[] producer_threads;
  }
  if (n_consumers) {
    delete[] consumers;
    delete[] consumer_threads;
  }
  // Destroy all mutex and conditional variables here.
}

void ProducerConsumerProblem::startProducers() {
  std::cout << "Starting Producers\n";
  active_producer_count = n_producers;
  // Compute number of items for each thread, and number of items per type per thread

  // Create producer threads P1, P2, P3,.. using pthread_create.
}

void ProducerConsumerProblem::startConsumers() {
  std::cout << "Starting Consumers\n";
  active_consumer_count = n_consumers;
  // Create consumer threads C1, C2, C3,.. using pthread_create.
}

void ProducerConsumerProblem::joinProducers() {
  std::cout << "Joining Producers\n";
  // Join the producer threads with the main thread using pthread_join
}

void ProducerConsumerProblem::joinConsumers() {
  std::cout << "Joining Consumers\n";
  // Join the consumer threads with the main thread using pthread_join
}

void ProducerConsumerProblem::printStats() {
  std::cout << "Producer stats\n";
  std::cout << "producer_id, items_produced_type0:value_type0, items_produced_type1:value_type1, items_produced_type2:value_type2, total_value_produced, time_taken\n";

  // Make sure you print the producer stats in the following manner
  //  0, 125000:31249750000, 83333:55555111112, 41667:38194638888, 124999500000, 0.973188
  //  1, 125000:31249875000, 83333:55555194445, 41667:38194680555, 124999750000, 1.0039
  //  2, 125000:31250000000, 83333:55555277778, 41667:38194722222, 125000000000, 1.02925
  //  3, 125000:31250125000, 83333:55555361111, 41667:38194763889, 125000250000, 0.999188

  long total_produced[3];  // total produced per type
  long total_value_produced[3];  // total value produced per type
  for (int i = 0; i < n_producers; i++) {
    // Print per producer statistics with above format
  }

  std::cout << "Total produced = " << total_produced[0]+total_produced[1]+total_produced[2] << "\n";
  std::cout << "Total value produced = " << total_value_produced[0]+total_value_produced[1]+total_value_produced[2] << "\n";
  std::cout << "Consumer stats\n";
  std::cout << "consumer_id, items_consumed_type0:value_type0, items_consumed_type1:value_type1, items_consumed_type2:value_type2, time_taken";
 
  // Make sure you print the consumer stats in the following manner
  // 0, 256488:63656791749, 163534:109699063438, 87398:79885550318, 1.02899
  // 1, 243512:61342958251, 169798:112521881008, 79270:72893255236, 1.02891

  long total_consumed[3];   // total consumed per type
  long total_value_consumed[3];    // total value consumed per type = 0;
  for (int i = 0; i < n_consumers; i++) {
    // Print per consumer statistcs with above format
  }

  std::cout << "Total consumed = " << total_consumed[0]+total_consumed[1]+total_consumed[2] << "\n";
  std::cout << "Total value consumed = " << total_value_consumed[0]+total_value_consumed[1]+total_value_consumed[2] << "\n";
}
