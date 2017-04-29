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
