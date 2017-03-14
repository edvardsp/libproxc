

#include <cstdlib>

#include <iostream>
#include <vector>

#include <boost/context/all.hpp>

namespace context = boost::context;

using ctx = context::execution_context;

int main()
{
    long n = 50;

    ctx sink(context::execution_context::current());

    ctx fib_source(
        [&sink](void*) mutable {
            long first = 0;
            long second = 1;
            sink(&first);
            sink(&second);
            for(;;) {
                long third = first + second;
                sink(&third);
                first = second;
                second = third;
            }
        }
    );

    for (long i = 0; i < n; ++i) {
        long *value = static_cast<long*>(fib_source());
        if (value == nullptr) {
            throw std::runtime_error("returned pointer is nullptr");
        }
        std::cout << "fib " << i << ": " << *value << std::endl;
    }

    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}

