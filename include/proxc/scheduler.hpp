
#pragma once

#include <memory>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/detail/hook.hpp>
#include <proxc/detail/queue.hpp>

#include <proxc/scheduling_policy/policy_base.hpp>

#include <boost/intrusive/list.hpp>

PROXC_NAMESPACE_BEGIN

namespace intrusive = boost::intrusive;

namespace detail {

// Schwarz counter
struct SchedulerInitializer;

} // detail

class Context;

using PolicyType = scheduling_policy::PolicyBase<Context>;

class Scheduler
{
public:
    using ReadyQueue = detail::queue::ListQueue< Context, detail::hook::Ready, & Context::ready_ >;

    struct time_point_comp 
    {
        bool operator()(Context const & left, Context const & right) const noexcept 
        { return left.time_point_ < right.time_point_; }
    };

    using WorkQueue  = detail::queue::ListQueue< Context, detail::hook::Work, & Context::work_ >;
    using WaitQueue  = detail::queue::ListQueue< Context, detail::hook::Wait, & Context::wait_ >;
    using SleepQueue = detail::queue::SetQueue< Context, detail::hook::Sleep, & Context::sleep_, time_point_comp>;

    std::unique_ptr<PolicyType>    policy_;

    Context *    main_ctx_{ nullptr };
    Context *    scheduler_ctx_{ nullptr };

    Context *    running_{ nullptr };

    WorkQueue    work_queue_{};

    bool    exit_{ false };

public:
    static Scheduler * self() noexcept;
    static Context * running() noexcept;

    Scheduler();
    Scheduler(Context * scheduler_ctx);
    ~Scheduler();

    void run(void *) noexcept;

    void terminate(Context *) noexcept;
    void schedule(Context *) noexcept;
};

PROXC_NAMESPACE_END

