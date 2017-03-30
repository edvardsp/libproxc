
#include <chrono>
#include <thread>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/detail/spsc_queue.hpp>

#include "setup.hpp"

using ItemType = std::size_t;
using Queue = proxc::detail::SpscQueue< ItemType >;

void test_empty_queue()
{
    Queue q{};

    throw_assert( q.is_empty(), "should be empty" );

    ItemType item;
    bool res;

    res = q.dequeue( item );
    throw_assert( q.is_empty(), "should be empty" );
    throw_assert( ! res, "should be false" );
}

void test_enqueue_and_dequeue()
{
    Queue q{};
    const std::size_t num_items = 45;

    throw_assert( q.is_empty(), "should be empty" );

    for ( std::size_t i = 0; i < num_items; ++i ) {
        q.enqueue( ItemType{ i } );
    }

    throw_assert( ! q.is_empty(), "should not be empty" );

    ItemType item;
    std::size_t i = 0;
    while ( ! q.is_empty() ) {
        bool res = q.dequeue( item );
        throw_assert( res, "should be true" );
        throw_assert_equ( item, ItemType { i++ }, "items should match" );
    }
}

void test_caching_of_nodes()
{
    Queue q{};
    const std::size_t num_items = 10;
    const std::size_t rounds    = 10;
    ItemType item;
    bool res;

    for ( std::size_t round = 1; round <= rounds; ++round ) {
        throw_assert( q.is_empty(), "should be empty" );

        for ( std::size_t i = 0; i < num_items * round; ++i ) {
            q.enqueue( ItemType{ i } );
        }

        throw_assert( ! q.is_empty(), "should not be empty" );

        while ( ! q.is_empty() ) {
            res = q.dequeue( item );
            throw_assert( res, "should be true" );
        }

        res = q.dequeue( item );
        throw_assert( ! res, "should be false" );
        throw_assert( q.is_empty(), "should be empty" );
    }
}

void test_put_some_get_some()
{
    Queue q{};
    const std::size_t num_items = 10;
    const std::size_t rounds = 10;
    ItemType item;
    bool res;

    for ( std::size_t round = 1; round <= rounds; ++round ) {
        const std::size_t n = num_items * round;
        for ( std::size_t i = 0; i < n; ++i ) {
            q.enqueue( ItemType { i } );
        }

        throw_assert( ! q.is_empty(), "should not be empty" );

        for ( std::size_t i = 0; i < n / 2; ++i ) {
            res = q.dequeue( item );
            throw_assert( res, "should be true" );
        }

        throw_assert( ! q.is_empty(), "should not be empty" );
    }

    throw_assert( ! q.is_empty(), "should not be empty" );

    while ( ! q.is_empty() ) {
        res = q.dequeue( item );
        throw_assert( res, "should be true" );
    }

    res = q.dequeue( item );
    throw_assert( ! res, "should be false" );
    throw_assert( q.is_empty(), "should be empty" );
}

void pin_thread_affinity( std::thread & t, long id )
{
    cpu_set_t cpuset;
    CPU_ZERO( & cpuset );
    CPU_SET( id, & cpuset );
    pthread_setaffinity_np( t.native_handle(), sizeof( cpu_set_t ), & cpuset );
}

void test_two_threads()
{
    Queue q{};
    const std::size_t num_items = 1000000;
    std::thread producer{
        [&]() {
            std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
            for ( std::size_t i = 0; i < num_items; ++i ) {
                q.enqueue( ItemType{ i } );
            }
        }
    };
    pin_thread_affinity( producer, 0 );
    std::thread consumer{
        [&]() {
            std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
            bool res;
            ItemType item;
            std::size_t i = 0;
            std::size_t tries = 0;
            while ( i < num_items ) {
                tries = 0;
                res = q.dequeue( item );
                if ( res ) {
                    throw_assert_equ( item, ItemType{ i++ }, "items should match" );
                } else {
                    ++tries;
                    throw_assert( tries < 10, "number of tries should not be exceeded" );
                }
            }
        }
    };
    pin_thread_affinity( producer, 1 );

    producer.join();
    consumer.join();
}

int main()
{
    test_empty_queue();
    test_enqueue_and_dequeue();
    test_caching_of_nodes();
    test_put_some_get_some();
    test_two_threads();

    return 0;
}
