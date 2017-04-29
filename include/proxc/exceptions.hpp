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

#pragma once

#include <string>
#include <system_error>

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN

class UnreachableError : public std::system_error
{
public:
    UnreachableError()
        : std::system_error{ std::make_error_code( std::errc::state_not_recoverable ), "unreachable" }
    {}

    UnreachableError( std::error_code ec )
        : std::system_error{ ec }
    {}

    UnreachableError( std::error_code ec, const char * what_arg )
        : std::system_error{ ec, what_arg }
    {}

    UnreachableError( std::error_code ec, std::string const & what_arg )
        : std::system_error{ ec, what_arg }
    {}

    ~UnreachableError() = default;
};

PROXC_NAMESPACE_END

