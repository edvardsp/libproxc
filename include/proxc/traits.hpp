
#pragma once

#include <type_traits>
#include <utility>

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN

namespace traits {

namespace detail {

typedef char one[1];
typedef char two[2];

} // namespace detail

template< typename Fn
        , typename ... Args
>
struct is_callable
{
private:
    template<typename = decltype( std::declval< Fn >()( std::declval< Args >() ... ) )>
    static detail::two & helper(int);
    static detail::one & helper(...);

public:
    using value = typename std::integral_constant< bool, sizeof(helper<>(0)) - 1 >::value;
};

template<typename T>
typename std::decay< T >::type
decay_copy(T && t) 
{
    return std::forward< T >(t);
}

} // namespace traits

PROXC_NAMESPACE_END


