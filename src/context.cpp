
#include <iostream>
#include <functional>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/exceptions.hpp>
#include <proxc/scheduler.hpp>

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
    , ctx_{ boost::context::execution_context::current() }
{
}

Context::Context(context::SchedulerType, EntryFn && fn)
    : type_{ Type::Scheduler }
    , entry_fn_{ std::move( fn ) }
    , ctx_{ [this](void * vp) { trampoline_(vp); } }
{
}

Context::Context(context::WorkType, EntryFn && fn)
    : type_{ Type::Work }
    , entry_fn_{ std::move( fn ) }
    , ctx_{ [this](void * vp) { trampoline_(vp); } }
    , use_count_{ 1 }
{
}

Context::~Context() noexcept
{
    // FIXME: should we make sure all contexts are terminated?
    // or just exit when main finishes?
    //BOOST_ASSERT( ! is_linked< hook::Ready >() );
    //BOOST_ASSERT( ! is_linked< hook::Wait >() );
    //BOOST_ASSERT( ! is_linked< hook::Sleep >() );

    //BOOST_ASSERT( wait_queue_.empty() );
}

Context::Id Context::get_id() const noexcept
{
    return Id{ const_cast< Context * >(this) };
}

void * Context::resume( void * vp ) noexcept
{
    return ctx_(vp);
}

bool Context::is_type(Type type) const noexcept
{
    return (static_cast<int>(type) & static_cast<int>(type_)) != 0;
}

bool Context::has_terminated() noexcept
{
    return has_terminated_;
}

void Context::print_debug() noexcept
{
    std::cout << "    Context id : " << get_id() << std::endl;
    std::cout << "      -> type  : ";
    switch (type_) {
    case Type::None:      std::cout << "None"; break;
    case Type::Main:      std::cout << "Main"; break;
    case Type::Scheduler: std::cout << "Scheduler"; break;
    case Type::Work:      std::cout << "Work"; break;
    default:              std::cout << "(invalid)"; break;
    }
    std::cout << std::endl;
    std::cout << "      -> Links :" << std::endl;
    if ( is_linked< hook::Work >() )       std::cout << "         | Work" << std::endl;
    if ( is_linked< hook::Ready >() )      std::cout << "         | Ready" << std::endl;
    if ( is_linked< hook::Wait >() )       std::cout << "         | Wait" << std::endl;
    if ( is_linked< hook::Sleep >() )      std::cout << "         | Sleep" << std::endl;
    if ( is_linked< hook::Terminated >() ) std::cout << "         | Terminated" << std::endl;
    std::cout << "      -> wait queue:" << std::endl;
    for (auto& ctx : wait_queue_) {
        std::cout << "         | " << ctx.get_id() << std::endl;
    }
}

void Context::trampoline_(void * vp)
{
    BOOST_ASSERT(entry_fn_ != nullptr);
    entry_fn_(vp);
    BOOST_ASSERT_MSG(false, "unreachable: Context should not return from entry_func_().");
    throw UnreachableError{ std::make_error_code( std::errc::state_not_recoverable ), "unreachable" };
}

void Context::wait_for(Context * ctx) noexcept
{
    BOOST_ASSERT(   ctx != nullptr );
    BOOST_ASSERT( ! ctx->is_linked< hook::Wait >() );

    link( ctx->wait_queue_ );
}

template<> detail::hook::Ready      & Context::get_hook_() noexcept { return ready_; }
template<> detail::hook::Work       & Context::get_hook_() noexcept { return work_; }
template<> detail::hook::Wait       & Context::get_hook_() noexcept { return wait_; }
template<> detail::hook::Sleep      & Context::get_hook_() noexcept { return sleep_; }
template<> detail::hook::AltSleep   & Context::get_hook_() noexcept { return alt_sleep_; }
template<> detail::hook::Terminated & Context::get_hook_() noexcept { return terminated_; }

PROXC_NAMESPACE_END

