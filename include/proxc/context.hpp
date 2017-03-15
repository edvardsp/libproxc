
#pragma once

#include <chrono>
#include <functional>
#include <tuple>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/traits.hpp>
#include <proxc/detail/hook.hpp>

#include <boost/assert.hpp>
#include <boost/context/execution_context_v1.hpp>
#include <boost/context/detail/apply.hpp>

PROXC_NAMESPACE_BEGIN

namespace hook = detail::hook;

class Scheduler;

namespace detail {


} // namespace detail

namespace context {

struct MainType {};
const MainType main_type{};

struct SchedulerType {};
const SchedulerType scheduler_type{};

struct WorkType {};
const WorkType work_type{};

} // namespace context

class Context
{
    friend class Scheduler;

    enum class Type {
        Main,
        Scheduler,
        Work,
    };

    enum class State {
        Ready,
        Wait,
        Sleep,
        Terminated,
    };

    using ContextType = boost::context::execution_context;

    using SchedulerFuncType = std::function< void(void *) >;
    using EntryFuncType     = std::function< void(void) >;

private:

    State    state_{ State::Ready };
    Type     type_;

    ContextType    ctx_;

public:
    // Intrusive hooks
    hook::Ready    ready_{};
    hook::Work     work_{};
    hook::Wait     wait_{};
    hook::Sleep    sleep_{};

    Context *    next_{ nullptr };

    std::chrono::steady_clock::time_point time_point_{ (std::chrono::steady_clock::time_point::max)() };
    
public:
    // static methods
    template<typename Fn, typename ... Args>
    static Context * make_work_context(Fn && fn, Args && ... args);

    // constructors and destructor
    explicit Context(context::MainType);
    explicit Context(context::SchedulerType, SchedulerFuncType &&);
    explicit Context(context::WorkType, EntryFuncType &&);

    ~Context() noexcept;

    // make not copy-able
    Context(Context const &)             = delete;
    Context & operator=(Context const &) = delete;

    // general methods
    [[noreturn]]
    void terminate() noexcept;
    
private:
    [[noreturn]]
    void entry_func_(EntryFuncType, void *) noexcept;

    // Intrusive hook methods
private:
    template<typename Hook>
    Hook & get_hook_() noexcept;

public:
    template<typename Hook>
    bool is_linked() noexcept;
    template<typename ... Ts>
    void link(boost::intrusive::list< Ts ... > & list) noexcept;
    template<typename ... Ts>
    void link(boost::intrusive::multiset< Ts ... > & set) noexcept;
    template<typename Hook>
    void unlink() noexcept;
};

template<typename Fn, typename ... Args>
Context * Context::make_work_context(Fn && fn, Args && ... args)
{
    static_assert(traits::is_callable< Fn, Args ... >::value, "Function is not well defined with given arguments.");
    auto tpl = std::make_tuple( std::forward< Args >(args) ... );
    using Tpl = decltype(tpl);
    auto func = [fn = traits::decay_copy( std::forward< Fn >(fn) ),
                       tpl = std::move(tpl)]
                      () noexcept
        {
            typename std::decay< Fn >::type fn_ = std::forward< Fn >(fn);
            typename std::decay< Tpl >::type tpl_ = std::forward< Tpl >(tpl);
            boost::context::detail::apply(std::move(fn_), std::move(tpl_));
        };
    return new Context{ 
        context::work_type, 
        std::move(func)
    };
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

