
#pragma once

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/channel.hpp>
#include <proxc/alt/choice_base.hpp>
#include <proxc/detail/delegate.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

template<typename T>
class ChoiceSend : public ChoiceBase
{
public:
    using ItemT = T;
    using TxT = channel::Tx< ItemT >;
    using EndT = channel::detail::ChanEnd< ItemT >;
    using FnT = proxc::detail::delegate< void( void ) >;

private:
    TxT &    tx_;
    ItemT    item_;
    EndT     end_;
    FnT      fn_;

public:
    ChoiceSend( Alt *         alt,
                Context *     ctx,
                TxT &         tx,
                ItemT const & item,
                FnT           fn )
        : ChoiceBase{ alt }
        , tx_{ tx }
        , item_{ item }
        , end_{ ctx, item_, this }
        , fn_{ std::move( fn ) }
    {}

    ChoiceSend( Alt *     alt,
                Context * ctx,
                TxT &     tx,
                ItemT &&  item,
                FnT       fn )
        : ChoiceBase{ alt }
        , tx_{ tx }
        , item_{ std::move( item ) }
        , end_{ ctx, item_, this }
        , fn_{ std::move( fn ) }
    {}

    ~ChoiceSend() {}

    void enter() noexcept
    {
        tx_.alt_enter( end_ );
    }

    void leave() noexcept
    {
        tx_.alt_leave();
    }

    bool is_ready() const noexcept
    {
        return tx_.alt_ready();
    }

    Result try_complete() noexcept
    {
        auto res = tx_.alt_send();
        switch ( res ) {
        case channel::AltResult::Ok:       return Result::Ok;
        case channel::AltResult::TryLater: return Result::TryLater;
        default:                           return Result::Failed;
        }
    }

    void run_func() const noexcept
    {
        if ( fn_ ) {
            fn_();
        }
    }
};

} // namespace alt
PROXC_NAMESPACE_END

