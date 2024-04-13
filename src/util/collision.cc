module; 

#include <vector>
#include <array>
#include <cmath>

export module util:collision;

import :shapes;

using line = std::pair<Vec2, Vec2>;

export std::vector<line> getAxis(const Rect& r, const float radian) {
  const Vec2 ox = { 1, 0 };
  const Vec2 oy = { 0, 1 };

  const Vec2 rx = ox.rotate(radian);
  const Vec2 ry = oy.rotate(radian);

  return {{
    { {r.x, r.y}, {rx.x, rx.y} },
    { {r.x, r.y}, {ry.x, ry.y} },
    }};
}

export std::array<Vec2, 4> getCorners(const Rect& r, const float radian) {
  const float cx = 0.5f * r.width;
  const float cy = 0.5f * r.height;

  const float ca = cos(radian);
  const float sa = sin(radian);

  const float r1x = -cx * ca - cy * sa;
  const float r1y = -cx * sa + cy * ca;
  const float r2x = cx * ca - cy * sa;
  const float r2y = cx * sa + cy * ca;

  return {
    {
      {r.x + r1x, r.y + r1y},
      {r.x + r2x, r.y + r2y},
      {r.x - r1x, r.y - r1y},
      {r.x - r2x, r.y - r2y},
    }
  };
}
//
// std::array<Vec2, 4> getCornersSIMD(const Rectangle& r, const float radian) {
//   float cx = 0.5f * r.width;
//   float cy = 0.5f * r.height;
//   float ca = cos(radian);
//   float sa = sin(radian);
// #ifdef __aarch64__
//   float32x4_t cxs = { -cx, -cx, cx, cx };
//   float32x4_t css = { ca, sa, ca, sa };
//   float32x4_t cys = { -cy, cy, -cy, cy };
//   float32x4_t csr = { sa, ca, sa, ca };
//
//   float32x4_t r1 = vmulq_f32(cxs, css);
//   float32x4_t r2 = vmulq_f32(cys, csr);
//   float32x4_t res = vaddq_f32(r1, r2);
//
//   float32x4_t coords = { r.x, r.y, r.x, r.y };
//
//   float32x4_t h1 = vaddq_f32(coords, res);
//   float32x4_t h2 = vsubq_f32(coords, res);
//   return {
//     {
//       {h1[0], h1[1]},
//       {h1[2], h1[3]},
//       {h2[0], h2[1]},
//       {h2[2], h2[3]},
//     }
//   };
// #elif __x86_64__
//   __m128 cxs = { -cx, -cx, cx, cx };
//   __m128 css = { ca, sa, ca, sa };
//   __m128 cys = { -cy, cy, -cy, cy };
//   __m128 csr = { sa, ca, sa, ca };
//
//   __m128 r1 = _mm_mul_ps(cxs,  css);
//   __m128 r2 = _mm_mul_ps(cys, csr);
//   __m128 res = _mm_add_ps(r1, r2);
//
//   __m128 coords = { r.x, r.y, r.x, r.y };
//
//   __m128 h1 = _mm_add_ps(coords,  res);
//   __m128 h2 = _mm_sub_ps(coords,  res);
//
//   return {
//     {
//       {h1[0], h1[1]},
//       {h1[2], h1[3]},
//       {h2[0], h2[1]},
//       {h2[2], h2[3]},
//     }
//   };
// #else
//   getCorners(r, radian);
// #endif
// }
//
export bool checkContains(std::array<Vec2, 4> cs1, std::array<Vec2, 4> cs2) {
  for(int i = 0; i < 4; ++i) {
    Vec2 edge = cs1[(i+1) % 4] - cs1[i];
    const Vec2 axis = {-edge.y, edge.x};

    float min1 = std::numeric_limits<float>().max();
    float max1 = std::numeric_limits<float>().min();
    float min2 = std::numeric_limits<float>().max();
    float max2 = std::numeric_limits<float>().min();
    for(int j = 0; j < 4; ++j) {

      float proj1 = cs1[j].dot(axis);
      float proj2 = cs2[j].dot(axis);

      min1 = std::min(min1, proj1);
      max1 = std::max(proj2, max1);
      min2 = std::min(min2, proj2);
      max2 = std::max(max2, proj2);
    }
    if(min1 >= max2 || min2 >= max1) {
      return false;
    }
  }
  return true;
}
//
// bool checkContainsSIMD(std::array<Vec2, 4> cs1, std::array<Vec2, 4> cs2) {
// #ifdef __x86_64__
//   for(int i = 0; i < 4; ++i) {
//     Vec2 edge = cs1[(i+1) % 4] - cs1[i];
//     Vector2 axis = {-edge.vec.y, edge.vec.x};
//     __m128 limits = {
//       std::numeric_limits<float>().max(),
//       std::numeric_limits<float>().min(),
//       std::numeric_limits<float>().max(),
//       std::numeric_limits<float>().min(),
//     };
//     for(int j = 0; j < 4; ++j) {
//       float proj1 = cs1[j].dot(Vec2{axis});
//       float proj2 = cs2[j].dot(Vec2{axis});
//       __m128 o1 = { proj1, limits[1], proj2, limits[3] };
//       __m128 o2 = { limits[0], proj1, limits[2], proj2 };
//       __m128 res1 = _mm_min_ps(o1, o2);
//       __m128 res2 = _mm_max_ps(o1, o2);
//       limits = __m128{ res1[0], res2[1], res1[2], res2[3] };
//     }
//     if(limits[0] >= limits[3] || limits[2] >= limits[1]) {
//       return false;
//     }
//   }
//   return true;
// }
// #elif __aarch64__
//   for(int i = 0; i < 4; ++i) {
//     Vec2 edge = cs1[(i+1) % 4] - cs1[i];
//     Vector2 axis = {-edge.vec.y, edge.vec.x};
//     float32x4_t limits = {
//       std::numeric_limits<float>().max(),
//       std::numeric_limits<float>().min(),
//       std::numeric_limits<float>().max(),
//       std::numeric_limits<float>().min(),
//     };
//     for(int j = 0; j < 4; ++j) {
//       float proj1 = cs1[j].dot(Vec2{axis});
//       float proj2 = cs2[j].dot(Vec2{axis});
//       float32x4_t o1 = { proj1, limits[1], proj2, limits[3] };
//       float32x4_t o2 = { limits[0], proj1, limits[2], proj2 };
//       float32x4_t res1 = vminq_f32(o1, o2);
//       float32x4_t res2 = vmaxq_f32(o1, o2);
//       limits = float32x4_t{ res1[0], res2[1], res1[2], res2[3] };
//     }
//     if(limits[0] >= limits[3] || limits[2] >= limits[1]) {
//       return false;
//     }
//   }
//   return true;
// }
// #endif
//
bool CheckCollisionRecsAngle(const Rect& r1, const float a1, const Rect& r2, const float a2) {
// #if defined(__x86_64__) || defined(__aarch64__)
//   auto cs1 = getCornersSIMD(r1, a1);
//   auto cs2 = getCornersSIMD(r2, a2);
//   if(!checkContainsSIMD(cs1, cs2)) return false;
//   if(!checkContainsSIMD(cs2, cs1)) return false;
// #else
  auto cs1 = getCorners(r1, a1);
  auto cs2 = getCorners(r2, a2);
  if(!checkContains(cs1, cs2)) return false;
  if(!checkContains(cs2, cs1)) return false;
// #endif
  return true;
}
