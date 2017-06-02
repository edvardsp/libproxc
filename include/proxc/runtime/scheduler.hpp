/*
 * MIT License
 *
 * Copyright (c) [2017] [Edvard S. Pettersen] <edvard.pettersen@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/exceptions.hpp>
#include <proxc/runtime/context.hpp>
#include <proxc/scheduling_policy/policy_base.hpp>
#include <proxc/detail/apply.hpp>
#include <proxc/detail/hook.hpp>
#include <proxc/detail/mpsc_queue.hpp>
#include <proxc/detail/queue.hpp>
#include <proxc/detail/traits.hpp>

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

namespace runtime {

using PolicyT = scheduling_policy::PolicyBase<Context>;
using ClockT = PolicyT::ClockT;
using TimePointT = PolicyT::TimePointT;

class Scheduler
{
private:
    using MtxT = detail::Spinlock;
    using LockT = std::unique_lock< MtxT >;

    // Schwarz counter
    struct Initializer
    {
        thread_local static Scheduler * self_;
        thread_local static std::size_t counter_;
        Initializer();
        ~Initializer();
    };

public:
    using ReadyQueue = detail::queue::ListQueue<
        Context, detail::hook::Ready, & Context::ready_
    >;

    struct CtxSwitchData
    {
        Context *    ctx_{ nullptr };
        LockT *      splk_{ nullptr };

        CtxSwitchData() = default;

        explicit CtxSwitchData( Context * ctx ) noexcept
            : ctx_{ ctx }
        {}

        explicit CtxSwitchData( LockT * splk ) noexcept
            : splk_{ splk }
        {}
    };

private:
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
    using TerminatedQueue = detail::queue::ListQueue<
        Context, detail::hook::Terminated, & Context::terminated_
    >;
    using RemoteQueue = detail::queue::Mpsc< Context >;

    std::unique_ptr< PolicyT >    policy_;

    boost::intrusive_ptr< Context >    main_ctx_{};
    boost::intrusive_ptr< Context >    scheduler_ctx_{};

    Context *    running_{ nullptr };

    MtxT    splk_{};

    WorkQueue          work_queue_{};
    SleepQueue         sleep_queue_{};
    TerminatedQueue    terminated_queue_{};

    RemoteQueue        remote_queue_{};

    alignas(cache_alignment) std::atomic< bool > exit_{ false };

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
    void wait( LockT & ) noexcept;

    bool wait_until( TimePointT const & ) noexcept;
    bool wait_until( TimePointT const &, Context * ) noexcept;
    bool wait_until( TimePointT const &, LockT &, bool lock = false ) noexcept;

    bool sleep_until( TimePointT const &, CtxSwitchData * = nullptr ) noexcept;

    bool alt_wait( Alt *, LockT & ) noexcept;

    void resume( CtxSwitchData * = nullptr ) noexcept;
    void resume( Context *, CtxSwitchData * = nullptr ) noexcept;

    void schedule( Context * ) noexcept;

    void attach( Context * ) noexcept;
    void detach( Context * ) noexcept;
    void commit( Context * ) noexcept;

    void yield() noexcept;
    void join( Context * ) noexcept;

    void print_debug() noexcept;

private:
    void resolve_ctx_switch_data_( CtxSwitchData * ) noexcept;

    void wakeup_sleep_() noexcept;
    void wakeup_waiting_on_( Context * ) noexcept;
    void transition_remote_() noexcept;
    void cleanup_terminated_() noexcept;
    void terminate_( Context * ) noexcept;

    void schedule_local_( Context * ) noexcept;
    void schedule_remote_( Context * ) noexcept;

    // called by main ctx in new threads when multi-core
    void signal_exit() noexcept;
    // actual context switch
    void resume_( Context *, CtxSwitchData * ) noexcept;
    // scheduler context loop
    [[noreturn]]
    void run_( void * );

    template<typename Fn, typename Tpl>
    [[noreturn]]
    static void trampoline( Fn &&, Tpl &&, void * );
};

template<typename Fn, typename ... Args>
boost::intrusive_ptr< Context > Scheduler::make_work( Fn && fn, Args && ... args ) noexcept
{
    static_assert( detail::traits::is_callable< Fn( Args ... ) >::value,
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
    Scheduler::self()->resolve_ctx_switch_data_( data );
    {
        Fn fn{ std::move( fn_ ) };
        Tpl tpl{ std::move( tpl_ ) };
        detail::apply( std::move( fn ), std::move( tpl ) );
    }
    Scheduler::self()->terminate_( Scheduler::running() );
    BOOST_ASSERT_MSG( false, "unreachable");
    throw UnreachableError{};
}

} // namespace runtime
PROXC_NAMESPACE_END

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic pop
#endif

