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
        em.cm.template get<CTransform2D>(i).position = em.cm.template get<CTransform2D>(i).position + em.cm.template get<CVelocity>(i).velocity;
      }
    }

    template<size_t N, typename ...C>
    void remove_out_of_bounds(const std::span<Entity> es, EntityManager<N, C...>& em) {
      for(auto i: es) {
        auto pos = em.cm.template get<CTransform2D>(i).position;
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
        const auto att = em.cm.template get<CAttraction>(i).attractor;
        auto entity_pos = em.cm.template get<CTransform2D>(i).position;
        auto attractor_pos = em.cm.template get<CTransform2D>(att).position;

        if(attractor_pos.x == em.cm.template get<CAttraction>(i).cache.x &&
            attractor_pos.y == em.cm.template get<CAttraction>(i).cache.y) {
          continue;
        }
        em.cm.template get<CAttraction>(i).cache = attractor_pos;
        auto dvec = attractor_pos - entity_pos;
        auto val = (dvec.x * dvec.x) + (dvec.y * dvec.y);
        float dist = sqrtf(val);
        float cur_power = em.cm.template get<CAttraction>(i).gravity / dist;
        em.cm.template get<CVelocity>(i).velocity.x = (dvec.x/dist) * cur_power;
        em.cm.template get<CVelocity>(i).velocity.y = (dvec.y/dist) * cur_power;
      }
    }

    template<size_t N, typename ...C>
    void check_collisions_with_single_entity(const std::span<Entity> es, const Entity e, EntityManager<N, C...>& em) {
      const auto& te = em.cm.template get<CTransform2D>(e);
      Circle ce(te.position, te.scale.x * 0.5f);
      for(auto i: es) {
        const auto& ti = em.cm.template get<CTransform2D>(i);
        Circle ci(ti.position, ti.scale.x * 0.5f);
        if(ci.collides_with(ce)) {
          em.cm.template get<CCollider>(i).callback(i, e);
        }
      }
    }

    template<size_t N, typename ...C>
    void remove_orphans(Entity e, EntityManager<N, C...>& em) {
      for(auto &c: em.cm.template get<CChildren>(e).children) {
        em.deads.push(c);
      }
    }

    template<size_t N, typename ...C>
    void remove_from_family(Entity e, EntityManager<N, C...>& em) {
      auto &parent = em.cm.template get<CParent>(e).parent;
      auto &siblings = em.cm.template get<CChildren>(parent).children;
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
