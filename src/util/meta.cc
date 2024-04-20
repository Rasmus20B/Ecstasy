module;

#include <type_traits>

export module util:meta;

namespace util {
  export namespace meta {
    template<typename, template<typename...> typename>
    struct is_template_instance : public std::false_type{};

    template<typename... Args, template<typename...> typename T>
    struct is_template_instance<T<Args...>, T> : public std::true_type {};

    template<typename T, template<typename...> typename V>
    inline constexpr bool is_template_instance_v = is_template_instance<T, V>::value; 

    template<typename T>
    struct ti {};

    template<typename T>
    struct tj {};

    using test1 = ti<int>;
    using test2 = ti<float>;

    static_assert(is_template_instance_v<test1, ti> == true);
    static_assert(is_template_instance_v<test2, ti> == true);
    static_assert(is_template_instance_v<test2, tj> == false);

  }
}
