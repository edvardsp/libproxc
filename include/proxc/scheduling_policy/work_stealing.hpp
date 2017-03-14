
#pragma once

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/work_steal_deque.hpp>
#include <proxc/scheduling_policy/policy_base.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

PROXC_NAMESPACE_BEGIN

namespace scheduling_policy {
namespace detail {

template<typename T>
class WorkStealingPolicy : public PolicyBase<T>
{
private:
    static std::size_t                          num_workers_;
    static std::vector<WorkStealingPolicy *>    work_stealers_;

    std::size_t                 id_;
    proxc::WorkStealDeque<T>    deque_{};
    std::mutex                  mtx_;
    std::condition_variable     cnd_;
    bool                        flag_{ false };

    static void init_(std::size_t num_workers);

public:
    // 0 == num_cores
    WorkStealingPolicy(std::size_t num_workers = 0);

    WorkStealingPolicy(WorkStealingPolicy const &) = delete;
    WorkStealingPolicy(WorkStealingPolicy &&)      = delete;

    WorkStealingPolicy & operator=(WorkStealingPolicy const &) = delete;
    WorkStealingPolicy & operator=(WorkStealingPolicy &&)      = delete;

    // Added work stealing methods
    void reserve(std::size_t capacity) noexcept;

    // Policy base interface methods
    void enqueue(T *) noexcept;

    T * pick_next() noexcept; 

    bool is_ready() const noexcept;

    void suspend_until(std::chrono::steady_clock::time_point const &) noexcept;

    void notify() noexcept;

private:
    T * steal() noexcept;
};

} // namespace detail

using WorkStealing = detail::WorkStealingPolicy<Context>;

} // namespace scheduling_policy

PROXC_NAMESPACE_END

