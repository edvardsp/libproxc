/*
 * MIT License
 *
 * Copyright (c) [2017] [Edvard S. Pettersen] <edvard.pettersen@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>
#include <string>

#include <proxc/config.hpp>

#include <proxc/context.hpp>

#include "setup.hpp"

using namespace proxc;

void test_context_id()
{
    Context m_ctx{ context::main_type };
    Context s_ctx{ context::scheduler_type, [](void *){} };
    Context w_ctx{ context::work_type, [](void *){} };

    throw_assert_equ(m_ctx.get_id(), m_ctx.get_id(),  "id should be equal");
    throw_assert_equ(s_ctx.get_id(), s_ctx.get_id(), "id should be equal");
    throw_assert_equ(w_ctx.get_id(), w_ctx.get_id(), "id should be equal");
    throw_assert_neq(m_ctx.get_id(), s_ctx.get_id(), "id should not be equal");
    throw_assert_neq(m_ctx.get_id(), w_ctx.get_id(), "id should not be equal");
    throw_assert_neq(s_ctx.get_id(), w_ctx.get_id(), "id should not be equal");
}

void test_back_and_forth()
{
    const std::string before = "Before context jump";
    const std::string after = "After context jump";
    std::string msg = before;

    Context m_ctx{ context::main_type };

    Context other_ctx{ context::work_type,
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

    Context m_ctx{ context::main_type };

    Context ping_ctx{ context::work_type,
        [&] (void * vp) {
            auto pong = static_cast< Context * >( vp );
            m_ctx.resume();
            while ( index++ < num_items ) {
                ints.push_back( 0 );
                pong->resume();
            }
            m_ctx.resume();
        }
    };

    Context pong_ctx{ context::work_type,
        [&] (void * vp) {
            auto ping = static_cast< Context * >( vp );
            m_ctx.resume();
            while ( index++ < num_items ) {
                ints.push_back( 1 );
                ping->resume();
            }
            m_ctx.resume();
        }
    };
    
    pong_ctx.resume( & ping_ctx );
    ping_ctx.resume( & pong_ctx );
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
