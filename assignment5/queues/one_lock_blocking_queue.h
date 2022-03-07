#include <atomic>
#include <condition_variable>
#include <mutex>

#include "../common/allocator.h"

template <class T>
class Node {
 public:
  T value;
  Node<T>* next;

  Node() : value(0), next(nullptr) {}
};

extern std::atomic<bool> no_more_enqueues;

template <class T>
class OneLockBlockingQueue {
  Node<T>* q_head;
  Node<T>* q_tail;
  std::mutex mtx;
  std::atomic<bool> wakeup_dq;
  std::condition_variable queue_empty;
  CustomAllocator my_allocator_;

 public:
  OneLockBlockingQueue() : my_allocator_() {
    std::cout << "Using OneLockBlockingQueue\n";
  }

  void initQueue(long t_my_allocator_size) {
    std::cout << "Using Allocator\n";
    my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
    // Initialize the queue head or tail here
    Node<T>* new_node = (Node<T>*)my_allocator_.newNode();
    new_node->next = nullptr;
    q_head = new_node;
    q_tail = new_node;
    wakeup_dq = false;
  }

  void enqueue(T value) {
    Node<T>* new_node = (Node<T>*)my_allocator_.newNode();
    new_node->value = value;
    new_node->next = nullptr;

    std::unique_lock<std::mutex> lck(mtx);
    // Append to q_tail and update the queue
    q_tail->next = new_node;
    q_tail = new_node;

    // signal wakeup for waiting dequeue operations
    if (wakeup_dq.load() == true || no_more_enqueues.load() == true) {
      // queue_empty.notify_one();
      queue_empty.notify_all();
    }
  }

  bool dequeue(T* value) {
    std::unique_lock<std::mutex> lck(mtx);
    Node<T>* sentinel = q_head;
    Node<T>* new_head = q_head->next;

    while (new_head == nullptr) {
      // queue is empty
      // Wait until enqueuer wakes me up OR no more enqueues are coming
      wakeup_dq = true;
      queue_empty.wait_for(lck, std::chrono::milliseconds(1),
                           [this] {return no_more_enqueues.load() == true; });
      if (new_head != nullptr) {
        sentinel = q_head;
        new_head = q_head->next;
        wakeup_dq = false;
      } else {
        // queue is empty
        return false;
      }
    }
    *value = new_head->value;
    q_head = new_head;
    my_allocator_.freeNode(sentinel);
    return true;
  }

  void cleanup() { my_allocator_.cleanup(); }
};
