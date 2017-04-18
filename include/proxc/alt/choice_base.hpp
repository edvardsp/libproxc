
#pragma once

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN

// forward declaration
class Alt;
PROXC_NAMESPACE_BEGIN

// forward declaration
class Alt;

namespace alt {

class ChoiceBase
{
public:
    ChoiceBase() {}
    virtual ~ChoiceBase() noexcept {}
public:
    ChoiceBase() {}
    virtual ~ChoiceBase() noexcept {}

    // make non-copyable
    ChoiceBase( ChoiceBase const & ) = delete;
    ChoiceBase & operator = ( ChoiceBase const & ) = delete;

    // make non-movable
    ChoiceBase( ChoiceBase && ) = delete;
    ChoiceBase & operator = ( ChoiceBase && ) = delete;

    // interface
    virtual bool is_ready( Alt * ) const noexcept = 0;
    virtual bool try_complete() noexcept = 0;
    virtual void run_func() const noexcept = 0;

private:
    virtual void enter() noexcept = 0;
    virtual void leave() noexcept = 0;
};

} // namespace alt
PROXC_NAMESPACE_END

    // interface
    virtual bool is_ready( Alt * ) const noexcept = 0;
    virtual bool try_complete() noexcept = 0;
    virtual void run_func() const noexcept = 0;
    virtual bool try_complete() noexcept = 0;
    virtual bool is_ready() const noexcept = 0;
    virtual void run_func() const noexcept = 0;
};

} // namespace alt
PROXC_NAMESPACE_END

