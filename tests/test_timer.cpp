
#include <chrono>
#include <thread>

#include <proxc.hpp>

#include "setup.hpp"

using namespace proxc;

void test_egg_timer()
{
    auto dur = std::chrono::microseconds( 100 );
    timer::Egg egg{ dur };
    throw_assert( ! egg.expired(), "timer should not be expired" );

    for ( std::size_t i = 0; i < 100; ++i ) {
        std::this_thread::sleep_for( dur );
        throw_assert( egg.expired(), "timer should be expired" );

        std::this_thread::sleep_for( dur );
        throw_assert( egg.expired(), "timer should keep being expired" );

        egg.restart();

        throw_assert( ! egg.expired(), "timer should not be expired after restart" );
        throw_assert( ! egg.expired(), "timer should still not be expired" );
    }
}

void test_repeat_timer()
{
    auto dur = std::chrono::microseconds( 1000 );
    timer::Repeat rep{ dur };

    for ( std::size_t i = 0; i < 100; ++i ) {
        throw_assert( ! rep.expired(), "timer should not be expired" );
        throw_assert( ! rep.expired(), "timer should not be expired" );
        std::this_thread::sleep_for( dur );
        throw_assert( rep.expired(), "timer should be expired" );
        throw_assert( ! rep.expired(), "timer set new time_point after expired" );
        throw_assert( ! rep.expired(), "timer set new time_point after expired" );

        std::this_thread::sleep_for( dur );
        throw_assert( rep.expired(), "timer should be expired" );
    }
}

void test_xx_timer()
{

}

int main()
{
    test_egg_timer();
    test_repeat_timer();
    test_xx_timer();

    return 0;
}
