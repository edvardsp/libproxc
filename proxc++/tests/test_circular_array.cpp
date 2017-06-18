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

#include <algorithm>
#include <functional>
#include <vector>

#include <proxc/config.hpp>
#include <proxc.hpp>
#include <proxc/detail/circular_array.hpp>

#include "setup.hpp"

using namespace proxc;

using ItemType = std::size_t;

void test_size_and_growing()
{
    std::size_t size = 64;

    auto ca = new detail::CircularArray< ItemType >{ size };
    throw_assert_equ( ca->size(), size, "size should match by default" );

    {
        size *= 2;
        // implicit size growth of 2
        auto ca_old = ca->grow( 0, 0 );
        std::swap( ca, ca_old );
        delete ca_old;
        throw_assert_equ( ca->size(), size, "size should match after implicit growth" );
    }


    {
        size *= 10;
        auto ca_old = ca->grow( 0, 0, size );
        std::swap( ca, ca_old );
        delete ca_old;
        throw_assert_equ( ca->size(), size, "size should match after explicit growth" );
    }

    delete ca;
}

void test_putting_and_getting()
{
    std::size_t size = 64;

    auto ca = new detail::CircularArray< ItemType >{ size };

    for ( std::size_t i = 0; i < size; ++i ) {
        auto item = ca->get( ItemType{ i } );
        throw_assert_equ( item, ItemType{}, "item should be default initialized" );
    }

    auto aux = []( bool should, std::size_t index ) -> ItemType
        { return should ? ItemType{ index } : ItemType{}; };
    std::vector< std::function< ItemType ( std::size_t ) > > filters = {
        [&aux]( auto index ) { return aux( index % 2 == 0, index ); },
        [&aux]( auto index ) { return aux( ( index / 2 ) % 2 == 0, index ); },
        [&aux]( auto index ) { return aux( ( index * 2 ) % 3, 42 ); },
        [&aux]( auto index ) { return aux( index, index ); },
    };

    for ( auto& filter : filters ) {
        for ( std::size_t index = 0; index < size; ++index ) {
            ca->put( index, filter( index ) );
        }
        for ( std::size_t index = 0; index < size; ++index ) {
            auto item = ca->get( index );
            throw_assert_equ( item, filter( index ), "item should match" );
        }
    }

    delete ca;
}

void test_putting_and_growing()
{
    std::size_t size = 256;
    std::size_t top = 0;
    std::size_t bottom = 0;
    auto ca = new detail::CircularArray< ItemType >{ size };

    {
        bottom = size;
        for ( std::size_t index = 0; index < size; ++index ) {
            ca->put( index, ItemType{ index } );
        }

        std::size_t new_size = size * 2;
        // implicit growth of * 2
        auto ca_old = ca->grow( top, bottom );
        std::swap( ca, ca_old );
        delete ca_old;

        for ( std::size_t index = 0; index < new_size; ++index ) {
            auto item = ca->get( index );
            auto answer = ( index < size ) ? ItemType{ index } : ItemType{};
            throw_assert_equ( item, answer, "item should match after growth" );
        }
        size = new_size;
    }


    {
        bottom = size;
        for ( std::size_t index = 0; index < size; ++index ) {
            ca->put( index, ItemType{ index } );
        }

        std::size_t new_size = size * 3;
        auto ca_old = ca->grow( top, bottom, new_size );
        std::swap( ca, ca_old );
        delete ca_old;

        for ( std::size_t index = 0; index < new_size; ++index ) {
            auto item = ca->get( index );
            auto answer = ( index < size ) ? ItemType{ index } : ItemType{};
            throw_assert_equ( item, answer, "item should match after growth" );
        }
    }

    delete ca;
}

void test_growing_with_smaller_size()
{
    std::size_t size = 1024;
    auto ca = new detail::CircularArray< ItemType >{ size };

    throw_assert_equ( ca->size(), size, "size should match" );

    auto ca_new = ca->grow( 0, 0, size / 2 );

    throw_assert_equ( ca, ca_new, "should be same pointer" );
    throw_assert_equ( ca->size(), size, "size should be unchanged" );

    delete ca;
}

int main()
{
    test_size_and_growing();
    test_putting_and_getting();
    test_putting_and_growing();
    test_growing_with_smaller_size();

    return 0;
}
