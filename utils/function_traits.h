#ifndef COMMON_LIBRARY_FUNCTION_TRAITS_H
#define COMMON_LIBRARY_FUNCTION_TRAITS_H

#include <functional>
#include <tuple>

namespace common_library {

template <typename T> struct function_traits;

// 普通函数
template <typename Ret, typename... Args> struct function_traits<Ret(Args...)>
{
    enum { arity = sizeof...(Args) };

    typedef Ret FunctionType(Args...);
    typedef Ret ReturnType;
    using STLFunctionType = std::function<Ret(Args...)>;
    typedef Ret (*Pointer)(Args...);

    template <size_t I> struct args
    {
        static_assert(
            I < arity,
            "index is out of range, index mush less than sizeof Args");
        using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
    };
};

// 函数指针
template <typename Ret, typename... Args>
struct function_traits<Ret (*)(Args...)> final : function_traits<Ret(Args...)>
{
};

// stl std::function
template <typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>> final
    : function_traits<Ret(Args...)>
{
};

// 成员函数
#define FUNCTION_TRAITS(...)                                                   \
    template <typename ReturnType, typename Class, typename... Args>           \
    struct function_traits<ReturnType (Class::*)(Args...) __VA_ARGS__> final   \
        : function_traits<ReturnType(Args...)>                                 \
    {                                                                          \
    };

FUNCTION_TRAITS()
FUNCTION_TRAITS(const)
FUNCTION_TRAITS(volatile)
FUNCTION_TRAITS(const volatile)

// 对象函数
template <typename Callable>
struct function_traits final : function_traits<decltype(&Callable::operator())>
{
};

}  // namespace common_library

#endif