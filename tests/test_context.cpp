
#include <iostream>
#include <string>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>

#include "setup.hpp"

using Context = proxc::Context;

void printer(int x, int y) 
{
    SafeCout::print(x, ", ", y);
}

void test_context()
{
     auto main_ctx = Context::running();
     auto scheduler = main_ctx->get_scheduler();
     auto func1_ctx = Context::make_work_context(
         printer,
         42, 1337
     );
     auto func2_ctx = Context::make_work_context(
         [](){ 
            SafeCout::print("no arguments!");
         }
     );
     scheduler->attach_work_context(func1_ctx);
     scheduler->attach_work_context(func2_ctx);

     scheduler->schedule(func1_ctx);
     func2_ctx->resume();
     std::cout << "done" << std::endl;
}

int main()
{
    test_context();

    return 0;
}
