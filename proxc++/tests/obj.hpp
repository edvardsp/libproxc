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

#include <algorithm>
#include <iostream>

class Obj
{
private:
    std::size_t value_;

public:
    Obj()
        : value_{}
    {}

    Obj( std::size_t value )
        : value_{ value }
    {}

    Obj( Obj const & ) = default;
    Obj & operator = ( Obj const & ) = default;

    Obj( Obj && other )
    {
        std::swap( value_, other.value_ );
        other.value_ = std::size_t{};
    }
    Obj & operator = ( Obj && other )
    {
        std::swap( value_, other.value_ );
        other.value_ = std::size_t{};
        return *this;
    }

    bool operator == ( Obj const & other )
    { return value_ == other.value_; }

    template<typename CharT, typename TraitsT>
    friend std::basic_ostream< CharT, TraitsT > &
    operator << ( std::basic_ostream< CharT, TraitsT > & out, Obj const & obj )
    { return out << "Obj{" << obj.value_ << "}"; }
};

