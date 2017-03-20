
#include <functional>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>

#include <boost/bind.hpp>
#include <boost/context/execution_context_v1.hpp>

PROXC_NAMESPACE_BEGIN

// intrusive_ptr friend methods
void intrusive_ptr_add_ref(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    ++ctx->use_count_;
}

void intrusive_ptr_release(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    if (--ctx->use_count_ != 0) { return; }

    // If context new allocated => delete
    delete ctx;
    // if context new placement allocated => call destructor
}

// Context methods
Context::Context(context::MainType)
    : type_{ Type::Main }
    , state_{ State::Running }
    , ctx_{ boost::context::execution_context::current() }
{
}

Context::Context(context::SchedulerType, EntryFn && fn)
    : type_{ Type::Scheduler }
    , state_{ State::Ready }
    , entry_fn_{ std::forward< EntryFn >(fn) }
    , ctx_{ [this](void * vp) { trampoline_(vp); } }
{
}

Context::Context(context::WorkType, EntryFn && fn)
    : type_{ Type::Work }
    , state_{ State::Ready }
    , entry_fn_{ std::forward< EntryFn >(fn) }
    , ctx_{ [this](void * vp) { trampoline_(vp); } }
{
}

Context::~Context() noexcept
{
    BOOST_ASSERT( ! is_linked< hook::Ready >() );
    BOOST_ASSERT( ! is_linked< hook::Wait >() );
    BOOST_ASSERT( ! is_linked< hook::Sleep >() );
}

Context::Id Context::get_id() const noexcept
{
    return Id{ const_cast< Context * >(this) };
}

void * Context::resume(void * vp) noexcept
{
    return ctx_(vp);
}

void Context::terminate() noexcept
{
    BOOST_ASSERT(state_ != State::Terminated);
    state_ = State::Terminated;
}

bool Context::is_type(Type type) const noexcept
{
    return (static_cast<int>(type) & static_cast<int>(type_)) != 0;
}

bool Context::in_state(State state) const noexcept
{
    return state == state_;
}

void Context::trampoline_(void * vp) noexcept
{
    BOOST_ASSERT(entry_fn_ != nullptr);
    entry_fn_(vp);
    BOOST_ASSERT_MSG(false, "unreachable: Context should not return from entry_func_().");
}

template<> detail::hook::Ready & Context::get_hook_() noexcept { return ready_; }
template<> detail::hook::Work &  Context::get_hook_() noexcept { return work_; }
template<> detail::hook::Wait &  Context::get_hook_() noexcept { return wait_; }
template<> detail::hook::Sleep & Context::get_hook_() noexcept { return sleep_; }
template<> detail::hook::Terminated & Context::get_hook_() noexcept { return terminated_; }

PROXC_NAMESPACE_END

