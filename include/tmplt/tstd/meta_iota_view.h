#ifndef TMPLT_TSTD_META_IOTA_VIEW_H
#define TMPLT_TSTD_META_IOTA_VIEW_H

#include "least.h"
#include <cstddef>
#include <limits>
#include <ranges>
#include <concepts>
#include <variant>
#include <array>
#include <utility>
#include <type_traits>
#include <iterator>

namespace tmplt::tstd
{

namespace ranges
{

template<std::size_t Count>
requires (Count <= std::numeric_limits<least_signed_t<std::numeric_limits<std::size_t>::max()>>::max())
class meta_iota_view : public std::ranges::view_interface<meta_iota_view<Count>>
{
    template<std::size_t ... I>
    class callable_visit
    {
        using variant_t = std::variant<std::integral_constant<std::size_t, I>...>;

    public:
        template<typename Index>
        requires (std::same_as<std::remove_cvref_t<Index>, std::in_place_index_t<I>> || ...)
        explicit consteval callable_visit(Index&& index) noexcept : variant{std::forward<Index>(index)}
        {}

        template<typename Visitor>
        requires requires(Visitor&& visitor, variant_t const& variant)
        {
            std::visit(std::forward<Visitor>(visitor), variant);
        }
        constexpr decltype(auto) operator()(Visitor&& visitor) const noexcept(noexcept(std::visit(std::declval<Visitor>(), std::declval<variant_t const&>())))
        {
            return std::visit(std::forward<Visitor>(visitor), variant);
        }

    private:
        variant_t variant;
    };

    static constexpr std::ranges::random_access_range auto values = []<std::size_t ... I>(std::index_sequence<I...>)
    {
        return std::array{callable_visit<I...>{std::in_place_index<I>}...};
    }(std::make_index_sequence<Count>{});

    class iterator
    {
        using index_t = std::make_unsigned_t<least_signed_t<Count>>;

    public:
        using value_type = std::ranges::range_value_t<decltype(values)>;

        explicit constexpr iterator() = default;

        [[nodiscard]] constexpr value_type operator*() const noexcept(std::is_nothrow_copy_constructible_v<value_type>)
        {
            return values[index];
        }

        template<std::integral T>
        [[nodiscard]] constexpr value_type operator[](T difference) const noexcept(noexcept(*(std::declval<iterator const&>() + std::declval<T&>())))
        {
            return *(*this + difference);
        }

        constexpr iterator& operator++() noexcept
        {
            ++index;

            return *this;
        }

        constexpr iterator operator++(int) noexcept(std::is_nothrow_copy_constructible_v<iterator>)
        {
            auto tmp = *this;
            ++*this;

            return tmp;
        }

        constexpr iterator& operator--() noexcept
        {
            --index;

            return *this;
        }

        constexpr iterator operator--(int) noexcept(std::is_nothrow_copy_constructible_v<iterator>)
        {
            auto tmp = *this;
            --*this;

            return tmp;
        }

        constexpr iterator& operator+=(std::integral auto difference) noexcept
        {
            index += index_t(difference);

            return *this;
        }

        constexpr iterator& operator-=(std::integral auto difference) noexcept
        {
            index -= index_t(difference);

            return *this;
        }

        [[nodiscard]] friend constexpr iterator operator+(iterator it, std::integral auto difference) noexcept(std::is_nothrow_copy_constructible_v<iterator>)
        {
            return it += difference;
        }

        [[nodiscard]] friend constexpr iterator operator+(std::integral auto difference, iterator it) noexcept(std::is_nothrow_copy_constructible_v<iterator>)
        {
            return it + difference;
        }

        [[nodiscard]] friend constexpr iterator operator-(iterator it, std::integral auto difference) noexcept(std::is_nothrow_copy_constructible_v<iterator>)
        {
            return it -= difference;
        }

        [[nodiscard]] friend constexpr std::signed_integral auto operator-(iterator const& lhs, iterator const& rhs) noexcept
        {
            return std::make_signed_t<index_t>(lhs.index - rhs.index);
        }

        [[nodiscard]] friend constexpr std::signed_integral auto operator-(iterator const& it, std::default_sentinel_t const&) noexcept
        {
            return std::make_signed_t<index_t>(it.index - Count);
        }

        [[nodiscard]] friend constexpr std::signed_integral auto operator-(std::default_sentinel_t const&, iterator const& it) noexcept
        {
            return std::make_signed_t<index_t>(Count - it.index);
        }

        [[nodiscard]] friend constexpr bool operator==(iterator const&, iterator const&) = default;

        [[nodiscard]] friend constexpr auto operator<=>(iterator const&, iterator const&) = default;

        [[nodiscard]] friend constexpr bool operator==(iterator const& it, std::default_sentinel_t const&) noexcept
        {
            return it.index == Count;
        }

    private:
        index_t index{};
    };

public:
    constexpr std::random_access_iterator auto begin(this auto) noexcept(std::is_nothrow_default_constructible_v<iterator>)
    {
        return iterator{};
    }

    template<typename T>
    constexpr std::sized_sentinel_for<std::ranges::iterator_t<T>> auto end(this T) noexcept(std::is_nothrow_copy_constructible_v<std::default_sentinel_t>)
    {
        return std::default_sentinel;
    }
};

namespace views
{

template<std::size_t Count>
class meta_iota_view_factory
{
public:
    [[nodiscard]] static constexpr std::ranges::view auto operator()() noexcept(std::is_nothrow_default_constructible_v<meta_iota_view<Count>>)
    {
        return meta_iota_view<Count>{};
    }
};

template<std::size_t Count>
inline constexpr auto meta_iota = meta_iota_view_factory<Count>{};

}

}

namespace views = ranges::views;

}

template<std::size_t Count>
inline constexpr bool std::ranges::enable_borrowed_range<tmplt::tstd::ranges::meta_iota_view<Count>> = true;

#endif /* TMPLT_TSTD_META_IOTA_VIEW_H */
