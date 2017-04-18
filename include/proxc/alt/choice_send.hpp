
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
    using ItemType = T;
    using TxType = channel::Tx< ItemType >;
    using FnType = detail::delegate< void( void ) >;

private:
    channel::ChanEnd    end_;

    TxType &    tx_;
    ItemType    item_;
    FnType      fn_;

public:
    ChoiceSend( Alt * alt,
                Context * ctx,
                TxType & tx,
                ItemType const & item,
                FnType fn )
        : ChoiceBase{ alt }
        , end_{ ctx, this }
        , tx_{ tx }
        , item_{ item }
        , fn_{ std::move( fn ) }
    {}

    ChoiceSend( Alt * alt,
                Context * ctx,
                TxType & tx,
                ItemType && item,
                FnType fn )
        : ChoiceBase{ alt }
        , end_{ ctx, this }
        , tx_{ tx }
        , item_{ std::move( item ) }
        , fn_{ std::move( fn ) }
    {}
<<<<<<< HEAD
=======
<<<<<<< HEAD

    ~ChoiceSend() {}
=======
>>>>>>> merge

    ~ChoiceSend() {}

    void enter() noexcept
    {
        tx_.alt_enter( end_ );
    }

    void leave() noexcept
    {
        tx_.alt_leave();
    }
>>>>>>> origin/diploma

    bool is_ready() const noexcept
    {
        return tx_.alt_ready( this );
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
<<<<<<< HEAD
=======
<<<<<<< HEAD

    void enter() noexcept
    {
        tx_.alt_enter( end_ );
    }

    void leave() noexcept
    {
        tx_.alt_leave();
    }
=======
>>>>>>> origin/diploma
>>>>>>> merge
};

} // namespace alt
PROXC_NAMESPACE_END

