module;

#include <vector>
#include <variant>
#include <functional>

export module ecstasy:component;

export import :components;

import util;

export namespace ecstasy {
  namespace component {

    template<typename D, typename ...C>
    concept valid_component = requires {
      requires (count_frequency_of_type<D, Typelist<C...>> != 0); 
    };

    template<size_t N, typename... C>
    struct ComponentManager {
      ComponentManager() {
        std::apply([](auto&... c) {
            (c.resize(N), ...);
            }, comps);
      }

      template<typename V>
      inline V& get(const size_t idx) {
        return std::get<std::vector<V>>(comps)[idx]; 
      }

      template<typename V>
      inline std::vector<V>& get() {
        return std::get<std::vector<V>>(comps); 
      }
      std::tuple<std::vector<C>...> comps;
    };
  }
}
