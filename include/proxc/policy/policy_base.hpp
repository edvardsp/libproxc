
#pragma once

#include <proxc/config.hpp>

#include <chrono>
#include <memory>

PROXC_NAMESPACE_BEGIN

namespace policy {

template<typename T>
struct PolicyBase
{
    PolicyBase(PolicyBase const &) = delete;
    PolicyBase(PolicyBase &&)      = delete;

    PolicyBase & operator=(PolicyBase const &) = delete;
    PolicyBase & operator=(PolicyBase &&)      = delete;

    virtual ~PolicyBase() = 0;

    virtual T * pick_next() = 0;

    virtual bool is_ready() const = 0;

    virtual bool suspend_until(std::chrono::steady_clock::time_point const & time_point) = 0;

    virtual void notify() = 0;
};

} // namespace policy

PROXC_NAMESPACE_END

