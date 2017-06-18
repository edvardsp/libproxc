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

#include <limits>
#include <random>

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace detail {

template<typename Integer>
class XorShift
{
private:
    Integer x_{ 123456789 }
          , y_{ 362436069 }
          , z_{ 521288629 }
          , w_{ std::random_device{}() };

    Integer random_() noexcept
    {
        Integer t = x_ ^ (x_ << Integer{ 11 });
        x_ = y_;
        y_ = z_;
        z_ = w_;
        return w_ = w_ ^ (w_ >> Integer{ 19 }) ^ (t ^ (t >> Integer{ 8 }));
    }

public:
    using result_type = Integer;

    XorShift() = default;
    ~XorShift() = default;

    constexpr Integer min() const noexcept
    {
        return std::numeric_limits< Integer >::min();
    }

    constexpr Integer max() const noexcept
    {
        return std::numeric_limits< Integer >::max();
    }

    Integer operator () () noexcept
    {
        return random_();
    }

};

} // namespace detail
PROXC_NAMESPACE_END

