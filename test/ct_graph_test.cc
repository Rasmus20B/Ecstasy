#include <gtest/gtest.h>
#include <print>
#include <fstream>
#include <filesystem>
#include <concepts>
#include <type_traits>

import util;

namespace gr = util::CTGraph;

TEST(ct_graph, ordering) {

  struct ADep{};
  struct BDep{};
  struct CDep{};
  struct DDep{};

  struct A : public gr::Node<
             gr::runBefore<BDep>
             > {};

  struct B : public gr::Node<
             gr::runAfter<ADep>,
             gr::runBefore<CDep>
             > {};

  struct C : public gr::Node<
             gr::runAfter<BDep>
             > {};

  struct D : public gr::Node<> {};

  static_assert(util::CTGraph::is_connected_v<A, B> == true);
  static_assert(util::CTGraph::is_connected_v<B, C> == true);
  // static_assert(util::CTGraph::is_connected_v<C, D> == false);

}
