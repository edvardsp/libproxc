
#include <iostream>
#include <string>

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>

#include "setup.hpp"

void printer(std::string msg)
{
    std::cout << msg << std::endl;
}
    
void printer2(std::string msg, int x)
{
    std::cout << msg << ": " << x << std::endl;
}
    
void test_scheduler_self()
{
    auto self = proxc::Scheduler::self();
    BOOST_ASSERT(self != nullptr);
    auto main_ctx = self->running();
    BOOST_ASSERT(main_ctx != nullptr);
}

void test_make_work()
{
    auto self = proxc::Scheduler::self();
    self->make_work( & printer, "Hello World!" );
    self->make_work( & printer, "Hello Other World!" );
    self->make_work( & printer2, "Hello This World!", 6 );
}

int main()
{
    test_scheduler_self();
    test_make_work();

    return 0;
}

