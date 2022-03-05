#include <mutex>

#include "../common/allocator.h"

#define LFENCE asm volatile("lfence" : : : "memory")
#define SFENCE asm volatile("sfence" : : : "memory")

template <class P>
struct pointer_t {
  P* ptr;

  pointer_t() : ptr(nullptr) {}

  pointer_t(P* node, uint count) {
    uintptr_t cnt = (uintptr_t)count << 48;
    uintptr_t node_addr = (uintptr_t)node & 0x0000FFFFFFFFFFFF;
    ptr = (P*)(cnt | node_addr);
  }

  P* address() {
    // Get the address by getting the 48 least significant bits of ptr
    return (P*)((uintptr_t)ptr & 0x0000FFFFFFFFFFFF);
  }

  uint count() {
    // Get the count from the 16 most significant bits of ptr
    return (uintptr_t)ptr & 0xFFFF000000000000;
  }

  pointer_t<P>& operator=(P* value) {
    ptr = value;
    return *this;
  }

  bool operator==(const pointer_t& other) { return this->ptr == other.ptr; }
};

template <class T>
class Node {
 public:
  T value;
  pointer_t<Node<T>> next;

  Node() : value(0), next(nullptr) {}
};

template <class T>
class NonBlockingQueue {
  pointer_t<Node<T>> q_head;
  pointer_t<Node<T>> q_tail;
  CustomAllocator my_allocator_;

 public:
  NonBlockingQueue() : my_allocator_() {
    std::cout << "Using NonBlockingQueue\n";
  }

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
    // Use LFENCE and SFENCE as mentioned in pseudocode
    Node<T>* new_node = (Node<T>*)my_allocator_.newNode();
    new_node->value = value;
    new_node->next.ptr = nullptr;
    pointer_t<Node<T>> tail;
    pointer_t<Node<T>> next;
    SFENCE;
    while (true) {
      tail = q_tail;
      LFENCE;
      next = tail.address()->next;
      LFENCE;
      if (tail == q_tail) {
        if (next.address() == nullptr) {
          // Try to link node at the end of the linked list
          pointer_t<Node<T>> new_node_ptr(new_node, next.count() + 1);
          if (CAS(&tail.address()->next, next, new_node_ptr)) {
            break;  // Enqueue is done
          }
        } else {
          // Tail was not pointing to the last node
          // Try to swing Tail to the next node
          pointer_t<Node<T>> new_tail_ptr(next.ptr, tail.count() + 1);
          CAS(&q_tail, tail, new_tail_ptr);
        }
      }
    }
    SFENCE;
    // Enqueue is done. Try to swing Tail to the inserted node
    // Note: doesnt matter if it fails or not because another thread will come
    // by to fix it
    pointer_t<Node<T>> new_tail_ptr(new_node, tail.count() + 1);
    CAS(&q_tail, tail, new_tail_ptr);
  }

  bool dequeue(T* value) {
    // Use LFENCE and SFENCE as mentioned in pseudocode
    pointer_t<Node<T>> head;
    pointer_t<Node<T>> tail;
    pointer_t<Node<T>> next;
    while (true) {
      head = q_head;
      LFENCE;
      tail = q_tail;
      LFENCE;
      next = head.address()->next;
      LFENCE;
      if (head == q_head) {
        if (head.address() == tail.address()) {
          if (next.address() == nullptr) {
            // queue is empty couldn’t dequeue
            return false;
          }
          // Tail is falling behind. Try to advance it
          pointer_t<Node<T>> new_tail_ptr(next.ptr, tail.count() + 1);
          CAS(&q_tail, tail, new_tail_ptr);
        } else {
          // Read value otherwise another dequeue might free the next node
          *value = next.address()->value;

          // Try to swing Head to the next node
          pointer_t<Node<T>> new_head_ptr(next.ptr, head.count() + 1);
          if (CAS(&q_head, head, new_head_ptr)) {
            break;  // Dequeue is done. Exit loop
          }
        }
      }
    }
    my_allocator_.freeNode(head.address());
    return true;
  }

  void cleanup() { my_allocator_.cleanup(); }
};
