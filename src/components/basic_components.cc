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

    struct CInput {};

    struct CAttraction {
      Entity attractor;
      f32 gravity;
      Vec2 cache;
    };
  }
}

