
#include <iostream>
#include <string>

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>

#include "setup.hpp"

void printer(std::string msg)
{
    std::cout << msg << std::endl;
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
}

int main()
{
    test_scheduler_self();
    test_make_work();

    return 0;
}

