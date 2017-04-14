
#pragma once

#include <proxc/config.hpp>

#include <proxc/alt/choice_base.hpp>
#include <proxc/channel.hpp>
#include <proxc/detail/delegate.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

template<typename T>
class ChoiceRecv : public ChoiceBase
{
public:
    using ItemType = T;
    using RxType = channel::Rx< ItemType >;
    using FnType = detail::delegate< void( ItemType ) >;

    ChoiceRecv( RxType & rx, FnType && fn )
        : rx_{ rx }
        , fn_{ std::forward< FnType >( fn )  }
    {}

    bool is_ready( Alt * alt ) const noexcept
    {
        return rx_.alt_ready( alt );
    }

    void complete_task() noexcept
    {
        ItemType item{};
        auto res = rx_.recv( item );
        BOOST_ASSERT( res == channel::OpResult::Ok );
        fn_( std::move( item ) );
    }

private:
    RxType &    rx_;
    FnType      fn_;
};

} // namespace alt
PROXC_NAMESPACE_END


