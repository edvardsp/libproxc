
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
    virtual void restart() noexcept = 0;
    virtual bool expired() const noexcept = 0;
};

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
        
    void restart() noexcept
    {
        time_point_ = ClockT::now() + duration_;
    }

    bool expired() const noexcept
    {
        return ClockT::now() >= time_point_;
    }

};

class Rep final : public Interface
{
private:
    DurationT     duration_;

public:
    template<class Rep, class Period>
    Rep( std::chrono::duration< Rep, Period > const & duration )
        : Interface{ ClockT::now() }
        , duration_{ duration }
    {}

    void restart() noexcept
    {
        time_point_ += duration_;
    }

    bool expired() const noexcept
    {
        return ClockT::now() >= time_point_;
    }
};

// FIXME: better name
class Xx final : public Interface
{
public:
    template<class Clock, class Duration>
    Xx( std::chrono::time_point< Clock, Duration > const & time_point )
        : Interface{ time_point }
    {}

    void restart() noexcept
    { /* do nothing */ }

    bool expired() const noexcept
    {
        return ClockT::now() >= time_point_;
    }
};

} // namespace timer
PROXC_NAMESPACE_END

