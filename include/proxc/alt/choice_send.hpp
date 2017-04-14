
#pragma once

#include <proxc/config.hpp>

#include <proxc/alt/choice_base.hpp>
#include <proxc/channel.hpp>
#include <proxc/detail/delegate.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

template<typename T>
class ChoiceSend : public ChoiceBase
{
public:
    using ItemType = T;
    using TxType = channel::Tx< ItemType >;
    using FnType = detail::delegate< void( void ) >;

private:
    channel::ChanEnd &    end_;

    TxType &    tx_;
    ItemType    item_;
    FnType      fn_;

public:
    ChoiceSend( channel::ChanEnd & end, TxType & tx, ItemType const & item, FnType fn )
        : end_{ end }
        , tx_{ tx }
        , item_{ item }
        , fn_{ std::move( fn ) }
    {
        enter();
    }

    ChoiceSend( channel::ChanEnd & end, TxType & tx, ItemType && item, FnType fn )
        : end_{ end }
        , tx_{ tx }
        , item_{ std::move( item ) }
        , fn_{ std::move( fn ) }
    {
        enter();
    }

    ~ChoiceSend()
    {
        leave();
    }

    bool is_ready( Alt * alt ) const noexcept
    {
        return tx_.alt_ready( alt );
    }

    bool try_complete() noexcept
    {
        auto res = tx_.alt_send( item_ );
        return res == channel::AltResult::Ok;
    }

    void run_func() const noexcept
    {
        fn_();
    }

private:
    void enter() noexcept
    {
        tx_.alt_enter( end_ );
    }

    void leave() noexcept
    {
        tx_.alt_leave();
    }
};

} // namespace alt
PROXC_NAMESPACE_END

