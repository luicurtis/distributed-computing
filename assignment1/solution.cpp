#include "solution.h"

void *producerFunction(void *_arg) {
  // Parse the _arg passed to the function.
  // Enqueue `n` items into the `production_buffer`. The items produced should
  // be 0, 1, 2,..., (n-1).
  // Keep track of the number of items produced, their type
  // and the value produced by the thread
  // The producer that was last active should ensure that all the consumers have
  // finished. NOTE: Each thread will enqueue `n` items.
  // Use mutex variables and conditional variables as necessary.
  // Each producer enqueues `np` items where np=n/nProducers except for producer
  // 0
  timer t;
  t.start();

  Producer *producer = (Producer *)_arg;

  long item_val = producer->item_start_val;
  long items_produced = 0;
  long cur_type_produced = 0;
  int cur_type = 0;
  long item_val_before_remainder =
      (producer->np - producer->remainder) * producer->increment_val;

  while (items_produced < producer->np) {
    // acquire lock on buffer and try to add to buffer
    CircularQueueEntry item = {item_val, producer->id, cur_type};
    pthread_mutex_lock(producer->buffer_mut);
    bool ret = producer->buffer->enqueue(item.value, item.type, item.source);

    if (ret == true) {
      if (producer->buffer->itemCount() >= 1) {
        // The queue is no longer empty
        // Signal all consumers indicating queue is not empty
        pthread_cond_broadcast(producer->buffer_empty);
      }
      // unlock the buffer mutex
      pthread_mutex_unlock(producer->buffer_mut);

      // update stat variables
      items_produced++;
      cur_type_produced++;
      producer->val_type[cur_type] += item.value;

      // Increment for next item value
      // Check if producer 0 and if there was a remainder
      if (producer->id == 0 && producer->remainder &&
          item.value >= item_val_before_remainder) {
        item_val += 1;
      } else {
        item_val += producer->increment_val;
      }

      // update cur_type of item
      if (cur_type_produced == producer->num_type[cur_type]) {
        cur_type++;
        cur_type_produced = 0;
      }
    } else {
      // production_buffer is full, so block on conditional variable waiting for
      // consumer to signal.
      // NOTE: pthread_cont_wait() will release the mutex
      pthread_cond_wait(producer->buffer_full, producer->buffer_mut);
      // unlock the buffer mutex
      pthread_mutex_unlock(producer->buffer_mut);
    }
  }

  // After production is completed: Update the number of producers that are
  // currently active.
  pthread_mutex_lock(producer->active_producer_count_mut);
  *producer->active_producer_count -= 1;

  if (*producer->active_producer_count == 0) {
    // The producer that was last active (can be determined using
    // `active_producer_count`) will keep signalling the consumers until all
    // consumers have finished (can be determined using
    // `active_consumer_count`).
    pthread_mutex_unlock(producer->active_producer_count_mut);
    while (true) {
      pthread_mutex_lock(producer->active_consumer_count_mut);
      if (*producer->active_consumer_count > 0) {
        pthread_mutex_unlock(producer->active_consumer_count_mut);
        pthread_cond_broadcast(producer->buffer_empty);
      } else {
        pthread_mutex_unlock(producer->active_consumer_count_mut);
        break;
      }
    }

  } else {
    // unlock the producer count mutex
    pthread_mutex_unlock(producer->active_producer_count_mut);
  }

  double time_taken = t.stop();
  producer->time_taken = time_taken;

  return nullptr;
}

void *consumerFunction(void *_arg) {
  // Parse the _arg passed to the function.
  // The consumer thread will consume items by dequeueing the items from the
  // `production_buffer`.
  // Keep track of the number of items consumed and their value and type
  // Once the productions is complete and the queue is also empty, the thread
  // will exit. NOTE: The number of items consumed by each thread need not be
  // the same Use mutex variables and conditional variables as necessary.
  // Each consumer dequeues items from the `production_buffer`
  timer t;
  t.start();

  Consumer *consumer = (Consumer *)_arg;

  while (true) {
    CircularQueueEntry item = {0, 0, 0};

    // acquire lock on buffer and try to remove an item
    pthread_mutex_lock(consumer->buffer_mut);
    bool ret = consumer->buffer->dequeue(&item.value, &item.source, &item.type);

    if (ret == true) {
      if (consumer->buffer->itemCount() <=
          consumer->buffer->getCapacity() - 1) {
        // The queue is no longer full
        // Signal all producers indicating queue is not full
        pthread_cond_broadcast(consumer->buffer_full);
      }
      // unlock the buffer mutex
      pthread_mutex_unlock(consumer->buffer_mut);
      // update stat variables
      consumer->num_type[item.type]++;
      consumer->val_type[item.type] += item.value;
    } else {
      // production_buffer is empty, so block on conditional variable waiting
      // for producer to signal. The thread can wake up because of 2 scenarios
      // NOTE: pthread_cont_wait() will release the mutex
      pthread_cond_wait(consumer->buffer_empty, consumer->buffer_mut);

      // Scenario 1: There are no more active producers (i.e., production is
      // complete) and the queue is empty. This is the exit condition for
      // consumers, and at this point consumers should decrement
      // `active_consumer_count`
      pthread_mutex_lock(consumer->active_producer_count_mut);
      if (*consumer->active_producer_count == 0 &&
          consumer->buffer->isEmpty()) {
        // unlock the buffer mutex
        pthread_mutex_unlock(consumer->buffer_mut);
        pthread_mutex_unlock(consumer->active_producer_count_mut);
        pthread_mutex_lock(consumer->active_consumer_count_mut);
        *consumer->active_consumer_count -= 1;
        pthread_mutex_unlock(consumer->active_consumer_count_mut);
        break;  // exit condition
      }
      // Scenario 2 : The queue is not empty and / or the producers are active.
      // Continue consuming.
      else {
        pthread_mutex_unlock(consumer->active_producer_count_mut);
        // unlock the buffer mutex
        pthread_mutex_unlock(consumer->buffer_mut);
      }
    }
  }

  double time_taken = t.stop();
  consumer->time_taken = time_taken;

  return nullptr;
}

Producer::Producer() : num_type(), val_type() {
  id = 0;
  item_start_val = 0;
  increment_val = 0;
  time_taken = 0.0;
  np = 0;
  remainder = 0;
  buffer = nullptr;
  buffer_mut = nullptr;
  active_producer_count_mut = nullptr;
  active_consumer_count_mut = nullptr;
  buffer_full = nullptr;
  buffer_empty = nullptr;
  active_producer_count = nullptr;
  active_consumer_count = nullptr;
}

Producer::~Producer() {}

Consumer::Consumer() : num_type(), val_type() {
  time_taken = 0.0;
  buffer = nullptr;
  buffer_mut = nullptr;
  active_producer_count_mut = nullptr;
  active_consumer_count_mut = nullptr;
  buffer_full = nullptr;
  buffer_empty = nullptr;
  active_producer_count = nullptr;
  active_consumer_count = nullptr;
}

Consumer::~Consumer() {}

ProducerConsumerProblem::ProducerConsumerProblem(long _n_items,
                                                 int _n_producers,
                                                 int _n_consumers,
                                                 long _queue_size)
    : n_items(_n_items),
      n_producers(_n_producers),
      n_consumers(_n_consumers),
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

  // Check edge cases on inputs
  if (_n_items <= 0 || n_consumers <= 0 || n_producers <= 0 ||
      _queue_size <= 0) {
    throw std::invalid_argument(
        "The commandline arguments: nItems, nProducers, nConsumers, and "
        "bufferSize cannot be less than or "
        "equal to 0.");
  }

  // Initalize active producer/consumer counts
  // This is to prevent the case where the producers finish producing before
  // consumers start.
  active_producer_count = n_producers;
  active_consumer_count = n_consumers;

  // Initialize all mutex and conditional variables here.
  pthread_mutex_init(&buffer_mut, NULL);
  pthread_mutex_init(&producer_count_mut, NULL);
  pthread_mutex_init(&consumer_count_mut, NULL);
  pthread_cond_init(&buffer_full, NULL);
  pthread_cond_init(&buffer_empty, NULL);
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
  pthread_mutex_destroy(&buffer_mut);
  pthread_mutex_destroy(&producer_count_mut);
  pthread_mutex_destroy(&consumer_count_mut);
  pthread_cond_destroy(&buffer_full);
  pthread_cond_destroy(&buffer_empty);
}

void ProducerConsumerProblem::startProducers() {
  std::cout << "Starting Producers\n";
  // active_producer_count = n_producers;
  // Compute number of items for each thread, and number of items per type
  // per thread
  long np = n_items / n_producers;
  long num_type_0 = np / 2;
  long num_type_1 = np / 3;
  long num_type_2 = np - num_type_0 - num_type_1;

  // Initialize and set thread detached attribute
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // Create producer threads P1, P2, P3,.. using pthread_create.
  for (int i = 0; i < n_producers; i++) {
    if (i == 0 && n_items % n_producers) {
      int remainder = n_items % n_producers;
      producers[i].remainder = remainder;
      producers[i].np = np + remainder;
      producers[i].num_type[0] = (np + remainder) / 2;
      producers[i].num_type[1] = (np + remainder) / 3;
      producers[i].num_type[2] =
          np + remainder - producers[i].num_type[0] - producers[i].num_type[1];
    } else {
      producers[i].remainder = 0;
      producers[i].np = np;
      producers[i].num_type[0] = num_type_0;
      producers[i].num_type[1] = num_type_1;
      producers[i].num_type[2] = num_type_2;
    }

    producers[i].id = i;
    producers[i].item_start_val = i;
    producers[i].increment_val = n_producers;
    producers[i].buffer = &production_buffer;
    producers[i].buffer_mut = &buffer_mut;
    producers[i].active_producer_count_mut = &producer_count_mut;
    producers[i].active_consumer_count_mut = &consumer_count_mut;
    producers[i].buffer_full = &buffer_full;
    producers[i].buffer_empty = &buffer_empty;
    producers[i].active_producer_count = &active_producer_count;
    producers[i].active_consumer_count = &active_consumer_count;

    pthread_create(&producer_threads[i], &attr, producerFunction,
                   (void *)&producers[i]);
  }

  // Free attribute
  pthread_attr_destroy(&attr);
}

void ProducerConsumerProblem::startConsumers() {
  std::cout << "Starting Consumers\n";
  // active_consumer_count = n_consumers;

  // Initialize and set thread detached attribute
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // Create consumer threads C1, C2, C3,.. using pthread_create.
  for (int i = 0; i < n_consumers; i++) {
    // set Consumer pointers
    consumers[i].buffer = &production_buffer;
    consumers[i].buffer_mut = &buffer_mut;
    consumers[i].active_producer_count_mut = &producer_count_mut;
    consumers[i].active_consumer_count_mut = &consumer_count_mut;
    consumers[i].buffer_full = &buffer_full;
    consumers[i].buffer_empty = &buffer_empty;
    consumers[i].active_producer_count = &active_producer_count;
    consumers[i].active_consumer_count = &active_consumer_count;

    pthread_create(&consumer_threads[i], NULL, consumerFunction,
                   (void *)&consumers[i]);
  }

  // Free attribute
  pthread_attr_destroy(&attr);
}

void ProducerConsumerProblem::joinProducers() {
  std::cout << "Joining Producers\n";
  // Join the producer threads with the main thread using pthread_join
  for (int i = 0; i < n_producers; i++) {
    int rc = pthread_join(producer_threads[i], NULL);
    if (rc) {
      printf("ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
  }
}

void ProducerConsumerProblem::joinConsumers() {
  std::cout << "Joining Consumers\n";
  // Join the consumer threads with the main thread using pthread_join
  for (int i = 0; i < n_consumers; i++) {
    int rc = pthread_join(consumer_threads[i], NULL);
    if (rc) {
      printf("ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
  }
}

void ProducerConsumerProblem::printStats() {
  std::cout << "Producer stats\n";
  std::cout
      << "producer_id, items_produced_type0:value_type0, "
         "items_produced_type1:value_type1, items_produced_type2:value_type2, "
         "total_value_produced, time_taken\n";

  // Make sure you print the producer stats in the following manner
  //  0, 125000:31249750000, 83333:55555111112, 41667:38194638888, 124999500000,
  //  0.973188 1, 125000:31249875000, 83333:55555194445, 41667:38194680555,
  //  124999750000, 1.0039 2, 125000:31250000000, 83333:55555277778,
  //  41667:38194722222, 125000000000, 1.02925 3, 125000:31250125000,
  //  83333:55555361111, 41667:38194763889, 125000250000, 0.999188

  long total_produced[3] = {0};        // total produced per type
  long total_value_produced[3] = {0};  // total value produced per type
  for (int i = 0; i < n_producers; i++) {
    // Print per producer statistics with above format
    std::cout << i << ", ";
    std::cout << producers[i].num_type[0] << ":" << producers[i].val_type[0]
              << ", ";
    std::cout << producers[i].num_type[1] << ":" << producers[i].val_type[1]
              << ", ";
    std::cout << producers[i].num_type[2] << ":" << producers[i].val_type[2]
              << ", ";
    std::cout << producers[i].val_type[0] + producers[i].val_type[1] +
                     producers[i].val_type[2]
              << ", ";
    std::cout << producers[i].time_taken << "\n";

    total_produced[0] += producers[i].num_type[0];
    total_produced[1] += producers[i].num_type[1];
    total_produced[2] += producers[i].num_type[2];
    total_value_produced[0] += producers[i].val_type[0];
    total_value_produced[1] += producers[i].val_type[1];
    total_value_produced[2] += producers[i].val_type[2];
  }

  std::cout << "Total produced = "
            << total_produced[0] + total_produced[1] + total_produced[2]
            << "\n";
  std::cout << "Total value produced = "
            << total_value_produced[0] + total_value_produced[1] +
                   total_value_produced[2]
            << "\n";
  std::cout << "Consumer stats\n";
  std::cout << "consumer_id, items_consumed_type0:value_type0, "
               "items_consumed_type1:value_type1, "
               "items_consumed_type2:value_type2, time_taken\n";

  // Make sure you print the consumer stats in the following manner
  // 0, 256488:63656791749, 163534:109699063438, 87398:79885550318, 1.02899
  // 1, 243512:61342958251, 169798:112521881008, 79270:72893255236, 1.02891

  long total_consumed[3] = {0};        // total consumed per type
  long total_value_consumed[3] = {0};  // total value consumed per type = 0;
  for (int i = 0; i < n_consumers; i++) {
    // Print per consumer statistcs with above format
    std::cout << i << ", ";
    std::cout << consumers[i].num_type[0] << ":" << consumers[i].val_type[0]
              << ", ";
    std::cout << consumers[i].num_type[1] << ":" << consumers[i].val_type[1]
              << ", ";
    std::cout << consumers[i].num_type[2] << ":" << consumers[i].val_type[2]
              << ", ";
    std::cout << consumers[i].time_taken << "\n";

    total_consumed[0] += consumers[i].num_type[0];
    total_consumed[1] += consumers[i].num_type[1];
    total_consumed[2] += consumers[i].num_type[2];
    total_value_consumed[0] += consumers[i].val_type[0];
    total_value_consumed[1] += consumers[i].val_type[1];
    total_value_consumed[2] += consumers[i].val_type[2];
  }

  std::cout << "Total consumed = "
            << total_consumed[0] + total_consumed[1] + total_consumed[2]
            << "\n";
  std::cout << "Total value consumed = "
            << total_value_consumed[0] + total_value_consumed[1] +
                   total_value_consumed[2]
            << "\n";
}
