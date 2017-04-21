
#pragma once

#include <proxc/config.hpp>

#include <proxc/detail/circular_array.hpp>

#include <atomic>
#include <memory>
#include <utility>

PROXC_NAMESPACE_BEGIN
namespace detail {

constexpr std::ptrdiff_t to_signed( std::size_t x ) noexcept
{
    constexpr std::size_t ptrdiff_max = static_cast< std::size_t >( PTRDIFF_MAX );
    constexpr std::size_t ptrdiff_min = static_cast< std::size_t >( PTRDIFF_MIN );
    static_assert( ptrdiff_max + 1 == ptrdiff_min, "Wrong unsigned integer wrapping behaviour");

    if ( x > ptrdiff_max ) {
        return static_cast< std::ptrdiff_t >( x - ptrdiff_min ) + PTRDIFF_MIN;
    } else {
        return static_cast< std::ptrdiff_t >( x );
    }
}

} // namespace detail

// Chase-Lev work-steal deque
//
// Dynamic Circular Work-Stealing Deque
// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.170.1097&rep=rep1&type=pdf
//
// Correct and Efﬁcient Work-Stealing for Weak Memory Models
// http://www.di.ens.fr/~zappa/readings/ppopp13.pdf
template<typename ItemT>
class WorkStealDeque
{
public:
    static constexpr std::size_t default_capacity = 1024;

private:
    using ArrayT = detail::CircularArray< ItemT * >;

    alignas(cache_alignment) std::atomic< std::size_t >    top_{ 0 };
    alignas(cache_alignment) std::atomic< std::size_t >    bottom_{ 0 };
    alignas(cache_alignment) std::atomic< ArrayT * >    array_;
    char padding_[cacheline_length];

public:
    WorkStealDeque()
        : array_{ new ArrayT{ default_capacity } }
    {}

    ~WorkStealDeque() noexcept
    {
        delete array_.load();
    }

    WorkStealDeque( WorkStealDeque const & )             = delete;
    WorkStealDeque & operator=( WorkStealDeque const & ) = delete;

    bool is_empty() const noexcept
    {
        std::size_t bottom = bottom_.load( std::memory_order_relaxed );
        std::size_t top = top_.load( std::memory_order_relaxed );
        return bottom <= top;
    }

    std::size_t capacity() const noexcept
    {
        auto array = array_.load( std::memory_order_acquire );
        return array->size();
    }

    void reserve( std::size_t capacity ) noexcept
    {
        auto array = array_.load( std::memory_order_relaxed );
        if ( capacity < array->size() ) {
            return;
        }

        std::size_t bottom = bottom_.load( std::memory_order_relaxed );
        std::size_t top = top_.load( std::memory_order_acquire );

        auto new_array = array->grow( top, bottom, capacity );
        std::swap( array, new_array );
        array_.store( array, std::memory_order_release );
        delete new_array;
    }

    void push(ItemT * item)
    {
        std::size_t bottom = bottom_.load( std::memory_order_relaxed );
        std::size_t top = top_.load( std::memory_order_acquire );
        auto array = array_.load( std::memory_order_relaxed );

        // Grow array if full
        if (   detail::to_signed( bottom - top ) 
             > detail::to_signed( array->size() - 1 ) ) {
            auto new_array = array->grow( top, bottom );
            std::swap( array, new_array );
            array_.store( array, std::memory_order_release );
            delete new_array;
        }

        array->put( bottom, item );
        std::atomic_thread_fence( std::memory_order_release );
        bottom_.store( bottom + 1, std::memory_order_relaxed );
    }

    ItemT * pop()
    {
        std::size_t bottom = bottom_.load( std::memory_order_relaxed ) - 1;
        auto a = array_.load( std::memory_order_relaxed );
        bottom_.store( bottom, std::memory_order_relaxed );
        std::atomic_thread_fence( std::memory_order_seq_cst );
        std::size_t top = top_.load( std::memory_order_relaxed );

        if ( detail::to_signed( top - bottom ) <= 0 ) {
            // non-empty queue
            if ( top == bottom ) {
                // last item in queue
                if ( !top_.compare_exchange_strong(
                        top,                        // expected
                        top + 1,                    // desired
                        std::memory_order_seq_cst,  // success
                        std::memory_order_relaxed   // fail
                    ) ) {
                    // failed race
                    bottom_.store( bottom + 1, std::memory_order_relaxed );
                    return nullptr;
                }
                bottom_.store( bottom + 1, std::memory_order_relaxed );
            }
            // Only fetch item when we know we got it.
            // This is so we don't release the item when we didn't get it.
            auto item = a->get( bottom );
            return item;
        } else {
            // empty-queue
            bottom_.store( bottom + 1, std::memory_order_relaxed );
            return nullptr;
        }
    }

    ItemT * steal()
    {
        // Loop while CAS fails.
        while ( true ) {
            // Make sure top is read before bottom
            std::size_t top = top_.load( std::memory_order_acquire );
            std::atomic_thread_fence( std::memory_order_seq_cst );
            std::size_t bottom = bottom_.load( std::memory_order_acquire );

            // Exit if deque is empty
            if ( detail::to_signed( bottom - top ) <= 0 ) {
                return nullptr;
            }

            auto array = array_.load( std::memory_order_consume );

            // Attempy to increment top
            if ( top_.compare_exchange_weak(
                    top,                        // expected
                    top + 1,                    // desired
                    std::memory_order_seq_cst,  // success
                    std::memory_order_relaxed   // fail
                ) ) {
                // Only fetch item when we know we got it.
                // This is so we dont release the item when we didn't get it.
                return array->get( top );
            }
        }
    }
};

PROXC_NAMESPACE_END

