module;

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <iostream>
#include <queue>
#include <ranges>

export module ecstasy:entity;

import :component;

import util;

export namespace ecstasy {
  using namespace component;


  template<size_t N, typename ...C>
  struct EntityManager {

    EntityManager() : cm(ComponentManager<N, C...>()) {}

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
      for(auto i : std::views::iota(0, std::to_underlying(component::ComponentID::Size))) {
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
      e_maps[std::to_underlying(component::get_component_id<T>())].try_emplace(e);
    }

    template<typename T>
    requires(valid_component<T, C...>)
    constexpr std::vector<Entity> get_associated_entities() noexcept {
      auto c_id = std::to_underlying(component::get_component_id<T>());
      return std::vector<Entity>(e_maps[c_id].begin(), e_maps[c_id].end());
    }

    void run_systems() {
      
    }


    pool_type pool;
    usize e_count = 1;
    usize e_cur = 1;
    map_type e_maps{};
    ComponentManager<N, C...> cm;
    RingBuffer<Entity, 10000> reuse;
    RingBuffer<Entity, 10000> deads;
    [[no_unique_address]] Typelist<C...> components;
  };
}

