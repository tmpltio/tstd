#ifndef TMPLT_TSTD_TUPLE_LIKE_H
#define TMPLT_TSTD_TUPLE_LIKE_H

#include <cstddef>
#include <type_traits>
#include <utility>
#include <concepts>

namespace tmplt::tstd
{

template<typename T, std::size_t I>
struct qualify_as_get
{
private:
    template<typename U>
    using const_t = std::conditional_t<std::is_const_v<std::remove_reference_t<T>>, std::add_const_t<U>, U>;

    template<typename U>
    using reference_t = std::conditional_t<std::is_lvalue_reference_v<T>, std::add_lvalue_reference_t<U>, std::add_rvalue_reference_t<U>>;

public:
    using type = reference_t<const_t<std::tuple_element_t<I, std::remove_cvref_t<T>>>>;
};

template<typename T, std::size_t I>
using qualify_as_get_t = typename qualify_as_get<T, I>::type;

template<typename T, std::size_t I>
concept member_getable = requires(T&& t)
{
    { std::forward<T>(t).template get<I>() } -> std::same_as<qualify_as_get_t<T, I>>;
};

template<typename T, std::size_t I>
concept adl_getable = requires(T&& t)
{
    { get<I>(std::forward<T>(t)) } -> std::same_as<qualify_as_get_t<T, I>>;
};

template<typename T, std::size_t I>
concept getable = requires
{
    typename std::tuple_element<I, std::remove_cvref_t<T>>::type;
    requires member_getable<T, I> || adl_getable<T, I>;
};

template<typename T>
concept tuple_like = requires
{
    { std::tuple_size<std::remove_cvref_t<T>>::value } -> std::convertible_to<std::size_t>;
    []<std::size_t ... I>(std::index_sequence<I...>) requires (getable<T, I> && ...)
    {}(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<T>>>{});
};

}

#endif /* TMPLT_TSTD_TUPLE_LIKE_H */
