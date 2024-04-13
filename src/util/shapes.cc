module;

#include <cmath>
#include <concepts>

export module util:shapes;

import :types;

export inline constexpr f32 PI = 3.14;

template<typename T>
concept Shape = requires(T shape) {
  {auto(shape.x)} -> std::floating_point;
  {auto(shape.y)} -> std::floating_point;
};

export struct Vec2 {
  Vec2 operator+(const Vec2 rhs) const { return Vec2(x + rhs.x, y + rhs.y); }
  Vec2& operator+=(const Vec2 rhs) { x += rhs.x; y += rhs.y; return *this; }
  Vec2 operator-(const Vec2 rhs) const { return Vec2(x - rhs.x, y - rhs.y); }
  Vec2 rotate(const f32 angle) const {
    return { x * (f32)cos(angle) - (y * (f32)sin(angle)),
             x * (f32)sin(angle) + (y * (f32)cos(angle))};
  }
  f32 dot(const Vec2 rhs) { return (x * rhs.x) + (y * rhs.y); }

  f32 x;
  f32 y;
};

export struct Rect {

  Vec2 operatorVec2() {
    return Vec2 {x, y};
  }
  f32 x;
  f32 y;
  f32 width;
  f32 height;
};

export struct Circle {

  Circle() = default;
  Circle(Vec2 position) : x(position.x), y(position.y) {}
  Circle(Vec2 position, f32 r) : x(position.x), y(position.y), radius(r) {}

  template<Shape S>
  constexpr auto collides_with(S other) -> bool {
    if constexpr(std::same_as<S, Circle>) {
      return collision_circles(*this, other);
    }
  }

  constexpr operator Vec2() const {
    return Vec2 {x, y};
  }

  f32 x;
  f32 y;
  f32 radius;
};

constexpr auto collision_circles(const Circle c1, const Circle c2) -> bool {
  Vec2 dc = Vec2(c1) - Vec2(c2);
  f32 distance = sqrtf(dc.dot(dc));
  return distance <= (c1.radius + c2.radius);
}
