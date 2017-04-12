
#pragma once

#include <proxc/config.hpp>

#include <proxc/alt/choice_base.hpp>
#include <proxc/channel/sync.hpp>
#include <proxc/channel/op_result.hpp>
#include <proxc/detail/delegate.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

template<typename T>
class ChoiceSend : public ChoiceBase
{
public:
    using ItemType = T;
    using TxType = channel::sync::Tx< ItemType >;
    using FnType = detail::delegate< void( void ) >;

    ChoiceSend( TxType & tx, ItemType const & item, FnType && fn )
        : tx_{ tx }
        , item_{ item }
        , fn_{ std::forward< FnType >( fn ) }
    {}

    ChoiceSend( TxType & tx, ItemType && item, FnType && fn )
        : tx_{ tx }
        , item_{ std::move( item ) }
        , fn_{ std::forward< FnType >( fn ) }
    {}

    bool is_ready() noexcept
    {
        return tx_.ready();
    }

    void complete_task() noexcept
    {
        auto res = tx_.send( std::move( item_ ) );
        BOOST_ASSERT( res == channel::OpResult::Ok );
        fn_();
    }

private:
    TxType &    tx_;
    ItemType    item_;
    FnType      fn_;
};

} // namespace alt
PROXC_NAMESPACE_END

