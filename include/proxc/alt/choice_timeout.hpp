
#pragma once

#include <chrono>

#include <proxc/config.hpp>

#include <proxc/alt/choice_base.hpp>
#include <proxc/detail/delegate.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

class ChoiceTimeout : public ChoiceBase
{
public:
    using FnType = detail::delegate< void( void ) >;

private:
    std::chrono::steady_clock::time_point    time_point_;
    FnType                                   fn_;

public:
    ChoiceTimeout()
        : time_point_{ std::chrono::steady_clock::time_point::min() }
        , fn_{ []{} }
    {
        enter();
    }

    template<typename Rep, typename Period>
    ChoiceTimeout( std::chrono::duration< Rep, Period > const & duration, FnType fn )
        : time_point_{ std::chrono::steady_clock::now() + duration }
        , fn_{ std::move( fn ) }
    {
        enter();
    }

    template<typename Clock, typename Dur>
    ChoiceTimeout( std::chrono::time_point< Clock, Dur > const & time_point, FnType fn )
        : time_point_{ time_point }
        , fn_{ std::move( fn ) }
    {
        enter();
    }

    ~ChoiceTimeout()
    {
        leave();
    }

    bool is_ready( Alt * ) const noexcept
    {
        return ( time_point_ != std::chrono::steady_clock::time_point::min() )
            ? std::chrono::steady_clock::now() >= time_point_
            : false
            ;
    }

    bool try_complete() noexcept
    {
        return true;
    }

    void run_func() const noexcept
    {
        fn_();
    }

    template<typename Clock, typename Dur>
    bool is_less( std::chrono::time_point< Clock, Dur > const & time_point ) const noexcept
    {
        return time_point_ < time_point;
    }

private:
    void enter() noexcept
    {}

    void leave() noexcept
    {}
};

} // namespace alt
PROXC_NAMESPACE_END

