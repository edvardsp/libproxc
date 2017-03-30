
#pragma once

#include <atomic>
#include <utility>

#include <proxc/config.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN
namespace detail {

// http://www.1024cores.net/home/lock-free-algorithms/queues/unbounded-spsc-queue
template<typename T>
class SpscQueue
{
private:
    using ItemType = T;

    struct alignas(cache_alignment) Node {
        std::atomic< Node * >    next_{ nullptr };
        ItemType                 item_{};
    };

    // accessed mainly by consumer, infrequently by producer
    alignas(cache_alignment) std::atomic< Node * >    tail_;
    // accessed only by producer
    Node *    head_;
    Node *    first_;
    Node *    tail_copy_;

public:
    SpscQueue()
    {
        auto n = new Node{};
        tail_.store( n, std::memory_order_relaxed );
        head_ = first_ = tail_copy_ = n;
    }

    ~SpscQueue() noexcept
    {
        BOOST_ASSERT( first_ != nullptr );
        auto n = first_;
        do {
            auto next = n->next_.load( std::memory_order_relaxed );
            delete n;
            n = next;
        } while ( n != nullptr );
    }

    // make non-copyable
    SpscQueue( SpscQueue const & ) = delete;
    SpscQueue & operator = ( SpscQueue const & ) = delete;

    bool is_empty() const noexcept
    {
         auto tail = tail_.load( std::memory_order_consume );
         return tail->next_.load( std::memory_order_acquire ) == nullptr;
    }

    void enqueue( ItemType const & item ) noexcept
    {
        auto n = alloc_node_();
        n->item_ = item;
        enqueue_impl_( n );
    }

    void enqueue( ItemType && item ) noexcept
    {
        auto n = alloc_node_();
        n->item_ = std::forward< ItemType >( item );
        enqueue_impl_( n );
    }

    bool dequeue( ItemType & item ) noexcept
    {
        auto tail = tail_.load( std::memory_order_consume );
        auto next = tail->next_.load( std::memory_order_acquire );
        if ( next != nullptr ) {
            item = next->item_;
            tail_.store( next, std::memory_order_release );
            return true;
        } else {
            return false;
        }
    }

private:
    void enqueue_impl_( Node * n ) noexcept
    {
        BOOST_ASSERT( n != nullptr );
        head_->next_.store( n, std::memory_order_release );
        head_ = n;
    }

    Node * alloc_node_()
    {
        if ( first_ != tail_copy_ ) {
            auto n = first_;
            first_ = first_->next_;
            n->next_.store( nullptr, std::memory_order_relaxed );
            return n;
        }
        tail_copy_ = tail_.load( std::memory_order_acquire );
        if ( first_ != tail_copy_ ) {
            auto n = first_;
            first_ = first_->next_;
            n->next_.store( nullptr, std::memory_order_relaxed );
            return n;
        }
        return new Node{};
    }

};

} // namespace detail
PROXC_NAMESPACE_END

