#ifndef COMMON_LIBRARY_FUNCTION_TRAITS_H
#define COMMON_LIBRARY_FUNCTION_TRAITS_H

#include <functional>
#include <tuple>

namespace common_library {

template <typename T> struct function_traits;

template <typename Ret, typename... Args> struct function_traits<Ret(Args...)> {
  enum { arity = sizeof...(Args) };

  typedef Ret function_type(Args...);
  typedef Ret return_type;
  using stl_function_type = std::function<Ret(Args...)>;
  typedef Ret (*pointer)(Args...);

  template <size_t I> struct args {
    static_assert(I < arity,
                  "index is out of range, index mush less than sizeof Args");
    using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
  };
};

} // namespace common_library

#endif