#ifndef TMPLT_TSTD_TUPLE_VIEW_H
#define TMPLT_TSTD_TUPLE_VIEW_H

#include "tuple_like.h"
#include "meta_iota_view.h"
#include "compose.h"
#include <ranges>
#include <memory>
#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>
#include <functional>
#include <compare>
#include <cstddef>

namespace tmplt::tstd
{

namespace ranges
{

template<tuple_like Tuple>
class tuple_view : public std::ranges::view_interface<tuple_view<Tuple>>
{
    template<typename T>
    class reference
    {
    public:
        explicit constexpr reference(T& value) noexcept : storage{std::addressof(value)}
        {}

        [[nodiscard]] constexpr auto& get() const noexcept
        {
            return *storage;
        }

    private:
        T* storage;
    };

    template<std::move_constructible T>
    class owner
    {
    public:
        template<typename U>
        requires std::constructible_from<T, U>
        explicit constexpr owner(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>) : storage{std::forward<U>(value)}
        {}

        constexpr owner(owner&&) = default;

        constexpr owner& operator=(owner&& other)
        {
            if(this != std::addressof(other))
            {
                if(other.storage.has_value())
                {
                    storage.emplace(other.get());
                }
                else
                {
                    storage.reset();
                }
            }

            return *this;
        }

        [[nodiscard]] constexpr auto&& get(this auto&& self) noexcept
        {
            return std::move(*self.storage);
        }

    private:
        std::optional<T> storage;
    };

    template<std::move_constructible T>
    requires std::movable<T> || std::is_nothrow_move_constructible_v<T>
    class owner<T>
    {
    public:
        template<typename U>
        requires std::constructible_from<T, U>
        explicit constexpr owner(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>) : storage{std::forward<U>(value)}
        {}

        constexpr owner(owner&&) = default;

        constexpr owner& operator=(owner&&) requires std::movable<T> = default;

        constexpr owner& operator=(owner&& other) noexcept
        {
            if(this != std::addressof(other))
            {
                storage.~T();
                ::new(const_cast<void*>(static_cast<void const volatile*>(std::addressof(storage)))) T(other.get());
            }

            return *this;
        }

        [[nodiscard]] constexpr auto&& get(this auto&& self) noexcept
        {
            return std::move(self.storage);
        }

    private:
        T storage;
    };

    using storage_t = std::conditional_t<std::is_lvalue_reference_v<Tuple>, reference<std::remove_reference_t<Tuple>>, owner<std::remove_reference_t<Tuple>>>;

    static constexpr std::ranges::random_access_range auto indexes = views::meta_iota<std::tuple_size_v<std::remove_cvref_t<Tuple>>>();

    template<typename Storage>
    class iterator
    {
        using index_t = std::ranges::iterator_t<decltype(indexes)>;
        using sentinel_t = std::ranges::sentinel_t<decltype(indexes)>;

        class value_factory
        {
            class caller_factory
            {
                template<typename Visitor>
                class caller
                {
                    using visitor_t = std::add_rvalue_reference_t<Visitor>;
                    using stored_t = decltype(std::declval<Storage* const&>()->get());

                    template<std::size_t I, typename T>
                    requires member_getable<T, I>
                    [[nodiscard]] static constexpr decltype(auto) get_element(T&& tuple) noexcept(noexcept(std::declval<T>().template get<I>()))
                    {
                        return std::forward<T>(tuple).template get<I>();
                    }

                    template<std::size_t I, typename T>
                    requires adl_getable<T, I> && (!member_getable<T, I>)
                    [[nodiscard]] static constexpr decltype(auto) get_element(T&& tuple) noexcept(noexcept(get<I>(std::declval<T>())))
                    {
                        return get<I>(std::forward<T>(tuple));
                    }

                public:
                    template<std::convertible_to<visitor_t> T>
                    explicit constexpr caller(Storage* storage, T&& visitor) noexcept : storage{storage}, visitor{std::forward<T>(visitor)}
                    {}

                    template<typename Index>
                    requires requires
                    {
                        { Index::value } -> std::convertible_to<std::size_t>;
                    } && std::invocable<visitor_t, qualify_as_get_t<stored_t, Index::value>>
                    constexpr decltype(auto) operator()(Index const&) const noexcept(std::is_nothrow_invocable_v<visitor_t, qualify_as_get_t<stored_t, Index::value>>)
                    {
                        return std::invoke(std::forward<Visitor>(visitor), get_element<Index::value>(storage->get()));
                    }

                private:
                    Storage* storage;
                    visitor_t visitor;
                };

                template<typename Visitor>
                caller(Storage*, Visitor&&) -> caller<Visitor>;

            public:
                template<typename Visitor>
                [[nodiscard]] static constexpr auto operator()(Storage* storage, Visitor&& visitor) noexcept(noexcept(caller{std::declval<Storage*&>(), std::declval<Visitor>()}))
                {
                    return caller{storage, std::forward<Visitor>(visitor)};
                }
            };

            [[nodiscard]] static consteval bool is_invoke_noexcept() noexcept
            {
                using bind_result_t = decltype(std::bind_front(std::declval<caller_factory>(), std::declval<Storage*&>()));

                constexpr auto is_dereference_noexcept = noexcept(*std::declval<index_t const&>());
                constexpr auto is_caller_nothrow_constructible = std::is_nothrow_constructible_v<caller_factory>;
                constexpr auto is_bind_noexcept = noexcept(std::bind_front(std::declval<caller_factory>(), std::declval<Storage*&>()));
                constexpr auto is_compose_noexcept = noexcept(compose(std::declval<std::iter_reference_t<index_t>>(), std::declval<bind_result_t>()));

                return is_dereference_noexcept && is_caller_nothrow_constructible && is_bind_noexcept && is_compose_noexcept;
            }

        public:
            [[nodiscard]] static constexpr auto operator()(Storage* storage, index_t const& index) noexcept(is_invoke_noexcept())
            {
                return compose(*index, std::bind_front(caller_factory{}, storage));
            }
        };

    public:
        using value_type = std::invoke_result_t<value_factory, Storage* const&, index_t const&>;

        explicit constexpr iterator() = default;

        explicit constexpr iterator(Storage* storage, index_t index) noexcept(std::is_nothrow_copy_constructible_v<index_t>) : storage{storage}, index{index}
        {}

        [[nodiscard]] constexpr auto operator*() const noexcept(std::is_nothrow_constructible_v<value_factory> && std::is_nothrow_invocable_v<value_factory, Storage* const&, index_t const&>)
        {
            return std::invoke(value_factory{}, storage, index);
        }

        template<std::integral T>
        [[nodiscard]] constexpr auto operator[](T difference) const noexcept(noexcept(*(std::declval<iterator const&>() + std::declval<T&>())))
        {
            return *(*this + difference);
        }

        constexpr iterator& operator++() noexcept(noexcept(++std::declval<index_t&>()))
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

        constexpr iterator& operator--() noexcept(noexcept(--std::declval<index_t&>()))
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
        constexpr iterator& operator+=(T difference) noexcept(noexcept(std::declval<index_t&>() += std::declval<T&>()))
        {
            index += difference;

            return *this;
        }

        template<std::integral T>
        constexpr iterator& operator-=(T difference) noexcept(noexcept(std::declval<index_t&>() -= std::declval<T&>()))
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

        template<typename T>
        [[nodiscard]] friend constexpr std::signed_integral auto operator-(iterator const& lhs, iterator<T> const& rhs) noexcept(noexcept(std::declval<index_t const&>() - std::declval<index_t const&>()))
        {
            return lhs.index - rhs.index;
        }

        [[nodiscard]] friend constexpr std::signed_integral auto operator-(iterator const& it, sentinel_t const& sentinel) noexcept(noexcept(std::declval<index_t const&>() - std::declval<sentinel_t const&>()))
        {
            return it.index - sentinel;
        }

        [[nodiscard]] friend constexpr std::signed_integral auto operator-(sentinel_t const& sentinel, iterator const& it) noexcept(noexcept(std::declval<sentinel_t const&>() - std::declval<index_t const&>()))
        {
            return sentinel - it.index;
        }

        template<typename T>
        [[nodiscard]] friend constexpr bool operator==(iterator const& lhs, iterator<T> const& rhs) noexcept(noexcept(std::declval<index_t const&>() == std::declval<index_t const&>()))
        {
            return lhs.storage == rhs.storage ? lhs.index == rhs.index : false;
        }

        template<typename T>
        [[nodiscard]] friend constexpr auto operator<=>(iterator const& lhs, iterator<T> const& rhs) noexcept(std::is_nothrow_invocable_v<decltype(std::compare_weak_order_fallback), index_t const&, index_t const&>)
        {
            return lhs.storage == rhs.storage ? std::compare_weak_order_fallback(lhs.index, rhs.index) : std::partial_ordering::unordered;
        }

        [[nodiscard]] friend constexpr bool operator==(iterator const& it, sentinel_t const& sentinel) noexcept(noexcept(std::declval<index_t const&>() == std::declval<sentinel_t const&>()))
        {
            return it.index == sentinel;
        }

    private:
        Storage* storage{nullptr};
        index_t index{};
    };

    template<typename T>
    [[nodiscard]] static consteval bool is_begin_noexcept() noexcept
    {
        using qualified_storage_t = std::add_pointer_t<std::conditional_t<std::is_const_v<std::remove_reference_t<T>>, std::add_const_t<storage_t>, storage_t>>;

        constexpr auto is_indexes_begin_noexcept = noexcept(std::ranges::begin(std::declval<decltype((indexes))>()));
        constexpr auto is_iterator_nothrow_constructible = std::is_nothrow_constructible_v<iterator<qualified_storage_t>, qualified_storage_t, std::ranges::iterator_t<decltype(indexes)>>;

        return is_indexes_begin_noexcept && is_iterator_nothrow_constructible;
    }

    [[nodiscard]] static consteval bool is_end_noexcept() noexcept
    {
        return noexcept(std::ranges::end(std::declval<decltype((indexes))>()));
    }

public:
    template<tuple_like T>
    requires std::constructible_from<storage_t, T>
    explicit constexpr tuple_view(T&& tuple) noexcept(std::is_nothrow_constructible_v<storage_t, T>) : storage{std::forward<T>(tuple)}
    {}

    template<typename T>
    constexpr std::random_access_iterator auto begin(this T&& self) noexcept(is_begin_noexcept<T>())
    {
        return iterator{std::addressof(self.storage), std::ranges::begin(indexes)};
    }

    template<typename T>
    constexpr std::sized_sentinel_for<std::ranges::iterator_t<T>> auto end(this T&&) noexcept(is_end_noexcept())
    {
        return std::ranges::end(indexes);
    }

private:
    storage_t storage;
};

template<typename Tuple>
tuple_view(Tuple&&) -> tuple_view<Tuple>;

namespace views
{

struct tuple_view_factory
{
    template<tuple_like Tuple>
    [[nodiscard]] static constexpr std::ranges::view auto operator()(Tuple&& tuple) noexcept(noexcept(tuple_view{std::declval<Tuple>()}))
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
