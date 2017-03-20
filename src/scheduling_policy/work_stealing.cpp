
#include <proxc/config.hpp>

#include <proxc/scheduling_policy/work_stealing.hpp>

#include <algorithm>
#include <mutex>
#include <random>
#include <thread>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN

namespace scheduling_policy {

template<>
std::size_t WorkStealing::num_workers_{ 0 };

template<>
std::vector<WorkStealing *> WorkStealing::work_stealers_{};

template<>
void WorkStealing::init_(std::size_t num_workers)
{
    num_workers_ = (num_workers == 0)
        ? std::max(std::thread::hardware_concurrency(), 1u )
        : num_workers;
    work_stealers_.resize(num_workers_);
}

template<>
WorkStealing::WorkStealingPolicy(std::size_t num_workers)
{
    static std::once_flag flag;
    std::call_once(flag, & WorkStealing::init_, num_workers);
    static std::atomic<std::size_t> n_id;
    id_ = n_id.fetch_add(1, std::memory_order_relaxed);
    work_stealers_[id_] = this;
    BOOST_ASSERT((num_workers == 0) || (num_workers == num_workers_));
    BOOST_ASSERT(id_ < num_workers_);
}

template<>
Context * WorkStealing::steal() noexcept
{
    return deque_.steal();
}

template<>
void WorkStealing::reserve(std::size_t capacity) noexcept
{
    deque_.reserve(capacity);
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
    if (ctx == nullptr && num_workers_ > 1) {
        static thread_local std::minstd_rand rng;
        std::size_t id{};
        do {
            id = std::uniform_int_distribution<std::size_t>{0, num_workers_ - 1}(rng);
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
void WorkStealing::suspend_until(TimePointType const & time_point) noexcept
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

} // namespace scheduling_policy

PROXC_NAMESPACE_END

