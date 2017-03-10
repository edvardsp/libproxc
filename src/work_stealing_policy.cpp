
#include <proxc/config.hpp>

#include <proxc/policy/work_stealing_policy.hpp>

#include <algorithm>
#include <mutex>
#include <random>
#include <thread>

PROXC_NAMESPACE_BEGIN

namespace policy {

class Context;

using WorkStealing = WorkStealingPolicy<Context>;

template<>
std::size_t WorkStealing::num_cpus_{ 0 };

template<>
std::vector<WorkStealing *> WorkStealing::work_stealers_{};

template<>
void WorkStealing::init_()
{
    num_cpus_ = std::thread::hardware_concurrency();
    num_cpus_ = std::min(num_cpus_, std::size_t{ 1 });
    work_stealers_.resize(num_cpus_);
}

template<>
WorkStealing::WorkStealingPolicy(std::size_t id)
    : id_{ id }
{
    static std::once_flag flag;
    std::call_once(flag, & WorkStealing::init_);
    work_stealers_[id] = this;
}

template<>
Context * WorkStealing::steal() noexcept
{
    return deque_.steal();
}

template<>
void WorkStealing::enqueue(Context * ctx) noexcept
{
    deque_.push(ctx);
}

template<>
Context * WorkStealing::pick_next() noexcept
{
    auto ctx = deque_.pop();
    if (ctx == nullptr) {
        static thread_local std::minstd_rand rng;
        std::size_t id{};
        do {
            id = std::uniform_int_distribution<std::size_t>{0, num_cpus_ - 1}(rng);
        } while (id == id_);
        ctx = work_stealers_[id]->steal();
    }
    return ctx;
}

template<>
bool WorkStealing::is_ready() const noexcept
{
    return ! deque_.is_empty();
}

template<>
void WorkStealing::suspend_until(std::chrono::steady_clock::time_point const & time_point) noexcept
{
    std::unique_lock<std::mutex> lk{ mtx_ };
    cnd_.wait_until(lk, time_point,
        [this]{ return flag_; });
    flag_ = false;
}

template<>
void WorkStealing::notify() noexcept
{
    std::unique_lock<std::mutex> lk{ mtx_ };
    flag_ = true;
    lk.unlock();
    cnd_.notify_all();
}

} // namespace policy


PROXC_NAMESPACE_END

