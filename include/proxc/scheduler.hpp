
#pragma once

#include <functional>
#include <memory>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduling_policy/policy_base.hpp>
#include <proxc/detail/hook.hpp>
#include <proxc/detail/queue.hpp>

PROXC_NAMESPACE_BEGIN

namespace detail {

// Schwarz counter
struct SchedulerInitializer;

} // detail

using PolicyType = scheduling_policy::PolicyBase<Context>;

class Scheduler
{
    using TrampolineFn = std::function< void(void) >;

public:
    using ReadyQueue = detail::queue::ListQueue< Context, detail::hook::Ready, & Context::ready_ >;

private:
    struct time_point_comp 
    {
        bool operator()(Context const & left, Context const & right) const noexcept 
        { return left.time_point_ < right.time_point_; }
    };

    using WorkQueue  = detail::queue::ListQueue< Context, detail::hook::Work, & Context::work_ >;
    using WaitQueue  = detail::queue::ListQueue< Context, detail::hook::Wait, & Context::wait_ >;
    using SleepQueue = detail::queue::SetQueue< Context, detail::hook::Sleep, & Context::sleep_, time_point_comp>;

    std::unique_ptr< PolicyType >    policy_;

    Context *    main_ctx_{ nullptr };
    Context *    scheduler_ctx_{ nullptr };

    Context *    running_{ nullptr };

    WorkQueue     work_queue_{};
    SleepQueue    sleep_queue_{};

    //bool    exit_{ false };

public:
    // static methods
    static Scheduler * self() noexcept;
    static Context * running() noexcept;

    // constructor and destructor
    Scheduler();

    ~Scheduler();

    // make non copy-able
    Scheduler(Scheduler const &)             = delete;
    Scheduler & operator=(Scheduler const &) = delete;

    // general methods
    [[noreturn]]
    void run(void *) noexcept;

    void resume(Context *) noexcept;
    [[noreturn]]
    void terminate(Context *) noexcept;
    void schedule(Context *) noexcept;

private:
    [[noreturn]]
    static void trampoline(TrampolineFn) noexcept;
};

PROXC_NAMESPACE_END

