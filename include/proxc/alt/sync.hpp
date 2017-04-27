
#pragma once

#include <atomic>

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

struct Sync
{
    enum class State
    {
        None,
        Offered,
        Accepted,
    };
    std::atomic< State >    state_{ State::None };
    Sync() = default;
};

} // namespace alt
PROXC_NAMESPACE_END

