
#pragma once

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace channel {

enum class OpResult {
    Ok,
    Empty,
    Full,
    Timeout,
    Closed,
    Error,
};

} // namespace channel
PROXC_NAMESPACE_END

