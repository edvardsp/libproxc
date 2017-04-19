
#pragma once

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>

#include <boost/assert.hpp>
#include <boost/container/small_vector.hpp>

PROXC_NAMESPACE_BEGIN

// forward declaration
class Alt;

namespace alt {

class ChoiceBase
{
private:
    Alt *    alt_;

public:
    ChoiceBase( Alt * ) noexcept;
    virtual ~ChoiceBase() noexcept {}

    // make non-copyable
    ChoiceBase( ChoiceBase const & ) = delete;
    ChoiceBase & operator = ( ChoiceBase const & ) = delete;

    // make non-movable
    ChoiceBase( ChoiceBase && ) = delete;
    ChoiceBase & operator = ( ChoiceBase && ) = delete;

    // base methods
    bool same_alt( Alt * ) const noexcept;
    bool try_select() noexcept;
    void maybe_wakeup() noexcept;

    // interface
    virtual void enter() noexcept = 0;
    virtual void leave() noexcept = 0;
    virtual bool is_ready() const noexcept = 0;

    virtual bool try_complete() noexcept = 0;
    virtual void run_func() const noexcept = 0;
};

} // namespace alt
PROXC_NAMESPACE_END

