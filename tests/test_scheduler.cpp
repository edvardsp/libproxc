
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

template<typename Arg>
void printer(Arg && arg)
{
    std::cout << arg << std::endl;
}

template<typename Arg, typename ... Args>
void printer(Arg && arg, Args && ... args)
{
    std::cout << arg;
    printer( std::forward< Args >(args)... );
}

void test_scheduler_self()
{
    auto self = Scheduler::self();
    throw_assert( self != nullptr, "scheduler should not be nullptr" );
}

void test_make_work()
{
    auto func = [](std::string msg, ms milli) {
        printer("Printing msg `", msg, "`, now sleeping ", milli.count(), "ms.");
        Scheduler::self()->sleep_until( Clock::now() + milli );
        printer("Printing msg `", msg, "`, after sleeping.");
    };
    auto p0 = Scheduler::make_work( func, "ctx0", ms(0) );
    auto p1 = Scheduler::make_work( func, "ctx1", ms(100) );
    auto p2 = Scheduler::make_work( func, "ctx2", ms(200) );
    auto p3 = Scheduler::make_work( func, "ctx3", ms(300) );
    auto self = Scheduler::self();
    self->commit( p0.get() );
    self->commit( p1.get() );
    self->commit( p2.get() );
    self->commit( p3.get() );
    self->join( p3.get() );
    self->join( p2.get() );
    self->join( p1.get() );
    self->join( p0.get() );
}

int main()
{
    test_scheduler_self();
    test_make_work();

    return 0;
}

