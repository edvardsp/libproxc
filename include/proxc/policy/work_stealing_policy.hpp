
#pragma once

#include <proxc/config.hpp>

#include <proxc/policy/policy_base.hpp>
#include <proxc/work_steal_deque.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

PROXC_NAMESPACE_BEGIN

namespace policy {

template<typename T>
class WorkStealingPolicy : public PolicyBase<T>
{
private:
    static std::size_t                          num_cpus_;
    static std::vector<WorkStealingPolicy *>    work_stealers_;

    std::size_t                 id_;
    proxc::WorkStealDeque<T>    deque_{};
    std::mutex                  mtx_;
    std::condition_variable     cnd_;
    bool                        flag_{ false };

    static void init_();

public:
    WorkStealingPolicy(std::size_t id);

    WorkStealingPolicy(WorkStealingPolicy const &) = delete;
    WorkStealingPolicy(WorkStealingPolicy &&)      = delete;

    WorkStealingPolicy & operator=(WorkStealingPolicy const &) = delete;
    WorkStealingPolicy & operator=(WorkStealingPolicy &&)      = delete;

    T * steal() noexcept;

    void enqueue(T *) noexcept;

    T * pick_next() noexcept; 

    bool is_ready() const noexcept;

    void suspend_until(std::chrono::steady_clock::time_point const &) noexcept;

    void notify() noexcept;
};

} // namespace policy

PROXC_NAMESPACE_END

