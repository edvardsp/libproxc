
#pragma once

#include <chrono>

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>
#include <proxc/process.hpp>

PROXC_NAMESPACE_BEGIN
namespace this_proc {

Process::Id get_id() noexcept
{
    return Scheduler::running()->get_id();
}

void yield() noexcept
{
    Scheduler::self()->yield();
}

template<typename Rep, typename Period>
void delay_for( std::chrono::duration< Rep, Period > const & duration ) noexcept
{
    (void)Scheduler::self()->sleep_until(
        std::chrono::steady_clock::now() + duration );
}

template<typename Clock, typename Dur>
void delay_until( std::chrono::time_point< Clock, Dur > const & time_point ) noexcept
{
    (void)Scheduler::self()->sleep_until( time_point );
}

} // namespace this_proc
PROXC_NAMESPACE_END

