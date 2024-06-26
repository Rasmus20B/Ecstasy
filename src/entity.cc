module;

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <ranges>

export module ecstasy:entity;

import :component;

import util;

export namespace ecstasy {
  using namespace component;

  namespace tl = util::typelist;

  template<typename T, size_t N, typename... C>
  requires(valid_component<T, C...>)
  T& getComponent(ComponentManager<N, C...>& cm, Entity i) {
    return cm.template get<T>(i);
  }

  template<size_t N, typename ...C>
  struct EntityManager {

    EntityManager() {
    };

    using pool_t = util::basic_sparse_set<Entity>;
    using map_t = std::unordered_map<int, util::basic_sparse_set<Entity>>;
    using queue_t = util::RingBuffer<Entity, N>;

    Entity create_entity() {
      if(!reuse.empty()) {
        auto id = reuse.front();
        reuse.pop();
        pool.try_emplace(id);
        e_count++;
        return id;
      } else {
        pool.try_emplace(e_cur);
        e_count++;
        e_cur++;
        return e_cur - 1;
      }
    }

    void delete_all_entities() {
      for(auto &i: pool.dense) {
        delete_entity(i);
      }
    }

    void delete_entity(Entity e) {
      for(auto i : std::views::iota(0u, c_list.size)) {
       e_maps[i].remove(e);
      }
      pool.remove(e);
      e_count--;
    }

    template<typename ...T>
    void add_components(Entity e, T&&... components) {
      (add_component(e, std::forward<T&&>(components)), ...);
    }

    template<typename T>
    requires(valid_component<T, C...>)
    void add_component(Entity e, T&& c) {
      cm.template get<T>(e) = std::move(c);
      e_maps[tl::get_index_of_type<T, decltype(c_list)>].try_emplace(e);
    }

    template<typename T>
    requires(valid_component<T, C...>)
    constexpr std::vector<Entity> get_associated_entities() noexcept {
      auto c_id = tl::get_index_of_type<T, decltype(c_list)>;
      return std::vector<Entity>(e_maps[c_id].begin(), e_maps[c_id].end());
    }

    usize e_count = 1;
    usize e_cur = 1;
    pool_t pool;
    map_t e_maps;
    queue_t reuse;
    queue_t deads;
    ComponentManager<N, C...> cm;
    [[no_unique_address]] tl::Typelist<C...> c_list;
  };
}

