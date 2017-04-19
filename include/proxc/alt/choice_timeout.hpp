
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
    using FnT = detail::delegate< void( void ) >;

private:
    std::chrono::steady_clock::time_point    time_point_;

    FnT    fn_;

public:
    ChoiceTimeout( Alt * alt )
        : ChoiceBase{ alt }
        , time_point_{ std::chrono::steady_clock::time_point::min() }
        , fn_{ []{} }
    {}

    template<typename Rep, typename Period>
    ChoiceTimeout( Alt * alt,
                   std::chrono::duration< Rep, Period > const & duration,
                   FnT fn )
        : ChoiceBase{ alt }
        , time_point_{ std::chrono::steady_clock::now() + duration }
        , fn_{ std::move( fn ) }
    {}

    template<typename Clock, typename Dur>
    ChoiceTimeout( Alt * alt,
                   std::chrono::time_point< Clock, Dur > const & time_point,
                   FnT fn )
        : ChoiceBase{ alt }
        , time_point_{ time_point }
        , fn_{ std::move( fn ) }
    {}

    ~ChoiceTimeout() {}

    void enter() noexcept
    { /* FIXME */ }

    void leave() noexcept
    { /* FIXME */ }

    bool is_ready() const noexcept
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
};

} // namespace alt
PROXC_NAMESPACE_END

