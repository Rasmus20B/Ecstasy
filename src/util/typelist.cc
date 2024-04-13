module;

#include <tuple>
#include <type_traits>
#include <string_view>
#include <source_location>
#include <limits>

#include <iostream>

export module util:typelist;

export {


template<typename T>
void print() {
  std::cout << std::source_location::current().function_name() << "\n";
}

template<typename T>
struct Identity {
  using value = T;
};

template<typename... Elems>
struct Typelist {
  static inline constexpr size_t size = sizeof...(Elems);
};

template<typename T1, typename ...T2>
consteval auto append(T1, T2...) {
  return Typelist<T1, T2...>{};
}

template<typename ...T2>
consteval auto append(Typelist<>, T2...) {
  return Typelist<T2...>{};
}

template<typename List>
struct FrontT;

template<typename Head, typename... Tail>
struct FrontT<Typelist<Head, Tail...>> {
  using value = Head;
};

template<>
struct FrontT <Typelist<>> {
  using value = std::false_type; 
};

template<typename List>
using Front = typename FrontT<List>::value;

template<typename List>
struct PopFrontT;

template<>
struct PopFrontT <Typelist<>> {
  using value = Typelist<>;
};

template<typename Head, typename ...Tail>
struct PopFrontT <Typelist<Head, Tail...>>{
  using value = Typelist<Tail...>;
};

template<typename List>
using PopFront = typename PopFrontT<List>::value;

template<typename A, typename B, typename List>
struct count_uniqueT;

template<typename List, typename Element>
struct PushFrontT;

template<typename... List, typename Element>
struct PushFrontT <Typelist<List...>, Element> {
  using value = Typelist<Element, List...>;
};

template<typename List, typename Element>
using PushFront = typename PushFrontT<List, Element>::value;

template<typename List, typename Element>
struct PushBackT;

template<typename... List, typename Element>
struct PushBackT <Typelist<List...>, Element> {
  using value = Typelist<List..., Element>;
};
template<typename List, typename Element>
using PushBack = typename PushBackT<List, Element>::value;

template<typename A, typename B, typename Tail>
struct count_uniqueT {
  static inline constexpr size_t value = (!std::is_same_v<A, B>) + count_uniqueT<B, Front<PopFront<Tail>>, PopFront<Tail>>::value;
};

template<typename A, typename B>
struct count_uniqueT<A, B, Typelist<>>  {
  static inline constexpr size_t value = (!std::is_same_v<A, B>);
};

template<typename T>
[[nodiscard]] consteval std::string_view templateID() {
  return std::source_location::current().function_name();
}

template<typename T, typename U>
struct type_cmp {
  static inline constexpr bool value = (templateID<T>()) < (templateID<U>());
};

template<typename List, typename Element,
  template<typename T, typename U> typename Compare, bool = (std::is_same_v<List, Typelist<>>)>
  struct InsertSortedT;

// Recursive Case
template<typename List, typename Element,
  template<typename T, typename U> class Compare>
  struct InsertSortedT<List, Element, Compare, false> {
    using NewTail = typename std::conditional<Compare<Element, Front<List>>::value,
                                Identity<List>,
                                InsertSortedT<PopFront<List>, Element, Compare>>::type;

    using NewHead = typename std::conditional<Compare<Element, Front<List>>::value,
                                Element,
                                Front<List>>::type;

    using value = PushFront<typename NewTail::value, NewHead>;
  };

// Base Case
template<typename List, typename Element,
  template<typename T, typename U> class Compare>
  struct InsertSortedT<List, Element, Compare, true> 
: public PushFrontT<List, Element>
  {};

template<typename List, typename Element,
  template<typename T, typename U> class Compare>
  using InsertSorted = typename InsertSortedT<List, Element, Compare>::value;

template<typename List,
  template<typename T, typename U> typename Compare, 
  bool = std::is_same_v<List, Typelist<>> >
  struct InsertionSortT;

template<typename List,
  template<typename T, typename U> class Compare>
  using InsertionSort = typename InsertionSortT<List, Compare>::value;

// Recursive Case
template<typename List,
  template<typename T, typename U> class Compare>
  struct InsertionSortT<List, Compare, false>
  : public InsertSortedT<InsertionSort<PopFront<List>, Compare>,
                         Front<List>, Compare> {};

// Base Case
template<typename List,
  template<typename T, typename U> class Compare>
  struct InsertionSortT<List, Compare, true> {
  using value = List;
  };

template<typename List, size_t i, size_t cur, bool cmp = i == cur>
requires(i >= cur)
struct atT;

template<typename List, size_t i, size_t cur>
requires(i >= cur)
struct atT<List, i, cur, false> {
  using type = typename atT<PopFront<List>, i, cur+1>::type;
};

template<typename List, size_t i, size_t cur>
struct atT<List, i, cur, true> {
  using type = Front<List>;
};

template<size_t i, size_t cur>
struct atT<Typelist<>, i, cur> {
  using type = std::false_type;
};

template<typename List, int i>
requires(List::size > i)
using at = atT<List, i, 0>::type;

template<template<typename> typename Pred, typename List>
[[nodiscard]] consteval int count_frequencyT(List) noexcept {
  return Pred<Front<List>>{} + count_frequencyT<Pred>(PopFront<List>{});
}

template<template<typename> typename Pred>
[[nodiscard]] consteval int count_frequencyT(Typelist<>) {
  return 0;
}

template<template<typename> typename Pred, typename Result>
[[nodiscard]] consteval auto filterT(Result result, Typelist<>) noexcept {
  return result;
}

template<template<typename> typename Pred, typename Result, typename List>
[[nodiscard]] consteval auto filterT(Result result, List) noexcept {
  if constexpr(Pred<Front<List>>{}) {
    return filterT<Pred>(append(result, Front<List>{}), PopFront<List>{});
  } else {
    return filterT<Pred>(result, PopFront<List>{});
  }
}

template<template<typename T> typename Pred, typename List>
using filter = std::decay_t<decltype(filterT<Pred>(Typelist<>{}, List{}))>;

template<typename T, typename Result>
[[nodiscard]] consteval auto filter_on_typeT(Result result, Typelist<>) noexcept {
  return result;
}

template<typename T, typename Result, typename List>
[[nodiscard]] consteval auto filter_on_typeT(Result result, List) noexcept {
  if constexpr(std::is_same_v<T, Front<List>>) {
    return filter_on_typeT<T>(append(result, Front<List>{}), PopFront<List>{});
  } else {
    return filter_on_typeT<T>(result, PopFront<List>{});
  }
}
template<typename T, typename List>
using filter_on_type = std::decay_t<decltype(filter_on_typeT<T>(Typelist<>{}, List{}))>;

template<typename List, typename Result>
struct remove_duplicatesT {
  using value = std::conditional<std::is_same_v<Front<List>, Front<PopFront<List>>>,
        typename remove_duplicatesT<PopFront<List>, Result>::value,
        typename remove_duplicatesT<PopFront<List>, PushBack<Result, Front<List>>>::value
          >::type;
};

template<typename Result>
struct remove_duplicatesT<Typelist<>, Result> {
  using value = Result;
};

template<typename T, typename List>
consteval int count_frequency_of_typeT(List) {
  return std::is_same_v<T, Front<List>> + count_frequency_of_typeT<T>(PopFront<List>{});
}

template<typename T>
consteval int count_frequency_of_typeT(Typelist<>) {
  return 0;
}

template<typename List>
using remove_duplicates = remove_duplicatesT<InsertionSort<List, type_cmp>, Typelist<>>::value;

template<auto start, auto end, auto inc, class F>
constexpr void constexpr_for_n(F&& f) {
  if constexpr(start < end) {
    f(std::integral_constant<decltype(start), start>());
    constexpr_for_n<start + inc, end, inc>(f);
  }
}

template<typename T, size_t count>
consteval size_t get_index_of_typeT(Typelist<>) {
  return std::numeric_limits<size_t>::max();
}

template<typename T, size_t count, typename List>
consteval size_t get_index_of_TypeT(List) {
  if constexpr(List::size == 0) {
    return std::numeric_limits<size_t>::max();
  }
  if constexpr(std::is_same_v<T, Front<List>>) {
    return count;
  } else {
    return get_index_of_TypeT<T, count + 1>(PopFront<List>{});
  }
}

template<typename T, typename List>
inline constexpr size_t get_index_of_type = get_index_of_TypeT<T, 0>(List{});

template<typename F, typename... Args>
constexpr void constexpr_for(F&& f, Args&&... args) {
  (f(std::forward<Args>(args)), ...);
}

template<template<typename T> typename Pred, typename List>
inline constexpr int count_frequency = count_frequencyT<Pred>(List{});

template<typename T, typename List>
inline constexpr int count_frequency_of_type = count_frequency_of_typeT<T>(List{});

template<typename List> 
inline constexpr size_t count_unique = count_uniqueT<Front<List>, Front<PopFront<List>>, PopFront<List>>::value;
}
