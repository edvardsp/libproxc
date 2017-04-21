
#pragma once

#include <chrono>

#include <proxc/config.hpp>

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

// FIXME: better name
// Single event. Will expire when the supplied time_point has been
// reached. Cannot be reset when expired.
class Xx final : public Interface
{
public:
    template<class Clock, class Duration>
    Xx( std::chrono::time_point< Clock, Duration > const & time_point )
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
PROXC_NAMESPACE_END

