
#pragma once

#include <proxc/config.hpp>

#include <proxc/channel.hpp>
#include <proxc/context.hpp>
#include <proxc/alt/choice_base.hpp>
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

private:
    channel::ChanEnd    end_;

    RxType &     rx_;
    FnType       fn_;
    ItemType     item_;

public:
    ChoiceRecv( Alt * alt,
                Context * ctx,
                RxType & rx,
                FnType fn )
        : ChoiceBase{ alt }
        , end_{ ctx, this }
        , rx_{ rx }
        , fn_{ std::move( fn ) }
        , item_{}
    {}

    ~ChoiceRecv() {}

    void enter() noexcept
    {
        rx_.alt_enter( end_ );
    }

    void leave() noexcept
    {
        rx_.alt_leave();
    }

    bool is_ready() const noexcept
    {
        return rx_.alt_ready( this );
    }

    bool try_complete() noexcept
    {
        auto res = rx_.alt_recv( item_ );
        return res == channel::AltResult::Ok;
    }

    void run_func() const noexcept
    {
        fn_( std::move( item_ ) );
    }
};

} // namespace alt
PROXC_NAMESPACE_END


