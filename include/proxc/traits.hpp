
#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include <proxc/config.hpp>

#include <boost/context/detail/apply.hpp>

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
    template<
        typename = decltype( std::declval< Fn >()( std::declval< Args >() ... ) )
    >
    static detail::two & helper(int);
    static detail::one & helper(...);

public:
    static constexpr bool value = std::integral_constant< bool, sizeof(helper(0)) - 1 >();
};

template<typename T>
typename std::decay< T >::type
decay_copy(T && t) 
{
    return std::forward< T >(t);
}

template<typename Fn, typename ...Args  >
decltype(auto) make_bind(Fn && fn, Args && ... args)
{
    auto tpl = std::make_tuple( std::forward< Args >(args) ... );
    return [fn  = decay_copy( std::forward< Fn >(fn) ),
            tpl = std::move(tpl)]() -> void { 
        boost::context::detail::apply(std::move(fn), std::move(tpl));
    };
}

} // namespace traits

PROXC_NAMESPACE_END


