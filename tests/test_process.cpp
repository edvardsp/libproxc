
#include <iostream>
#include <string>

#include <proxc/config.hpp>

#include <proxc/process.hpp>
#include <proxc/scheduler.hpp>

#include "setup.hpp"

using namespace proxc;

int value1 = 0;
std::string value2 = "";

void fn1()
{
    value1 = 42;
}

void fn2( int val1, std::string val2 )
{
    value1 = val1;
    value2 = std::move( val2 );
}

void fn3()
{
    for ( int i = 0; i < 10; ++i ) {
        value1 = i;
        Scheduler::self()->yield();
    }
}

void fn4()
{
    Scheduler::self()->yield();
}

void fn5()
{
    Process p{ fn4 };
    p.join();
}

void test_process_id()
{
    Process p10{ fn1 };
    Process p11{ fn1 };
    Process p2{ fn2, 100, "Hello World!" };
    Process p3{ fn3 };

    throw_assert_equ( p10.get_id(), p10.get_id(), "id should be equal" );
    throw_assert_equ( p11.get_id(), p11.get_id(), "id should be equal" );
    throw_assert_equ( p2.get_id(),  p2.get_id(),  "id should be equal" );
    throw_assert_equ( p3.get_id(),  p3.get_id(),  "id should be equal" );

    throw_assert_neq( p10.get_id(), p11.get_id(), "id should not be equal" );
    throw_assert_neq( p10.get_id(), p2.get_id(),  "id should not be equal" );
    throw_assert_neq( p10.get_id(), p3.get_id(),  "id should not be equal" );
    throw_assert_neq( p11.get_id(), p2.get_id(),  "id should not be equal" );
    throw_assert_neq( p11.get_id(), p3.get_id(),  "id should not be equal" );
    throw_assert_neq( p2.get_id(),  p3.get_id(),  "id should not be equal" );
}

void test_process_join()
{
    {
        value1 = 0;
        Process p{ fn1 };
        p.launch();
        p.join();
        throw_assert_equ( value1, 42, "value1 should be set to new value" );
    }
    {
        value1 = 0;
        value2 = "";
        Process p{ fn2, 123, "123" };
        p.launch();
        p.join();
        throw_assert_equ( value1, 123,   "value1 should be set to new value" );
        throw_assert_equ( value2, "123", "value2 should be set to new value" );
    }
}

int main()
{
    test_process_id();
    test_process_join();

    return 0;
}
