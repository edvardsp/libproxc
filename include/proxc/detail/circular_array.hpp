
#pragma once

#include <atomic>
#include <memory>
#include <type_traits>

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace detail {

template<typename T>
class CircularArray
{
private:
    using ItemType    = T;
    using AtomicType  = typename std::atomic< ItemType >;
    using StorageType = typename std::aligned_storage_t<
        sizeof(AtomicType), cache_alignment
    >::type;

    std::size_t      size_;
    StorageType *    storage_;

public:
    explicit CircularArray( std::size_t size );
    ~CircularArray() noexcept;
    std::size_t size() const noexcept;
    auto get( std::size_t index ) noexcept -> ItemType;
    void put( std::size_t index, ItemType item ) noexcept;
    CircularArray * grow( std::size_t top, std::size_t bottom, std::size_t size );
    CircularArray * grow( std::size_t top, std::size_t bottom );
};

template<typename T>
CircularArray< T >::CircularArray( std::size_t size )
    : size_{ size }
    , storage_{ new StorageType[size_] }
{
    // loop over entire array and instantiate a new AtomicType
    for ( std::size_t i = 0; i < size_; ++i ) {
        void * placement = static_cast< void * >( std::addressof( storage_[i] ) );
        ::new (placement) AtomicType{};
    }
}

template<typename T>
CircularArray< T >::~CircularArray< T >() noexcept
{
    // loop over entire array and call destructor for each element
    for ( std::size_t i = 0; i < size_; ++i ) {
        auto some_atomic = reinterpret_cast< AtomicType * >( std::addressof( storage_[i] ) );
        some_atomic->~AtomicType();
    }
    delete [] storage_;
}

template<typename T>
std::size_t CircularArray< T >::size() const noexcept
{
    return size_;
}

template<typename T>
auto CircularArray< T >::get( std::size_t index ) noexcept
    -> ItemType
{
    auto addr = std::addressof( storage_[index % size_] );
    return std::move( reinterpret_cast< AtomicType * >( addr )
        ->load( std::memory_order_relaxed ) );
}

template<typename T>
void CircularArray< T >::put( std::size_t index, ItemType item ) noexcept
{
    auto addr = std::addressof( storage_[index % size_] );
    reinterpret_cast< AtomicType *>( addr )
        ->store( std::move( item ), std::memory_order_relaxed );
}

template<typename T>
CircularArray< T > *
CircularArray< T >::grow( std::size_t top, std::size_t bottom, std::size_t size )
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

template<typename T>
CircularArray< T > *
CircularArray< T >::grow( std::size_t top, std::size_t bottom )
{
    return grow( top, bottom, size_ * 2 );
}

} // namespace detail
PROXC_NAMESPACE_END

