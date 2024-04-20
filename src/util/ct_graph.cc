module;

#include <tuple>
#include <concepts>

export module util:ct_graph;

import :typelist;
import :meta;

namespace tl = util::typelist;
namespace mt = util::meta;

namespace util {
  export namespace CTGraph {

    template<typename From, typename To>
    struct Edge {
      using from = From;
      using to = To;
    };

    template<typename List>
    struct Node {
      List edges;
    };

    template<typename T>
    struct runBefore {};

    template<typename T>
    struct runAfter {};


    constexpr auto check = []<typename T>() -> bool {
      return mt::is_template_instance_v<T, runBefore>;
    };

    template<typename A, typename B>
    struct Connected {
      using a_befores = tl::filter_on_type<runBefore, A>;
      static constexpr bool value = true;
    };


    template<typename n1, typename n2>
    constexpr bool is_connected_v = Connected<n1, n2>::value;
  }
}
