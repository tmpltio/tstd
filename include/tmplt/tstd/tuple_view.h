#ifndef TMPLT_TSTD_TUPLE_VIEW_H
#define TMPLT_TSTD_TUPLE_VIEW_H

#include "meta_iota_view.h"
#include "compose.h"
#include <tuple>
#include <cstddef>
#include <type_traits>
#include <ranges>
#include <concepts>
#include <utility>
#include <memory>
#include <iterator>
#include <functional>

namespace tmplt::tstd
{

namespace ranges
{

namespace detail
{

template<typename Tuple, std::size_t I>
struct qualify_as_get
{
private:
    template<typename T>
    using const_t = std::conditional_t<std::is_const_v<std::remove_reference_t<Tuple>>, std::add_const_t<T>, T>;

    template<typename T>
    using reference_t = std::conditional_t<std::is_lvalue_reference_v<Tuple>, std::add_lvalue_reference_t<T>, std::add_rvalue_reference_t<T>>;

public:
    using type = reference_t<const_t<std::tuple_element_t<I, std::remove_cvref_t<Tuple>>>>;
};

template<typename Tuple, std::size_t I>
using qualify_as_get_t = typename qualify_as_get<Tuple, I>::type;

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

}

template<typename T>
concept tuple_like = requires
{
    { std::tuple_size<std::remove_cvref_t<T>>::value } -> std::convertible_to<std::size_t>;
    []<std::size_t ... I>(std::index_sequence<I...>) requires (detail::getable<T, I> && ...)
    {}(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<T>>>{});
};

template<typename Tuple>
requires std::same_as<Tuple, std::remove_reference_t<Tuple>> && tuple_like<std::add_lvalue_reference_t<Tuple>>
class tuple_view : public std::ranges::view_interface<tuple_view<Tuple>>
{
    static void construction_check(Tuple&);

    static void construction_check(Tuple&&) = delete;

    static constexpr std::ranges::random_access_range auto index_range = views::meta_iota<std::tuple_size_v<std::remove_cv_t<Tuple>>>();

    class iterator
    {
        using index_iterator_t = std::ranges::iterator_t<decltype(index_range)>;
        using index_sentinel_t = std::ranges::sentinel_t<decltype(index_range)>;

        class value_creator
        {
            class caller_factory
            {
                template<typename Visitor>
                class caller
                {
                    template<std::size_t I>
                    requires detail::member_getable<std::add_lvalue_reference_t<Tuple>, I>
                    [[nodiscard]] constexpr decltype(auto) get_element() const noexcept(noexcept(std::declval<Tuple* const&>()->template get<I>()))
                    {
                        return tuple->template get<I>();
                    }

                    template<std::size_t I>
                    requires detail::adl_getable<std::add_lvalue_reference_t<Tuple>, I> && (!detail::member_getable<std::add_lvalue_reference_t<Tuple>, I>)
                    [[nodiscard]] constexpr decltype(auto) get_element() const noexcept(noexcept(get<I>(std::declval<Tuple&>())))
                    {
                        return get<I>(*tuple);
                    }

                public:
                    template<std::convertible_to<std::add_rvalue_reference_t<Visitor>> T>
                    explicit constexpr caller(Tuple* tuple, T&& visitor) noexcept : tuple{tuple}, visitor{std::forward<T>(visitor)}
                    {}

                    template<typename Index>
                    requires requires
                    {
                        { Index::value } -> std::convertible_to<std::size_t>;
                        
                    } && std::invocable<Visitor, detail::qualify_as_get_t<std::add_lvalue_reference_t<Tuple>, Index::value>>
                    constexpr decltype(auto) operator()(Index const&) const noexcept(std::is_nothrow_invocable_v<Visitor, detail::qualify_as_get_t<std::add_lvalue_reference_t<Tuple>, Index::value>>)
                    {
                        return std::invoke(std::forward<Visitor>(visitor), get_element<Index::value>());
                    }

                private:
                    Tuple* tuple;
                    std::add_rvalue_reference_t<Visitor> visitor;
                };

                template<typename Visitor>
                caller(Tuple*, Visitor&&) -> caller<Visitor>;

            public:
                template<typename Visitor>
                [[nodiscard]] static constexpr auto operator()(Tuple* tuple, Visitor&& visitor) noexcept(noexcept(caller{std::declval<Tuple*&>(), std::declval<Visitor>()}))
                {
                    return caller{tuple, std::forward<Visitor>(visitor)};
                }
            };

            [[nodiscard]] static consteval bool is_invoke_noexcept() noexcept
            {
                using bind_result_t = decltype(std::bind_front(std::declval<caller_factory>(), std::declval<Tuple*&>()));

                constexpr auto is_bind_noexcept = noexcept(std::bind_front(std::declval<caller_factory>(), std::declval<Tuple*&>()));
                constexpr auto is_compose_noexcept = noexcept(compose(std::declval<std::iter_reference_t<index_iterator_t>>(), std::declval<bind_result_t>()));

                return is_bind_noexcept && is_compose_noexcept;
            }

        public:
            [[nodiscard]] static constexpr auto operator()(Tuple* tuple, index_iterator_t index) noexcept(is_invoke_noexcept())
            {
                return compose(*index, std::bind_front(caller_factory{}, tuple));
            }
        };

    public:
        using value_type = std::invoke_result_t<value_creator, Tuple* const&, index_iterator_t const&>;

        explicit constexpr iterator() = default;

        explicit constexpr iterator(Tuple* tuple, index_iterator_t index) noexcept(std::is_nothrow_copy_constructible_v<index_iterator_t>) : tuple{tuple}, index{index}
        {}

        [[nodiscard]] constexpr value_type operator*() const noexcept(std::is_nothrow_invocable_v<value_creator, Tuple* const&, index_iterator_t const&>)
        {
            return std::invoke(value_creator{}, tuple, index);
        }

        template<std::integral T>
        [[nodiscard]] constexpr value_type operator[](T difference) const noexcept(noexcept(*(std::declval<iterator const&>() + std::declval<T&>())))
        {
            return *(*this + difference);
        }

        constexpr iterator& operator++() noexcept(noexcept(++std::declval<index_iterator_t&>()))
        {
            ++index;

            return *this;
        }

        constexpr iterator operator++(int) noexcept(std::is_nothrow_copy_constructible_v<iterator> && noexcept(++std::declval<iterator&>()))
        {
            auto tmp = *this;
            ++*this;

            return tmp;
        }

        constexpr iterator& operator--() noexcept(noexcept(--std::declval<index_iterator_t&>()))
        {
            --index;

            return *this;
        }

        constexpr iterator operator--(int) noexcept(std::is_nothrow_copy_constructible_v<iterator> && noexcept(--std::declval<iterator&>()))
        {
            auto tmp = *this;
            --*this;

            return tmp;
        }

        template<std::integral T>
        constexpr iterator& operator+=(T difference) noexcept(noexcept(std::declval<index_iterator_t&>() += std::declval<T&>()))
        {
            index += difference;

            return *this;
        }

        template<std::integral T>
        constexpr iterator& operator-=(T difference) noexcept(noexcept(std::declval<index_iterator_t&>() -= std::declval<T&>()))
        {
            index -= difference;

            return *this;
        }

        template<std::integral T>
        [[nodiscard]] friend constexpr iterator operator+(iterator it, T difference) noexcept(noexcept(std::declval<iterator&>() += std::declval<T&>()) && std::is_nothrow_copy_constructible_v<iterator>)
        {
            return it += difference;
        }

        template<std::integral T>
        [[nodiscard]] friend constexpr iterator operator+(T difference, iterator it) noexcept(noexcept(std::declval<iterator&>() + std::declval<T&>()) && std::is_nothrow_copy_constructible_v<iterator>)
        {
            return it + difference;
        }

        template<std::integral T>
        [[nodiscard]] friend constexpr iterator operator-(iterator it, T difference) noexcept(noexcept(std::declval<iterator&>() -= std::declval<T&>()) && std::is_nothrow_copy_constructible_v<iterator>)
        {
            return it -= difference;
        }

        [[nodiscard]] friend constexpr std::signed_integral auto operator-(iterator const& lhs, iterator const& rhs) noexcept(noexcept(std::declval<index_iterator_t const&>() - std::declval<index_iterator_t const&>()))
        {
            return lhs.index - rhs.index;
        }

        [[nodiscard]] friend constexpr std::signed_integral auto operator-(iterator const& it, index_sentinel_t const& sentinel) noexcept(noexcept(std::declval<index_iterator_t const&>() - std::declval<index_sentinel_t const&>()))
        {
            return it.index - sentinel;
        }

        [[nodiscard]] friend constexpr std::signed_integral auto operator-(index_sentinel_t const& sentinel, iterator const& it) noexcept(noexcept(std::declval<index_sentinel_t const&>() - std::declval<index_iterator_t const&>()))
        {
            return sentinel - it.index;
        }

        [[nodiscard]] friend constexpr bool operator==(iterator const&, iterator const&) = default;

        [[nodiscard]] friend constexpr auto operator<=>(iterator const&, iterator const&) = default;

        [[nodiscard]] friend constexpr bool operator==(iterator const& it, index_sentinel_t const& sentinel) noexcept(noexcept(it.index == sentinel))
        {
            return it.index == sentinel;
        }

    private:
        Tuple* tuple{nullptr};
        index_iterator_t index{};
    };

public:
    template<tuple_like T>
    requires std::convertible_to<T, Tuple&> && requires { construction_check(std::declval<T>()); }
    explicit constexpr tuple_view(T&& tuple) noexcept(noexcept(static_cast<Tuple&>(std::declval<T>()))) : tuple{std::addressof(static_cast<Tuple&>(std::forward<T>(tuple)))}
    {}

    constexpr std::random_access_iterator auto begin() const noexcept(noexcept(std::ranges::begin(std::declval<decltype((index_range))>())) && std::is_nothrow_constructible_v<iterator, Tuple* const&, std::ranges::iterator_t<decltype(index_range)>>)
    {
        return iterator{tuple, std::ranges::begin(index_range)};
    }

    constexpr std::sentinel_for<std::ranges::iterator_t<tuple_view>> auto end() const noexcept(noexcept(std::ranges::end(std::declval<decltype((index_range))>())))
    {
        return std::ranges::end(index_range);
    }

private:
    Tuple* tuple;
};

template<typename Tuple>
tuple_view(Tuple&&) -> tuple_view<std::remove_reference_t<Tuple>>;

namespace views
{

struct tuple_view_factory
{
    template<tuple_like Tuple>
    [[nodiscard]] static constexpr auto operator()(Tuple&& tuple) noexcept(noexcept(tuple_view{std::declval<Tuple>()}))
    {
        return tuple_view{std::forward<Tuple>(tuple)};
    }
};

inline constexpr auto tuple = tuple_view_factory{};

}

}

namespace views = ranges::views;

}

template<typename Tuple>
inline constexpr bool std::ranges::enable_borrowed_range<tmplt::tstd::ranges::tuple_view<Tuple>> = true;

#endif /* TMPLT_TSTD_TUPLE_VIEW_H */
