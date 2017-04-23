
#include <algorithm>
#include <mutex>
#include <random>
#include <thread>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/scheduling_policy/work_stealing.hpp>

#include <proxc/detail/num_cpus.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN
namespace scheduling_policy {
namespace detail {

template<>
std::size_t WorkStealing::num_workers_{ 0 };

template<>
std::vector<WorkStealing *> WorkStealing::work_stealers_{};

template<>
Barrier WorkStealing::barrier_{};

template<>
void WorkStealing::init_()
{
    num_workers_ = ::proxc::detail::num_cpus();
    work_stealers_.resize( num_workers_ );
}

template<>
WorkStealing::WorkStealingPolicy()
{
    static std::once_flag flag;
    std::call_once( flag, & WorkStealing::init_ );
    static std::atomic<std::size_t> n_id{ 0 };
    id_ = n_id.fetch_add(1, std::memory_order_acq_rel);
    work_stealers_[id_] = this;
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
    ctx->link( ready_queue_ );
    deque_.push(ctx);
}

template<>
Context * WorkStealing::pick_next() noexcept
{
    auto ctx = deque_.pop();
    if ( ctx != nullptr ) {
        ctx->unlink< hook::Ready >();

    } else {
        static thread_local std::minstd_rand rng;
        std::size_t id{};
        do {
            id = std::uniform_int_distribution< std::size_t >{ 0, num_workers_ - 1 }( rng );
        } while (id == id_);
        /* ctx = work_stealers_[id]->steal(); */
    }
    return ctx;
}

template<>
bool WorkStealing::is_ready() const noexcept
{
    return ! deque_.is_empty();
}

template<>
void WorkStealing::suspend_until(TimePointT const & time_point) noexcept
{
    barrier_.wait_until( time_point );
}

template<>
void WorkStealing::notify() noexcept
{
    barrier_.notify();
}

} // namespace detail
} // namespace scheduling_policy
PROXC_NAMESPACE_END

