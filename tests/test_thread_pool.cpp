
#include <proxc.hpp>

#include <iostream>

#include "setup.hpp"

#include "thread_pool.hpp"

int main()
{
    proxc::ThreadPool<int> tp{4};
}
