
#pragma once

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

class ChoiceBase
{
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
    virtual bool is_ready() noexcept = 0;

    virtual void complete_task() noexcept = 0;
};

} // namespace alt
PROXC_NAMESPACE_END

