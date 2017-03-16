
#include <iostream>
#include <string>

#include <proxc/config.hpp>

#include <proxc/context.hpp>

#include "setup.hpp"

using Context = proxc::Context;

void test_context()
{
    std::vector<std::size_t> ints;
    std::size_t num_items = 100;
    std::size_t index = 0;

    Context m_ctx{ proxc::context::main_type };

    Context even_ctx{ proxc::context::work_type,
        [&m_ctx,&ints,&index,&num_items] (void * vp) {
            Context * odd = static_cast< Context * >(vp);
            m_ctx.resume();
            while (index < num_items) {
                ints.push_back(2 * index);
                odd->resume();
            }
            m_ctx.resume();
        }
    };

    Context odd_ctx{ proxc::context::work_type,
        [&m_ctx,&ints,&index,&num_items] (void * vp) {
            Context * even = static_cast< Context * >(vp);
            m_ctx.resume();
            while (index < num_items) {
                ints.push_back(2 * index++ + 1);
                even->resume();
            }
            m_ctx.resume();
        }
    };

    odd_ctx.resume(&even_ctx);
    even_ctx.resume(&odd_ctx);
    even_ctx.resume();

    for (std::size_t i = 0; i < num_items; ++i) {
        throw_assert_equ(ints[i], i, "items should be equal");
    }
    std::cout << "main: done" << std::endl;
}

int main()
{
    test_context();

    return 0;
}
