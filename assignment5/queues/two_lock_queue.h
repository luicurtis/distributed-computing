#include <mutex>

#include "../common/allocator.h"

template <class T>
class Node {
 public:
  long value;
  Node<T>* next;

  Node() : value(0), next(nullptr) {}
};

template <class T>
class TwoLockQueue {
  Node<T>* q_head;
  Node<T>* q_tail;
  std::mutex enq_mtx;
  std::mutex deq_mtx;
  CustomAllocator my_allocator_;

 public:
  TwoLockQueue() : my_allocator_() { std::cout << "Using TwoLockQueue\n"; }

  void initQueue(long t_my_allocator_size) {
    std::cout << "Using Allocator\n";
    my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
    // Initialize the queue head or tail here
    Node<T>* new_node = (Node<T>*)my_allocator_.newNode();
    new_node->next = nullptr;
    q_head = new_node;
    q_tail = new_node;
  }

  void enqueue(T value) {
    Node<T>* new_node = (Node<T>*)my_allocator_.newNode();
    new_node->value = value;
    new_node->next = nullptr;

    enq_mtx.lock();
    q_tail->next = new_node;
    q_tail = new_node;
    enq_mtx.unlock();
  }

  bool dequeue(T* value) {
    deq_mtx.lock();
    Node<T>* sentinel = q_head;
    Node<T>* new_head = q_head->next;
    if (new_head == nullptr) {
      // queue is empty
      deq_mtx.unlock();
      return false;
    }
    *value = new_head->value;
    q_head = new_head;
    deq_mtx.unlock();
    my_allocator_.freeNode(sentinel);
    return true;
  }

  void cleanup() { my_allocator_.cleanup(); }
};