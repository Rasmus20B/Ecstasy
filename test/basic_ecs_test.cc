#include <gtest/gtest.h>
#include <print>
#include <fstream>
#include <filesystem>

import ecstasy;
import util;

using namespace ecstasy;
using namespace ecstasy::component;
using namespace ecstasy::systems;

struct FakeAssetManager {
  template<typename T>
  struct Asset {
    std::string path;
    std::filesystem::file_time_type ftime;
    T data;
  };
  std::unordered_map<std::string, Asset<Texture2D>> sprites;
  std::unordered_map<std::string, Asset<std::vector<u8>>> scripts;
};

inline FakeAssetManager fa;

TEST(scripting, adder) {

  EntityManager<20, CTransform2D, CVelocity> em; 
  auto test = em.create_entity();
  em.add_components<CTransform2D, CVelocity>(test,
      {
        .position = {100, 100},
        .scale = { 50, 50},
        .rotation = 0
      },
      {
        .velocity = {80, 80}
      });

  auto ents = em.get_associated_entities<CTransform2D>();
  /* When the program counter extends past the program size, 
   * the entity is pushed to the deads container */
  auto &handle = em.cm.template get<CTransform2D>(test);
  EXPECT_LT(handle.position.y, 1080);
  while(em.deads.empty()) {
    std::println("x: {}, y: {}", handle.position.x, handle.position.y);
    systems::move_transform(ents, em);
    systems::remove_out_of_bounds(ents, em);
  }
  EXPECT_GT(handle.position.y, 1080);
  /* Clean entity pool for next text */
  systems::remove_deads(em);
}
