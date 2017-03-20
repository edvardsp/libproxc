
#pragma once

#include <proxc/config.hpp>

#include <chrono>

PROXC_NAMESPACE_BEGIN

namespace scheduling_policy {

template<typename T>
struct PolicyBase
{
    using ClockType = std::chrono::steady_clock;
    using TimePointType = ClockType::time_point;

    PolicyBase() {}

    PolicyBase(PolicyBase const &) = delete;
    PolicyBase(PolicyBase &&)      = delete;

    PolicyBase & operator=(PolicyBase const &) = delete;
    PolicyBase & operator=(PolicyBase &&)      = delete;

    virtual ~PolicyBase() {}

    virtual void enqueue(T *) noexcept = 0;

    virtual T * pick_next() noexcept = 0;

    virtual bool is_ready() const noexcept = 0;

    virtual void suspend_until(TimePointType const &) noexcept = 0;

    virtual void notify() noexcept = 0;
};

} // namespace scheduling_policy

PROXC_NAMESPACE_END

