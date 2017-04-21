
#pragma once

#include <atomic>
#include <memory>
#include <type_traits>

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace detail {

template<typename ItemT>
class CircularArray
{
private:
    using AtomicT  = typename std::atomic< ItemT >;
    using StorageT = typename std::aligned_storage_t<
        sizeof(AtomicT), cache_alignment
    >::type;

    std::size_t      size_;
    StorageT *    storage_;

public:
    explicit CircularArray( std::size_t size );
    ~CircularArray() noexcept;
    std::size_t size() const noexcept;
    auto get( std::size_t index ) noexcept -> ItemT;
    void put( std::size_t index, ItemT item ) noexcept;
    CircularArray * grow( std::size_t top, std::size_t bottom, std::size_t size );
    CircularArray * grow( std::size_t top, std::size_t bottom );
};

template<typename ItemT>
CircularArray< ItemT >::CircularArray( std::size_t size )
    : size_{ size }
    , storage_{ new StorageT[size_] }
{
    // loop over entire array and instantiate a new AtomicT
    for ( std::size_t i = 0; i < size_; ++i ) {
        void * placement = static_cast< void * >( std::addressof( storage_[i] ) );
        ::new (placement) AtomicT{};
    }
}

template<typename ItemT>
CircularArray< ItemT >::~CircularArray< ItemT >() noexcept
{
    // loop over entire array and call destructor for each element
    for ( std::size_t i = 0; i < size_; ++i ) {
        auto some_atomic = reinterpret_cast< AtomicT * >( std::addressof( storage_[i] ) );
        some_atomic->~AtomicT();
    }
    delete [] storage_;
}

template<typename ItemT>
std::size_t CircularArray< ItemT >::size() const noexcept
{
    return size_;
}

template<typename ItemT>
auto CircularArray< ItemT >::get( std::size_t index ) noexcept
    -> ItemT
{
    auto addr = std::addressof( storage_[index % size_] );
    return std::move( reinterpret_cast< AtomicT * >( addr )
        ->load( std::memory_order_relaxed ) );
}

template<typename ItemT>
void CircularArray< ItemT >::put( std::size_t index, ItemT item ) noexcept
{
    auto addr = std::addressof( storage_[index % size_] );
    reinterpret_cast< AtomicT *>( addr )
        ->store( std::move( item ), std::memory_order_relaxed );
}

template<typename ItemT>
CircularArray< ItemT > *
CircularArray< ItemT >::grow( std::size_t top, std::size_t bottom, std::size_t size )
{
    if ( size < size_ ) {
        return this;
    }

    auto new_array = new CircularArray{ size };
    for ( std::size_t i = top; i != bottom; i++ ) {
        new_array->put( i, std::move( get( i ) ) );
    }
    return new_array;
}

template<typename ItemT>
CircularArray< ItemT > *
CircularArray< ItemT >::grow( std::size_t top, std::size_t bottom )
{
    return grow( top, bottom, size_ * 2 );
}

} // namespace detail
PROXC_NAMESPACE_END

