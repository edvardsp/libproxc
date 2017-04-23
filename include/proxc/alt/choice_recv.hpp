
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
    using ItemT = T;
    using RxT   = channel::Rx< ItemT >;
    using EndT  = channel::detail::ChanEnd< ItemT >;
    using FnT   = proxc::detail::delegate< void( ItemT ) >;

private:
    RxT &    rx_;
    ItemT    item_;
    EndT     end_;
    FnT      fn_;

public:
    ChoiceRecv( Alt *     alt,
                Context * ctx,
                RxT &     rx,
                FnT       fn )
        : ChoiceBase{ alt }
        , rx_{ rx }
        , item_{}
        , end_{ ctx, item_, this }
        , fn_{ std::move( fn ) }
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
        return rx_.alt_ready();
    }

    bool try_complete() noexcept
    {
        auto res = rx_.alt_recv();
        return res == channel::AltResult::Ok;
    }

    void run_func() const noexcept
    {
        if ( fn_ ) {
            fn_( std::move( item_ ) );
        }
    }
};

} // namespace alt
PROXC_NAMESPACE_END


