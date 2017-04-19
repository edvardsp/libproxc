
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

enum class AltResult {
    Ok,
    NoEnd,
    Closed,
    SyncFailed,
    SelectFailed,
};

} // namespace channel
PROXC_NAMESPACE_END

