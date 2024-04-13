module;

#include <vector>
#include <variant>
#include <functional>

export module ecstasy:component;

export import :components;

import util;

export namespace ecstasy {
  namespace component {
    template<typename T>
    concept Component = requires {
      requires get_component_id<T>() != ComponentID::Size;
    };

    template<size_t N, Component... C>
    struct ComponentManager {
      ComponentManager() {
        std::apply([](auto&... c) {
            (c.resize(N), ...);
            }, comps);
      }

      template<Component V>
      inline V& get(const size_t idx) {
        return std::get<std::vector<V>>(comps)[idx]; 
      }

      template<Component V>
      inline std::vector<V>& get() {
        return std::get<std::vector<V>>(comps); 
      }
      std::tuple<std::vector<C>...> comps;
    };

    inline ComponentManager<100000,  CTransform2D,
                                     CVelocity,
                                     CScript,
                                     CHealth,
                                     CSprite,
                                     CInput,
                                     CAttraction,
                                     CCollider,
                                     CBulletManager,
                                     CChildren,
                                     CParent> component_manager{};
  }
}
