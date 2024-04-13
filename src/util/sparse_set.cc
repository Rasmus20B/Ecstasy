module;

#include <expected>
#include <cstddef>
#include <vector>
#include <cstdint>

export module util:sparse_set;

template<typename T>
concept is_valid_ent_type = (std::is_same_v<T, std::uint32_t> || std::is_same_v<T, std::uint64_t>);

export template<typename Entity = std::uint32_t, typename Allocator = std::allocator<Entity>>
requires(is_valid_ent_type<Entity>)
struct basic_sparse_set {
  using sparse_container_t = std::vector<Entity, Allocator>;
  using dense_container_t = std::vector<Entity, Allocator>;


  struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = Entity;
    using pointer           = Entity*;
    using reference         = Entity&;

    Iterator(pointer ptr) : m_ptr(ptr) {}

    reference operator*() const { return *m_ptr; }
    pointer operator->() const { return *m_ptr; }

    Iterator& operator++() { m_ptr++; return *this; }

    Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

    friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; }
    friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_ptr != b.m_ptr; }

    pointer m_ptr;
  };

  Iterator begin()  { return Iterator(&dense[0]); }
  Iterator end()    { return Iterator(&dense[head]);  }

  [[nodiscard]] inline size_t size() {
    return head;
  }
  
  [[nodiscard]] inline size_t empty() {
    return head == 0;
  }

  [[nodiscard]] inline bool contains(const Entity e) {
    auto idx = sparse[e];
    return idx < head && dense[idx] == e;
  }

  void remove(const Entity e) {
    if(sparse.size() <= e) {
      sparse.resize(e + 1);
    }
    if(!contains(e)) {
      return;
    }
    head--;
    auto idx = sparse[e];
    dense[idx] = dense[head];
    sparse[dense[head]] = idx;
  }

  void try_emplace(const Entity e) {
    if(sparse.size() <= e) {
      sparse.resize(e + 1);
    }
    if(contains(e)) {
      return;
    }
    sparse[e] = head;
    if(head + 1 >= dense.size()) {
      dense.resize(dense.size() + head + 1);
    }
    dense[head] = e;
    head++;
  }

  sparse_container_t sparse;
  dense_container_t dense;
  Entity head = 0;
};
