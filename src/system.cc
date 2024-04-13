module;

#include <algorithm>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <cassert>
#include <span>

#include <print>
#include <arm_neon.h>
#include <thread>


export module ecstasy:system;

import :component;
import :entity;

import util;

export namespace ecstasy {
  using namespace component;
  namespace systems {
    using namespace ecstasy::component;

    template<size_t N, typename ...C>
    void move_transform(const std::span<Entity> es, EntityManager<N, C...>& em) {
      for(auto i : es) {
        auto& p = getComponent<CTransform2D>(em.cm, i).position;
        p += getComponent<CVelocity>(em.cm, i).velocity;
      }
    }

    template<size_t N, typename ...C>
    void remove_out_of_bounds(const std::span<Entity> es, EntityManager<N, C...>& em) {
      for(auto i: es) {
        auto pos = getComponent<CTransform2D>(em.cm, i).position;
        if(pos.x < 0 ||
           pos.y < 0 ||
           pos.x > 1920 ||
           pos.y > 1080) {
          em.deads.push(i);
        }
      }
    }

    template<size_t N, typename ...C>
    void orient_to_attractor(const std::span<Entity> es, EntityManager<N, C...>& em) {
      for(auto i: es) {
        const auto att = getComponent<CAttraction>(em.cm, i).attractor;
        auto entity_pos = getComponent<CTransform2D>(em.cm, i).position;
        auto attractor_pos = getComponent<CTransform2D>(em.cm, att).position;

        if(attractor_pos.x == getComponent<CAttraction>(em.cm, i).cache.x &&
            attractor_pos.y == getComponent<CAttraction>(em.cm, i).cache.y) {
          continue;
        }
        getComponent<CAttraction>(em.cm, i).cache = attractor_pos;
        auto dvec = attractor_pos - entity_pos;
        auto val = (dvec.x * dvec.x) + (dvec.y * dvec.y);
        float dist = sqrtf(val);
        float cur_power = getComponent<CAttraction>(em.cm, i).gravity / dist;
        getComponent<CVelocity>(em.cm, i).velocity.x = (dvec.x/dist) * cur_power;
        getComponent<CVelocity>(em.cm, i).velocity.y = (dvec.y/dist) * cur_power;
      }
    }

    template<size_t N, typename ...C>
    void check_collisions_with_single_entity(const std::span<Entity> es, const Entity e, EntityManager<N, C...>& em) {
      const auto& te = getComponent<CTransform2D>(em.cm, e);
      Circle ce(te.position, te.scale.x * 0.5f);
      for(auto i: es) {
        const auto& ti = getComponent<CTransform2D>(em.cm, i);
        Circle ci(ti.position, ti.scale.x * 0.5f);
        if(ci.collides_with(ce)) {
          getComponent<CCollider>(em.cm, i).callback(i, e);
        }
      }
    }

    template<size_t N, typename ...C>
    void remove_orphans(Entity e, EntityManager<N, C...>& em) {
      for(auto &c: getComponent<CChildren>(em.cm, e).children) {
        em.deads.push(c);
      }
    }

    template<size_t N, typename ...C>
    void remove_from_family(Entity e, EntityManager<N, C...>& em) {
      auto &parent = getComponent<CParent>(em.cm, e).parent;
      auto &siblings = getComponent<CChildren>(em.cm, parent).children;
      siblings.erase(std::remove(siblings.begin(), siblings.end(), e), siblings.end());
    }

    template<size_t N, typename ...C>
    void remove_deads(EntityManager<N, C...>& em) {
      std::vector<Entity> par_ents;
      std::vector<Entity> ch_ents;
      if constexpr(count_frequency_of_type<CParent, Typelist<C...>>) {
        par_ents = em.template get_associated_entities<CParent>();
      }
      if constexpr(count_frequency_of_type<CChildren, Typelist<C...>>) {
        ch_ents = em.template get_associated_entities<CChildren>();
      }
      while(!em.deads.empty()) {
        auto e = em.deads.front();
        if constexpr(count_frequency_of_type<CChildren, Typelist<C...>>) {
          if(std::find(ch_ents.begin(), ch_ents.end(), e) != ch_ents.end()) {
            remove_orphans(e, em);
          }
        }
        if constexpr(count_frequency_of_type<CParent, Typelist<C...>>) {
          if(std::find(par_ents.begin(), par_ents.end(), e) != par_ents.end()) {
            remove_from_family(e, em);
          }
        }
        em.deads.pop();
        em.delete_entity(e);
        em.reuse.push(e);
      }
    }
  }
}
