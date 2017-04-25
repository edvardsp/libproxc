
#pragma once

#include <atomic>
#include <iostream>
#include <memory>

#include <proxc/config.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN
namespace detail {
namespace queue {

// based on Dmitry Vyukov's intrusive MPSC queue
// http://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue
// https://groups.google.com/forum/#!topic/lock-free/aFHvZhu1G-0

template<typename T>
class Mpsc
{
private:
    using ItemT = T;
    using StorageT = typename std::aligned_storage<
        sizeof( T ), alignof( T )
    >::type;

    alignas(cache_alignment) StorageT    storage_{};
    ItemT *                              stub_;

    alignas(cache_alignment) std::atomic< ItemT * >    head_;
    alignas(cache_alignment) ItemT *                   tail_;

public:
    // constructor and destructor
    Mpsc()
        : stub_{ reinterpret_cast< ItemT * >( std::addressof( storage_ ) ) }
        , head_{ stub_ }
        , tail_{ stub_ }
    {
        stub_->mpsc_next_.store( nullptr, std::memory_order_release );
    }
    ~Mpsc() {}

    // make non-copyable
    Mpsc( Mpsc const & )               = delete;
    Mpsc & operator = ( Mpsc const & ) = delete;

    // called by multiple producers
    void push( ItemT * item ) noexcept
    {
        BOOST_ASSERT( item != nullptr );
        item->mpsc_next_.store( nullptr, std::memory_order_release );
        ItemT * prev = head_.exchange( item, std::memory_order_acq_rel );
        prev->mpsc_next_.store( item, std::memory_order_release );
    }

    // called by single consumer
    ItemT * pop() noexcept
    {
        ItemT * tail = tail_;
        ItemT * next = tail->mpsc_next_.load( std::memory_order_acquire );
        if ( tail == stub_ ) {
            if ( next == nullptr ) {
                return nullptr;
            }
            tail_ = next;
            tail = next;
            next = next->mpsc_next_.load( std::memory_order_acquire );
        }
        if ( next != nullptr ) {
            tail_ = next;
            tail->mpsc_next_.store( nullptr, std::memory_order_release );
            return tail;
        }
        ItemT * head = head_.load( std::memory_order_acquire );
        if ( tail != head ) {
            return nullptr;
        }
        push( stub_ );
        next = tail->mpsc_next_.load( std::memory_order_acquire );
        if ( next != nullptr ) {
            tail_ = next;
            tail->mpsc_next_.store( nullptr, std::memory_order_release );
            return tail;
        }
        return nullptr;
    }
};

} // namespace queue
} // namespace detail
PROXC_NAMESPACE_END

