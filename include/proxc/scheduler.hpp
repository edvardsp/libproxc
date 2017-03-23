
#pragma once

#include <functional>
#include <memory>
#include <tuple>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/traits.hpp>
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
public:
    using ReadyQueue = detail::queue::ListQueue< Context, detail::hook::Ready, & Context::ready_ >;

private:
    struct time_point_cmp_
    {
        bool operator()(Context const & left, Context const & right) const noexcept
        { return left.time_point_ < right.time_point_; }
    };

    using WorkQueue  = detail::queue::ListQueue< Context, detail::hook::Work, & Context::work_ >;
    using SleepQueue = detail::queue::SetQueue< Context, detail::hook::Sleep, & Context::sleep_, time_point_cmp_ >;
    using TerminatedQueue = detail::queue::ListQueue< Context, detail::hook::Terminated, & Context::terminated_ >;

    std::unique_ptr< PolicyType >    policy_;

    boost::intrusive_ptr< Context >    main_ctx_{};
    boost::intrusive_ptr< Context >    scheduler_ctx_{};

    Context *    running_{ nullptr };

    WorkQueue          work_queue_{};
    SleepQueue         sleep_queue_{};
    TerminatedQueue    terminated_queue_{};

    bool    exit_{ false };

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
    static boost::intrusive_ptr< Context > make_work(Fn && fn, Args && ... args) noexcept;

    void resume() noexcept;
    void resume(Context *) noexcept;
    [[noreturn]]
    void terminate(Context *) noexcept;
    void schedule(Context *) noexcept;

    void attach(Context *) noexcept;
    void detach(Context *) noexcept;
    void commit(Context *) noexcept;

    void yield() noexcept;
    void join(Context *) noexcept;

    void sleep_until(TimePointType const &) noexcept;

    void wakeup_sleep() noexcept;
    void wakeup_waiting_on(Context *) noexcept;
    void cleanup_terminated() noexcept;

    void print_debug() noexcept;

private:
    // actual context switch
    void resume_(Context *, void * vp = nullptr) noexcept;
    // scheduler context loop
    [[noreturn]]
    void run_(void *) noexcept;

    template<typename Fn, typename Tpl>
    [[noreturn]]
    static void trampoline(Fn &&, Tpl &&, void *) noexcept;
};

template<typename Fn, typename ... Args>
boost::intrusive_ptr< Context > Scheduler::make_work(Fn && fn, Args && ... args) noexcept
{
    static_assert(traits::is_callable< Fn, Args ... >::value, "function is not callable with given arguments");
    auto tpl = std::make_tuple( std::forward< Args >(args) ... );
    auto func = [fn = traits::decay_copy( std::forward< Fn >(fn) ),
                 tpl = std::move(tpl)]
                (void * vp) { trampoline(std::move(fn), std::move(tpl), vp); };
    return boost::intrusive_ptr< Context >{
        new Context{ context::work_type, std::move(func) } };
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

