
#pragma once

#include <chrono>
#include <functional>
#include <tuple>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/traits.hpp>
#include <proxc/detail/delegate.hpp>
#include <proxc/detail/hook.hpp>
#include <proxc/detail/queue.hpp>

#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/context/execution_context_v1.hpp>

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
public:
    friend class Scheduler;

    enum class Type {
        None      = 1 << 0,
        Main      = 1 << 1,
        Scheduler = 1 << 2,
        Work      = 1 << 3,
        Static    = Main | Scheduler, // cannot migrate from a scheduler
        Dynamic   = Work,             // can migrate between schedulers
        Process   = Main | Dynamic    // contexts which are not a scheduler
    };

    class Id
    {
    private:
        Context *    ptr_{ nullptr };

    public:
        Id(Context * ctx) noexcept : ptr_{ ctx } {}

        bool operator == (Id const & other) const noexcept { return ptr_ == other.ptr_; }
        bool operator != (Id const & other) const noexcept { return ptr_ != other.ptr_; }
        bool operator <= (Id const & other) const noexcept { return ptr_ <= other.ptr_; }
        bool operator >= (Id const & other) const noexcept { return ptr_ >= other.ptr_; }
        bool operator <  (Id const & other) const noexcept { return ptr_ < other.ptr_; }
        bool operator >  (Id const & other) const noexcept { return ptr_ > other.ptr_; }

        explicit operator bool() const noexcept { return ptr_ != nullptr; }
        bool operator ! () const noexcept { return ptr_ == nullptr; }

        template<typename CharT, typename TraitsT>
        friend std::basic_ostream< CharT, TraitsT > &
        operator << (std::basic_ostream< CharT, TraitsT > & os, Id const & id)
        {
            if (id) { return os << id.ptr_; }
            else    { return os << "invalid id"; }
        }
    };

    using ContextType = boost::context::execution_context;
    using TimePointType = std::chrono::steady_clock::time_point;

    using SchedulerFn = std::function< void(void *) >;
    using EntryFn     = detail::delegate< void(void *) >;

private:
    Type    type_;
    bool    has_terminated_{ false };

    EntryFn        entry_fn_{ nullptr };
    ContextType    ctx_;

    // intrusive_ptr friend methods and counter
    friend void intrusive_ptr_add_ref(Context * ctx) noexcept;
    friend void intrusive_ptr_release(Context * ctx) noexcept;
    std::size_t    use_count_{ 0 };

public:
    TimePointType time_point_{ TimePointType::max() };

    // Intrusive hooks
    hook::Ready         ready_{};
    hook::Work          work_{};
    hook::Sleep         sleep_{};
    hook::Terminated    terminated_{};

    // Wait queue must be context specific, as this differs from
    // from context to context
    hook::Wait    wait_{};
    using WaitQueue = detail::queue::ListQueue< Context, detail::hook::Wait, & Context::wait_ >;
    WaitQueue     wait_queue_{};

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
    Id get_id() const noexcept;
    void * resume(void * vp = nullptr) noexcept;

    bool is_type(Type) const noexcept;
    bool has_terminated() noexcept;

    void print_debug() noexcept;

private:
    [[noreturn]]
    void trampoline_(void *);

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
    void wait_for(Context *) noexcept;
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
    BOOST_ASSERT( ! is_linked< typename boost::intrusive::list< Ts ... >::value_traits::hook_type >() );
    list.push_back( *this);
}

template<typename ... Ts>
void Context::link(boost::intrusive::multiset< Ts ... > & list) noexcept
{
    BOOST_ASSERT( ! is_linked< typename boost::intrusive::multiset< Ts ... >::value_traits::hook_type >() );
    list.insert( *this);
}

template<typename Hook>
void Context::unlink() noexcept
{
    BOOST_ASSERT( is_linked< Hook >() );
    get_hook_< Hook >().unlink();
}

PROXC_NAMESPACE_END

