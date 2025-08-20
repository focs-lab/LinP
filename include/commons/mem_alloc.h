#pragma once

namespace linp {

// Represents static sized memory_allocator for fast alloc and free
template <typename T>
  requires(sizeof(T) >= sizeof(void*))  // safe reinterpret_cast to free_node*
struct memory_allocator {
  struct free_node {
    free_node* next;
  };

  explicit memory_allocator(size_t size)
      : data{(T*)std::malloc(sizeof(T) * size)}, used{0}, free_list{nullptr} {}

  ~memory_allocator() { std::free(data); }

  T* alloc() {
    if (free_list) {  // reuse from free_list
      T* slot = reinterpret_cast<T*>(free_list);
      free_list = free_list->next;
      return slot;
    }
    return &data[used++];  // allocate from block
  }

  void free(T* slot) {
    slot->~T();
    auto* node = reinterpret_cast<free_node*>(slot);
    node->next = free_list;
    free_list = node;
  }

 private:
  T* data;
  size_t used;
  free_node* free_list;
};

};  // namespace linp