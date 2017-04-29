/*
 * MIT License
 *
 * Copyright (c) [2017] [Edvard S. Pettersen] <edvard.pettersen@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

