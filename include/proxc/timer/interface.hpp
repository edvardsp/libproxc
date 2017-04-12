
#pragma once

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN

class Timer
{
public:
    Timer() = default;
    virtual ~Timer() noexcept {}

    // make copyable
    Timer( Timer const & ) = default;
    Timer & operator = ( Timer const & ) = default;

    // make movable
    Timer( Timer && ) = default;
    Timer & operator = ( Timer && ) = default;

    // interface
    virtual bool wait() noexcept = 0;
    virtual bool is_active() noexcept = 0;
    virtual void reset() noexcept = 0;
};

PROXC_NAMESPACE_END

