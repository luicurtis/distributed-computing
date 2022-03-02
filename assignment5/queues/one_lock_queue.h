#include <mutex>

#include "../common/allocator.h"

template <class T>
class Node {
 public:
  long value;
  Node<T>* next;
};

template <class T>
class OneLockQueue {
  Node<T>* q_head;
  Node<T>* q_tail;
  std::mutex mtx;
  CustomAllocator my_allocator_;

 public:
  OneLockQueue() : my_allocator_() { std::cout << "Using OneLockQueue\n"; }

  void initQueue(long t_my_allocator_size) {
    std::cout << "Using Allocator\n";
    my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
    // Initialize the queue head or tail here
    Node<T>* new_node = (Node<T>*)my_allocator_.newNode();
    q_head = new_node;
    q_tail = new_node;
  }

  void enqueue(T value) {
    mtx.lock();
    Node<T>* new_node = (Node<T>*)my_allocator_.newNode();
    new_node->value = value;
    new_node->next = nullptr;
    q_tail->next = new_node;
    q_tail = new_node;
    mtx.unlock();
  }

  bool dequeue(T* value) {
    mtx.lock();
    Node<T>* sentinel = q_head;
    Node<T>* new_head = q_head->next;
    if (new_head == nullptr) {
      // queue is empty
      mtx.unlock();
      return false;
    }
    *value = new_head->value;
    q_head = new_head;
    my_allocator_.freeNode(sentinel);
    mtx.unlock();
    return true;
  }

  void cleanup() { my_allocator_.cleanup(); }
};