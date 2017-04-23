
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/exceptions.hpp>
#include <proxc/traits.hpp>
#include <proxc/scheduling_policy/policy_base.hpp>
#include <proxc/detail/apply.hpp>
#include <proxc/detail/hook.hpp>
#include <proxc/detail/queue.hpp>

#include <boost/intrusive_ptr.hpp>

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-private-field"
#endif

PROXC_NAMESPACE_BEGIN

// forward declarations
class Alt;

namespace alt {

class ChoiceBase;

} // namespace alt

namespace detail {

// Schwarz counter
struct SchedulerInitializer;

} // detail

using PolicyT = scheduling_policy::PolicyBase<Context>;
using ClockT = PolicyT::ClockT;
using TimePointT = PolicyT::TimePointT;

class Scheduler
{
public:
    using ReadyQueue = detail::queue::ListQueue< Context, detail::hook::Ready, & Context::ready_ >;

    struct CtxSwitchData
    {
        Context *                         ctx_{ nullptr };
        std::unique_lock< Spinlock > *    splk_{ nullptr };

        CtxSwitchData() = default;

        explicit CtxSwitchData( Context * ctx ) noexcept
            : ctx_{ ctx }
        {}

        explicit CtxSwitchData( std::unique_lock< Spinlock > * splk ) noexcept
            : splk_{ splk }
        {}
    };

private:
    friend struct detail::SchedulerInitializer;

    struct time_point_cmp_
    {
        bool operator()( Context const & left, Context const & right ) const noexcept
        { return left.time_point_ < right.time_point_; }
    };

    using WorkQueue = detail::queue::ListQueue<
        Context, detail::hook::Work, & Context::work_
    >;
    using SleepQueue = detail::queue::SetQueue<
        Context, detail::hook::Sleep, & Context::sleep_, time_point_cmp_
    >;
    // FIXME: correct cmp functor?
    using AltSleepQueue = detail::queue::SetQueue<
        Context, detail::hook::AltSleep, & Context::alt_sleep_, time_point_cmp_
    >;
    using TerminatedQueue = detail::queue::ListQueue<
        Context, detail::hook::Terminated, & Context::terminated_
    >;

    std::unique_ptr< PolicyT >    policy_;

    boost::intrusive_ptr< Context >    main_ctx_{};
    boost::intrusive_ptr< Context >    scheduler_ctx_{};

    Context *    running_{ nullptr };

    WorkQueue          work_queue_{};
    SleepQueue         sleep_queue_{};
    AltSleepQueue      alt_sleep_queue_{};
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
    Scheduler( Scheduler const & )             = delete;
    Scheduler & operator=( Scheduler const & ) = delete;


    // general methods
    template<typename Fn, typename ... Args>
    static boost::intrusive_ptr< Context > make_work( Fn && fn, Args && ... args ) noexcept;

    void wait() noexcept;
    void wait( Context * ) noexcept;
    void wait( std::unique_lock< Spinlock > &, bool lock = false ) noexcept;

    bool wait_until( TimePointT const & ) noexcept;
    bool wait_until( TimePointT const &, Context * ) noexcept;
    bool wait_until( TimePointT const &, std::unique_lock< Spinlock > &, bool lock = false ) noexcept;

    void alt_wait( Alt *, std::unique_lock< Spinlock > &, bool lock = false ) noexcept;

    void resume( CtxSwitchData * = nullptr ) noexcept;
    void resume( Context *, CtxSwitchData * = nullptr ) noexcept;
    void terminate( Context * ) noexcept;
    void schedule( Context * ) noexcept;

    void attach( Context * ) noexcept;
    void detach( Context * ) noexcept;
    void commit( Context * ) noexcept;

    void yield() noexcept;
    void join( Context * ) noexcept;

    bool sleep_until( TimePointT const &, CtxSwitchData * = nullptr ) noexcept;

    void wakeup_sleep() noexcept;
    void wakeup_waiting_on( Context * ) noexcept;
    void cleanup_terminated() noexcept;

    void print_debug() noexcept;

private:
    void resolve_ctx_switch_data( CtxSwitchData * ) noexcept;
    // called by main ctx in new threads when multi-core
    void join_scheduler() noexcept;
    // actual context switch
    void resume_( Context *, CtxSwitchData * ) noexcept;
    // scheduler context loop
    void run_( void * ) noexcept;

    template<typename Fn, typename Tpl>
    [[noreturn]]
    static void trampoline( Fn &&, Tpl &&, void * );
};

template<typename Fn, typename ... Args>
boost::intrusive_ptr< Context > Scheduler::make_work( Fn && fn, Args && ... args ) noexcept
{
    static_assert( traits::is_callable< Fn( Args ... ) >::value,
        "function is not callable with given arguments" );
    auto func = [fn = std::move( fn ),
                 tpl = std::make_tuple( std::forward< Args >( args ) ... )]
                ( void * vp ) mutable
                { trampoline( std::move( fn ), std::move( tpl ), vp ); };
    return boost::intrusive_ptr< Context >{
        new Context{ context::work_type, std::move( func ) } };
}

template<typename Fn, typename Tpl>
void Scheduler::trampoline( Fn && fn_, Tpl && tpl_, void * vp )
{
    CtxSwitchData * data = static_cast< CtxSwitchData * >( vp );
    Scheduler::self()->resolve_ctx_switch_data( data );
    {
        Fn fn{ std::move( fn_ ) };
        Tpl tpl{ std::move( tpl_ ) };
        detail::apply( std::move( fn ), std::move( tpl ) );
    }
    auto self = Scheduler::self();
    self->terminate( self->running_ );
    BOOST_ASSERT_MSG( false, "unreachable: should not return from scheduler trampoline( ).");
    throw UnreachableError{};
}

PROXC_NAMESPACE_END

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic pop
#endif

