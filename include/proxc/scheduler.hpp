
#pragma once

#include <functional>
#include <memory>
#include <tuple>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduling_policy/policy_base.hpp>
#include <proxc/detail/hook.hpp>
#include <proxc/detail/queue.hpp>

#include <boost/intrusive_ptr.hpp>

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-private-field"
#endif

PROXC_NAMESPACE_BEGIN

namespace detail {

// Schwarz counter
struct SchedulerInitializer;

} // detail

using PolicyType = scheduling_policy::PolicyBase<Context>;
using ClockType = PolicyType::ClockType;
using TimePointType = PolicyType::TimePointType;

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
    using TerminatedQueue = detail::queue::ListQueue< Context, detail::hook::Terminated, & Context::terminated_ >;

    std::unique_ptr< PolicyType >    policy_;

    boost::intrusive_ptr< Context >    main_ctx_{};
    boost::intrusive_ptr< Context >    scheduler_ctx_{};

    Context *    running_{ nullptr };

    WorkQueue          work_queue_{};
    WaitQueue          wait_queue_{};
    SleepQueue         sleep_queue_{};
    TerminatedQueue    terminated_queue_{};

    bool    exit_{ false };
    char padding_[cacheline_length];

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
    template<typename Fn, typename ... Args>
    void make_work(Fn && fn, Args && ... args) noexcept;

    void resume(Context *) noexcept;
    [[noreturn]]
    void terminate(Context *) noexcept;
    void schedule(Context *) noexcept;

    void attach(Context *) noexcept;
    void detach(Context *) noexcept;

    void join(Context *) noexcept;
    void suspend_running() noexcept;

    void transition_sleep() noexcept;
    void cleanup_terminated() noexcept;

private:
    // scheduler context loop
    [[noreturn]]
    void run_(void *) noexcept;

    template<typename Fn, typename Tpl>
    [[noreturn]]
    static void trampoline(Fn &&, Tpl &&, void *) noexcept;
};

template<typename Fn, typename ... Args>
void Scheduler::make_work(Fn && fn, Args && ... args) noexcept
{
    auto tpl = std::make_tuple( std::forward< Args >(args) ... );
    auto func = [fn = traits::decay_copy( std::forward< Fn >(fn) ),
                 tpl = std::move(tpl)]
                (void * vp) { trampoline(std::move(fn), std::move(tpl), vp); };
    auto ctx = new Context{ context::work_type, std::move(func) };
    attach(ctx);
    schedule(ctx);
}

template<typename Fn, typename Tpl>
void Scheduler::trampoline(Fn && fn_, Tpl && tpl_, void * vp) noexcept
{
    {
        typename std::decay< Fn >::type fn = std::forward< Fn >(fn_);
        typename std::decay< Tpl >::type tpl = std::forward< Tpl >(tpl_);
        // TODO do anything with vp?
        (void)vp;
        boost::context::detail::apply( std::move(fn), std::move(tpl) );
    }
    auto self = Scheduler::self();
    self->terminate(self->running_);
    BOOST_ASSERT_MSG(false, "unreachable: should not return from scheduler trampoline().");
}

PROXC_NAMESPACE_END

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic pop
#endif

