
#include <iostream>
#include <string>

#include <proxc/config.hpp>

#include <proxc/context.hpp>

#include "setup.hpp"

using Context = proxc::Context;

void test_context_id()
{
    Context m_ctx{ proxc::context::main_type };
    Context p1_ctx{ proxc::context::scheduler_type, [](void *){} };
    Context p2_ctx{ proxc::context::work_type, [](void *){} };

    throw_assert_equ(m_ctx.get_id(),  m_ctx.get_id(),  "id should be equal");
    throw_assert_equ(p1_ctx.get_id(), p1_ctx.get_id(), "id should be equal");
    throw_assert_equ(p2_ctx.get_id(), p2_ctx.get_id(), "id should be equal");
    throw_assert_neq(m_ctx.get_id(),  p1_ctx.get_id(), "id should not be equal");
    throw_assert_neq(m_ctx.get_id(),  p2_ctx.get_id(), "id should not be equal");
    throw_assert_neq(p1_ctx.get_id(), p2_ctx.get_id(), "id should not be equal");
}

void test_back_and_forth()
{
    const std::string before = "Before context jump";
    const std::string after = "After context jump";
    std::string msg = before;

    Context m_ctx{ proxc::context::main_type };

    Context other_ctx{ proxc::context::work_type,
        [&](void *) {
            msg = after;
            m_ctx.resume();
        } };

    throw_assert_equ(msg.compare(before), 0, "msg is not correct before context jump.");
    other_ctx.resume();
    throw_assert_equ(msg.compare(after), 0, "msg is not correct after context jump.");
}

void test_ping_pong()
{
    std::size_t num_items = 1000;
    std::size_t index = 0;
    std::vector<std::size_t> ints;
    ints.reserve(num_items);

    Context m_ctx{ proxc::context::main_type };

    Context ping_ctx{ proxc::context::work_type,
        [&] (void * vp) {
            Context * pong = static_cast< Context * >(vp);
            m_ctx.resume();
            while (index++ < num_items) {
                ints.push_back(0);
                pong->resume();
            }
            m_ctx.resume();
        }
    };

    Context pong_ctx{ proxc::context::work_type,
        [&] (void * vp) {
            Context * ping = static_cast< Context * >(vp);
            m_ctx.resume();
            while (index++ < num_items) {
                ints.push_back(1);
                ping->resume();
            }
            m_ctx.resume();
        }
    };

    pong_ctx.resume(&ping_ctx);
    ping_ctx.resume(&pong_ctx);
    ping_ctx.resume();

    std::size_t j = 1;
    for (auto i : ints) {
        throw_assert_equ(i, j ^= std::size_t{ 1 }, "items should be equal");
    }
    std::cout << "main: done" << std::endl;
}

int main()
{
    test_context_id();
    test_back_and_forth();
    test_ping_pong();

    return 0;
}
