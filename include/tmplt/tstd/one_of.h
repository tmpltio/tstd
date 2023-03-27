#ifndef TMPLT_TSTD_ONE_OF_H
#define TMPLT_TSTD_ONE_OF_H

#include <concepts>

namespace tmplt::tstd
{

template<typename T, typename ... U>
concept one_of = (std::same_as<T, U> || ...);

}

#endif /* TMPLT_TSTD_ONE_OF_H */
