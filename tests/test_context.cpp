
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
    // TODO
}

int main()
{
    test_context();

    return 0;
}
