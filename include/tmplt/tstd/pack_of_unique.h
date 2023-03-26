#ifndef TMPLT_TSTD_PACK_OF_UNIQUE_H
#define TMPLT_TSTD_PACK_OF_UNIQUE_H

#include <type_traits>

namespace tmplt::tstd
{

namespace detail
{

template<typename ...>
struct are_all_unique : std::true_type
{};

template<typename T, typename ... R>
struct are_all_unique<T, R...> : std::bool_constant<std::conjunction_v<std::negation<std::is_same<T, R>>...> && are_all_unique<R...>::value>
{};

}

template<typename ... T>
concept pack_of_unique = detail::are_all_unique<T...>::value;

}

#endif /* TMPLT_TSTD_PACK_OF_UNIQUE_H */
