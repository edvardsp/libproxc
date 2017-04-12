
#pragma once

#include <proxc/config.hpp>

#include <proxc/timer/interface.hpp>

PROXC_NAMESPACE_BEGIN
namespace timer {

class EggTimer : public Timer
{
public:
    EggTimer() = default;

    bool wait() noexcept
    {

        return false;
    }

    bool is_active() noexcept
    {

        return false;
    }

    void reset() noexcept
    {

    }
};

} // namespace timer
PROXC_NAMESPACE_END

