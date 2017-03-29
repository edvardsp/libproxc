
#pragma once

#include <atomic>
#include <utility>

#include <proxc/config.hpp>

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

    Node * alloc_node()
    {
        if ( first_ != tail_copy_ ) {
            auto n = first_;
            first_ = first_->next_;
            return n;
        }
        tail_copy_ = tail_.load( std::memory_order_acquire );
        if ( first_ != tail_copy_ ) {
            auto n = first_;
            first_ = first_->next_;
            return n;
        }
        return new Node{};
    }

public:
    SpscQueue()
    {
        auto n = new Node{};
        tail_.store( n, std::memory_order_relaxed );
        head_ = first_ = tail_copy_ = n;
    }

    ~SpscQueue() noexcept
    {
        auto n = first_;
        do {
            auto next = n->next_.load( std::memory_order_relaxed );
            delete n;
            n = next;
        } while ( n != nullptr );
    }

    SpscQueue( SpscQueue const & ) = delete;
    SpscQueue & operator = ( SpscQueue const & ) = delete;

    bool is_empty() const noexcept
    {
         auto tail = tail_.load( std::memory_order_consume );
         return tail->next_.load( std::memory_order_acquire ) == nullptr;
    }

    void enqueue_impl( Node * n ) noexcept
    {
        head_->next_.store( n, std::memory_order_release );
        head_ = n;
    }

    void enqueue( ItemType const & item ) noexcept
    {
        auto n = alloc_node();
        n->item_ = item;
        enqueue_impl( n );
    }

    void enqueue( ItemType && item ) noexcept
    {
        auto n = alloc_node();
        n->item_ = std::forward< ItemType >( item );
        enqueue_impl( n );
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
};

} // namespace detail
PROXC_NAMESPACE_END

