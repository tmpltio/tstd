#ifndef TMPLT_TSTD_SPECIALIZATION_OF_H
#define TMPLT_TSTD_SPECIALIZATION_OF_H

#include <type_traits>

namespace tmplt::tstd
{

namespace detail
{

template<typename T, template<typename ...> typename U>
struct is_specialization_of : std::false_type
{};

template<typename ... T, template<typename ...> typename U>
struct is_specialization_of<U<T...>, U> : std::true_type
{};

}

template<typename T, template<typename ...> typename U>
concept specialization_of = detail::is_specialization_of<std::remove_cvref_t<T>, U>::value;

}

#endif /* TMPLT_TSTD_SPECIALIZATION_OF_H */
