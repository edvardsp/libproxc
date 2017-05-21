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

#include <chrono>
#include <memory>
#include <type_traits>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/detail/delegate.hpp>

PROXC_NAMESPACE_BEGIN
namespace timer {

class Interface
{
protected:
    using ClockT     = std::chrono::steady_clock;
    using TimePointT = ClockT::time_point;
    using DurationT  = ClockT::duration;

    TimePointT    time_point_;

public:
    Interface( TimePointT const & time_point = TimePointT::max() )
        : time_point_{ time_point }
    {}
    virtual ~Interface() {}

    // make copyable
    Interface( Interface const & )               = default;
    Interface & operator = ( Interface const & ) = default;

    // make movable
    Interface( Interface && )               = default;
    Interface & operator = ( Interface && ) = default;

    // Interface methods
    virtual void reset() noexcept = 0;
    virtual bool expired() noexcept = 0;

    // base methods
    bool operator < ( Interface const & other ) const noexcept
    { return time_point_ < other.time_point_; }

    TimePointT const & get() const noexcept
    { return time_point_; }
};

// Single event, which expires after a given duration. The timer starts
// when the timer is created. After expiration, the timer can be reset.
class Egg final : public Interface
{
private:
    DurationT    duration_;

public:
    template<class Rep, class Period>
    Egg( std::chrono::duration< Rep, Period > const & duration )
        : Interface{ ClockT::now() + duration }
        , duration_{ duration }
    {}

    void reset() noexcept
    {
        time_point_ = ClockT::now() + duration_;
    }

    bool expired() noexcept
    {
        return ClockT::now() >= time_point_;
    }

};

// Tick timer. Will expire continiously with an interval equal
// to the supplied duration. Will reset itself after expiration.
class Repeat final : public Interface
{
private:
    DurationT     duration_;

public:
    template<class Rep, class Period>
    Repeat( std::chrono::duration< Rep, Period > const & duration )
        : Interface{ ClockT::now() + duration }
        , duration_{ duration }
    {}

    void reset() noexcept
    { /* do nothing */ }

    bool expired() noexcept
    {
        bool timeout = ( ClockT::now() >= time_point_ );
        if ( timeout ) {
            time_point_ += duration_;
        }
        return timeout;
    }
};

// Single event. Will expire when the supplied time_point has been
// reached. Cannot be reset when expired.
class Date final : public Interface
{
public:
    template<class Clock, class Duration>
    Date( std::chrono::time_point< Clock, Duration > const & time_point )
        : Interface{ time_point }
    {}

    void reset() noexcept
    { /* do nothing */ }

    bool expired() noexcept
    {
        return ClockT::now() >= time_point_;
    }
};

} // namespace timer

namespace detail {
namespace traits {

template<typename Timer>
struct is_timer
    : std::integral_constant<
        bool,
        std::is_base_of<
            timer::Interface,
            typename std::decay< Timer >::type
        >::value
    >
{};

} // namespace traits
} // namespace detail

PROXC_NAMESPACE_END

