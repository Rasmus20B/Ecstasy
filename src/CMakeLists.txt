add_library(util)
target_sources(util
  PUBLIC
  FILE_SET
  CXX_MODULES FILES
  util/util.cc
  util/collision.cc
  util/ring_buffer.cc
  util/sparse_set.cc
  util/shapes.cc
  util/types.cc
  util/typelist.cc
  )

add_library(ecstasy)
target_sources(ecstasy
  PUBLIC
  FILE_SET
  CXX_MODULES FILES
  ecstasy.cc
  component.cc
  entity.cc
  components/components.cc
  components/basic_components.cc
  systems/basic_systems.cc
  )

target_link_libraries(ecstasy util)
