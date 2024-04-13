module;

#include <concepts>
#include <functional>

export module ecstasy.components.basic;

import util;

export namespace ecstasy {
  namespace component {
    struct CTransform2D {
      Vec2 position;
      Vec2 scale;
      f32 rotation;
    };

    struct CVelocity {
      Vec2 velocity;
    };

    struct CCollider {
      std::function<void(Entity self, Entity collided)> callback;
    };

    struct CHealth {
      i32 health;
    };

    struct CParent {
      Entity parent;
    };

    struct CChildren {
      std::vector<Entity> children;
    };

    struct Texture2D {
      uint32_t id;
      uint32_t width;
      uint32_t height;
      uint32_t mipmaps;
      uint32_t format;
    };
    struct CSprite {
      Texture2D* sprite;
    };

    struct CInput {};

    struct CAttraction {
      Entity attractor;
      f32 gravity;
      Vec2 cache;
    };
  }
}

