#ifndef TMPLT_TSTD_META_PROJECTION_H
#define TMPLT_TSTD_META_PROJECTION_H

#include <concepts>
#include <type_traits>
#include <functional>
#include <utility>

namespace tmplt::tstd::ranges
{

class meta_projection_factory
{
    template<typename Function>
    class internal_projection
    {
        template<typename Self>
        using function_t = decltype((std::declval<Self>().function));

    public:
        template<std::convertible_to<Function> T>
        explicit constexpr internal_projection(T&& function) noexcept(std::is_nothrow_constructible_v<Function, T>) : function{std::forward<T>(function)}
        {}

        template<typename Self, std::invocable<function_t<Self>> Value>
        constexpr decltype(auto) operator()(this Self&& self, Value&& value) noexcept(std::is_nothrow_invocable_v<Value, function_t<Self>>)
        {
            return std::invoke(std::forward<Value>(value), std::forward_like<Self>(self.function));
        }

    private:
        [[no_unique_address]] Function function;
    };

    template<typename T>
    internal_projection(T&&) -> internal_projection<std::decay_t<T>>;

public:
    template<typename Function>
    [[nodiscard]] static constexpr auto operator()(Function&& function) noexcept(noexcept(internal_projection{std::declval<Function>()}))
    {
        return internal_projection{std::forward<Function>(function)};
    }
};

inline constexpr auto meta_projection = meta_projection_factory{};

}

#endif /* TMPLT_TSTD_META_PROJECTION_H */
