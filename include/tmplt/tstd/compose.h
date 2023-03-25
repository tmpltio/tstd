#ifndef TMPLT_TSTD_COMPOSE_H
#define TMPLT_TSTD_COMPOSE_H

#include <concepts>
#include <type_traits>
#include <utility>
#include <functional>

namespace tmplt::tstd
{

class compose_factory
{
    template<typename Outer, typename Inner>
    class compose_two
    {
        template<typename Self>
        using outer_t = decltype((std::declval<Self>().outer));
        template<typename Self>
        using inner_t = decltype((std::declval<Self>().inner));

        template<typename Self, typename ... Args>
        [[nodiscard]] static consteval bool is_invoke_noexcept() noexcept
        {
            constexpr auto is_inner_invoke_noexcept = std::is_nothrow_invocable_v<inner_t<Self>, Args...>;
            constexpr auto is_outer_invoke_noexcept = std::is_nothrow_invocable_v<outer_t<Self>, std::invoke_result_t<inner_t<Self>, Args...>>;

            return is_inner_invoke_noexcept && is_outer_invoke_noexcept;
        }

    public:
        template<std::convertible_to<Outer> T, std::convertible_to<Inner> U>
        explicit constexpr compose_two(T&& outer, U&& inner) noexcept(std::is_nothrow_constructible_v<Outer, T> && std::is_nothrow_constructible_v<Inner, U>) : outer{std::forward<T>(outer)}, inner{std::forward<U>(inner)}
        {}

        template<typename Self, typename ... Args>
        requires std::invocable<inner_t<Self>, Args...> && std::invocable<outer_t<Self>, std::invoke_result_t<inner_t<Self>, Args...>>
        constexpr decltype(auto) operator()(this Self&& self, Args&&... args) noexcept(is_invoke_noexcept<Self, Args...>())
        {
            return std::invoke(std::forward_like<Self>(self.outer), std::invoke(std::forward_like<Self>(self.inner), std::forward<Args>(args)...));
        }

    private:
        [[no_unique_address]] Outer outer;
        [[no_unique_address]] Inner inner;
    };

    template<typename T, typename U>
    compose_two(T&&, U&&) -> compose_two<std::decay_t<T>, std::decay_t<U>>;

    template<typename Outer, typename ... Rest>
    [[nodiscard]] static consteval bool is_call_noexcept() noexcept
    {
        constexpr auto is_nested_noexcept = std::is_nothrow_invocable_v<compose_factory const&, Rest...>;
        constexpr auto is_compose_noexcept = noexcept(compose_two{std::declval<Outer>(), std::declval<std::invoke_result_t<compose_factory const&, Rest...>>()});

        return is_nested_noexcept && is_compose_noexcept;
    }

public:
    template<typename Function>
    [[nodiscard]] static constexpr auto&& operator()(Function&& function) noexcept
    {
        return std::forward<Function>(function);
    }

    template<typename Outer, typename ... Rest>
    [[nodiscard]] static constexpr auto operator()(Outer&& outer, Rest&&... rest) noexcept(is_call_noexcept<Outer, Rest...>())
    {
        return compose_two{std::forward<Outer>(outer), std::invoke(compose_factory{}, std::forward<Rest>(rest)...)};
    }
};

inline constexpr auto compose = compose_factory{};

}

#endif /* TMPLT_TSTD_COMPOSE_H */
