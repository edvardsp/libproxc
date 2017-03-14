
#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/scheduling_policy/policy_base.hpp>

PROXC_NAMESPACE_BEGIN

namespace scheduling_policy {
namespace detail {

template<typename T>
class RoundRobinPolicy : public PolicyBase<T> 
{
private:
    Scheduler::ReadyQueue      ready_queue_{};
    std::mutex                 mtx_{};
    std::condition_variable    cnd_{};
    bool                       flag_{ false };

public:
    RoundRobinPolicy() = default;

    RoundRobinPolicy(RoundRobinPolicy const &)             = delete;
    RoundRobinPolicy & operator=(RoundRobinPolicy const &) = delete;

    void enqueue(T *) noexcept;

    T * pick_next() noexcept;

    bool is_ready() const noexcept;

    void suspend_until(std::chrono::steady_clock::time_point const &) noexcept;

    void notify() noexcept;
};

} // namespace detail

using RoundRobin = detail::RoundRobinPolicy<Context>;

} // namespace scheduling_policy

PROXC_NAMESPACE_END

