
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
        Running,
        Ready,
        Wait,
        Sleep,
        Terminated,
    };

    using ContextType = boost::context::execution_context;

    using SchedulerFn = std::function< void(void *) >;
    using EntryFn     = std::function< void(void *) >;

private:

    Type     type_;
    State    state_;

    EntryFn        entry_fn_{ nullptr };
    ContextType    ctx_;

    Context *    next_{ nullptr };

public:
    // Intrusive hooks
    hook::Ready    ready_{};
    hook::Work     work_{};
    hook::Wait     wait_{};
    hook::Sleep    sleep_{};

    std::chrono::steady_clock::time_point time_point_{ (std::chrono::steady_clock::time_point::max)() };
    
public:
    // constructors and destructor
    explicit Context(context::MainType);
    explicit Context(context::SchedulerType, EntryFn &&);
    explicit Context(context::WorkType, EntryFn &&);

    ~Context() noexcept;

    // make non copy-able
    Context(Context const &)             = delete;
    Context & operator=(Context const &) = delete;

    // general methods
    void * resume(void * vp = nullptr) noexcept;
    [[noreturn]]
    void terminate() noexcept;
    
private:
    /* [[noreturn]] */
    /* static void entry_func_(SchedulerFn, void *) noexcept; */
    [[noreturn]]
    void trampoline_(void *) noexcept;

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

