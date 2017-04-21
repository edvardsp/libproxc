
#pragma once

#include <chrono>

#include <proxc/config.hpp>

#include <proxc/timer.hpp>
#include <proxc/alt/choice_base.hpp>
#include <proxc/detail/delegate.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

class ChoiceTimeout final : public ChoiceBase
{
public:
    using FnT = proxc::detail::delegate< void( void ) >;

private:
    std::chrono::steady_clock::time_point  time_point_;
    FnT                                    fn_{ []{} };

public:
    ChoiceTimeout( Alt * alt )
        : ChoiceBase{ alt }
    {}

    ChoiceTimeout( Alt * alt,
                   std::chrono::steady_clock::time_point  tp,
                   FnT fn )
        : ChoiceBase{ alt }
        , time_point_{ tp }
        , fn_{ std::move( fn ) }
    {}

    ~ChoiceTimeout() {}

    void enter() noexcept
    { /* FIXME */ }

    void leave() noexcept
    { /* FIXME */ }

    bool is_ready() const noexcept
    {
        return false;
    }

    bool try_complete() noexcept
    {
        return false;
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

