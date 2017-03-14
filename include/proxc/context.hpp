
#pragma once

#include <chrono>
#include <tuple>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/detail/hook.hpp>

#include <boost/assert.hpp>
#include <boost/context/execution_context_v1.hpp>
#include <boost/context/detail/apply.hpp>

PROXC_NAMESPACE_BEGIN

class Scheduler;

namespace detail {

// Schwarz counter
struct ContextInitializer;

} // namespace detail

class Context
{
    friend class Scheduler;

public:
    struct MainContext {};
    static const MainContext main_context;

    struct SchedulerContext {};
    static const SchedulerContext scheduler_context;

    struct WorkContext {};
    static const WorkContext work_context;

private:
    using ContextType = boost::context::execution_context;

    ContextType    ctx_;
    Scheduler *    scheduler_;

public:
    // Intrusive hooks
    detail::hook::Ready ready_{};
    detail::hook::Work     work_{};
    detail::hook::Wait     wait_{};
    detail::hook::Sleep    sleep_{};

    std::chrono::steady_clock::time_point time_point_{ (std::chrono::steady_clock::time_point::max)() };
    
    static Context * running() noexcept;
    static void reset_running() noexcept;

    static Context * make_scheduler_context(Scheduler * scheduler);
    template<typename Fn, typename ... Args>
    static Context * make_work_context(Fn && fn, Args && ... args);

    explicit Context(MainContext);
    explicit Context(SchedulerContext, Scheduler *);
    template<typename Fn, typename Tpl>
    Context(WorkContext, Fn && fn, Tpl && tpl);

    ~Context();

    Context(Context const &)             = delete;
    Context & operator=(Context const &) = delete;

    void set_scheduler(Scheduler *) noexcept;
    Scheduler * get_scheduler() noexcept;

    void terminate() noexcept;
    void resume() noexcept;
    void resume(Context *) noexcept;
    void suspend();
    void join();
    void wait_until();
    void schedule(Context *) {}
    void is_context();
    void is_terminated();
    void attach();
    void detach();
    
private:
    template<typename Fn, typename Tpl>
    void entry_func_(Fn && fn, Tpl && tpl, Context * ctx) noexcept;

    // Intrusive hook methods
public:
    template<typename Hook>
    bool is_linked() noexcept;
    template<typename ... Ts>
    void link(boost::intrusive::list< Ts ... > & list) noexcept;
    template<typename ... Ts>
    void link(boost::intrusive::multiset< Ts ... > & set) noexcept;
    template<typename Hook>
    void unlink() noexcept;

private:
    template<typename Hook>
    Hook & get_hook_() noexcept;
};

template<typename Fn, typename Tpl>
Context::Context(WorkContext, Fn && fn, Tpl && tpl)
    : ctx_{ [this, fn  = std::forward<Fn>(fn), 
                   tpl = std::forward<Tpl>(tpl)] (void * vp)
                { entry_func_(std::move(fn), std::move(tpl), static_cast<Context *>(vp)); } }
{
}

template<typename Fn, typename Tpl>
void Context::entry_func_(Fn && fn, Tpl && tpl, Context * ctx) noexcept
{
    {
        typename std::decay<Fn>::type  fn_  = std::forward<Fn>(fn);
        typename std::decay<Tpl>::type tpl_ = std::forward<Tpl>(tpl);
        if (ctx != nullptr) {
            running()->schedule(ctx);
        }
        boost::context::detail::apply(std::move(fn_), std::move(tpl_));
    }
    terminate();
    BOOST_ASSERT_MSG(false, "Context has already terminated");
}

template<typename Fn, typename ... Args>
Context * Context::make_work_context(Fn && fn, Args && ... args)
{
    Context * new_ctx = new Context{ 
        work_context, 
        std::forward<Fn>(fn),
        std::make_tuple( std::forward<Args>(args)... )
    };

    return new_ctx;
}

// Intrusive list methods.
template<typename Hook>
bool Context::is_linked() noexcept
{
    return get_hook_< Hook >().is_linked();
}

template<typename ... Ts>
void Context::link(boost::intrusive::list< Ts ... > & list) noexcept
{
    using Hook = typename boost::intrusive::list< Ts ... >::value_traits::hook_type;
    BOOST_ASSERT( ! is_linked< Hook >() );
    list.push_back( *this);
}

template<typename ... Ts>
void Context::link(boost::intrusive::multiset< Ts ... > & list) noexcept
{
    using Hook = typename boost::intrusive::multiset< Ts ... >::value_traits::hook_type;
    BOOST_ASSERT( ! is_linked< Hook >() );
    list.insert( *this);
}

template<typename Hook>
void Context::unlink() noexcept
{
    BOOST_ASSERT( is_linked< Hook >() );
    get_hook_< Hook >().unlink();
}

PROXC_NAMESPACE_END

