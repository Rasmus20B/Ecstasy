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

  template<typename T, size_t N, typename... C>
  requires(valid_component<T, C...>)
  T& getComponent(ComponentManager<N, C...>& cm, Entity i) {
    return cm.template get<T>(i);
  }


  template<size_t N, typename ...C>
  struct EntityManager {

    EntityManager() {
    };

    using pool_type = basic_sparse_set<Entity>;
    using map_type = std::unordered_map<int, basic_sparse_set<Entity>>;

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
      e_maps[get_index_of_type<T, decltype(c_list)>].try_emplace(e);
    }

    template<typename T>
    requires(valid_component<T, C...>)
    constexpr std::vector<Entity> get_associated_entities() noexcept {
      auto c_id = get_index_of_type<T, decltype(c_list)>;
      return std::vector<Entity>(e_maps[c_id].begin(), e_maps[c_id].end());
    }

    pool_type pool;
    usize e_count = 1;
    usize e_cur = 1;
    map_type e_maps{};
    ComponentManager<N, C...> cm;
    RingBuffer<Entity, N> reuse;
    RingBuffer<Entity, N> deads;
    [[no_unique_address]] Typelist<C...> c_list;
  };
}

