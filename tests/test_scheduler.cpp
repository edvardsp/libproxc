
#include <chrono>
#include <iostream>
#include <initializer_list>
#include <string>

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>

#include <boost/intrusive_ptr.hpp>

#include "setup.hpp"

using ms = std::chrono::milliseconds;
using Clock = proxc::ClockType;

using namespace proxc;

void test_scheduler_self()
{
    auto self = Scheduler::self();
    throw_assert( self != nullptr, "scheduler should not be nullptr" );
}

void test_scheduler_running()
{
    auto running = Scheduler::running();
    throw_assert( running != nullptr, "running ctx should not be nullptr" );
    throw_assert( running->is_type( proxc::Context::Type::Main ), "running ctx should be main" );

}

void test_sleep_and_join()
{
    std::vector< std::size_t > answer = { 0, 100, 200, 300, 400, 500 };
    std::vector< std::size_t > numbers;
    std::vector< boost::intrusive_ptr< Context > > procs;

    auto func = [&numbers](std::size_t milli) {
        Scheduler::self()->sleep_until( Clock::now() + ms(milli) );
        numbers.push_back(milli);
    };

    for (auto it = answer.rbegin(); it != answer.rend(); ++it) {
        auto p = Scheduler::make_work( func, *it );
        Scheduler::self()->commit( p.get() );
        procs.push_back( std::move( p ) );
    }

    for (auto& p : procs) {
        Scheduler::self()->join( p.get() );
    }

    throw_assert_equ( numbers.size(), answer.size(), "both vectors should be of equal size" );
    for(std::size_t i = 0; i < numbers.size(); ++i) {
        throw_assert_equ( numbers[i], answer[i], "numbers should be equal" );
    }
}

int main()
{
    test_scheduler_self();
    test_scheduler_running();
    test_sleep_and_join();

    return 0;
}

