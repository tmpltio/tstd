#ifndef TMPLT_TSTD_LEAST_H
#define TMPLT_TSTD_LEAST_H

#include <concepts>
#include <limits>
#include <type_traits>

namespace tmplt::tstd
{

template<std::unsigned_integral auto I>
struct least
{
private:
    template<typename T, typename ... R>
    struct selector
    {
        using type = std::conditional_t<I <= std::numeric_limits<T>::max(), T, typename selector<R...>::type>;
    };

    template<typename T>
    struct selector<T>
    {
        using type = T;
    };

public:
    using type = typename selector<signed char, unsigned char, signed short, unsigned short, signed int, unsigned int, signed long, unsigned long, signed long long, unsigned long long>::type;
    using signed_type = typename selector<signed char, signed short, signed int, signed long, signed long long>::type;
    using unsigned_type = typename selector<unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long>::type;
};

template<std::unsigned_integral auto I>
using least_t = typename least<I>::type;

template<std::unsigned_integral auto I>
using least_signed_t = typename least<I>::signed_type;

template<std::unsigned_integral auto I>
using least_unsigned_t = typename least<I>::unsigned_type;

}

#endif /* TMPLT_TSTD_LEAST_H */
