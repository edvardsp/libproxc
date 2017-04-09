
#include <chrono>
#include <iostream>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/parallel.hpp>
#include <proxc/process.hpp>

#include "setup.hpp"

using namespace proxc;

void fn1()
{
    std::cout << "fn1" << std::endl;
}

void fn2()
{
    std::cout << "fn2" << std::endl;
}

void fn3()
{
    std::cout << "fn3" << std::endl;
}

void fn4( int i )
{
    std::cout << "fn4: " << i << std::endl;
}

void fn5( int x, int y )
{
    std::cout << "fn5: " << x << ", " << y << std::endl;
}

int main()
{
    parallel(
        proc( fn1 )
    );
    parallel(
        proc( fn1 ),
        proc( fn2 ),
        proc( fn3 )
    );
}
