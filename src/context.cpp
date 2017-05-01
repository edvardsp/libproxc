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

#include <iostream>
#include <mutex>

#include <proxc/config.hpp>

#include <proxc/runtime/context.hpp>
#include <proxc/runtime/scheduler.hpp>

#include <proxc/exceptions.hpp>

#include <proxc/detail/spinlock.hpp>

#include <boost/context/execution_context_v1.hpp>

PROXC_NAMESPACE_BEGIN
namespace runtime {

// intrusive_ptr friend methods
void intrusive_ptr_add_ref( Context * ctx ) noexcept
{
    BOOST_ASSERT( ctx != nullptr );
    ++ctx->use_count_;
}

void intrusive_ptr_release( Context * ctx ) noexcept
{
    BOOST_ASSERT( ctx != nullptr );
    if ( --ctx->use_count_ != 0 ) { return; }

    // If context new allocated => delete
    delete ctx;
    // if context new placement allocated => call destructor
}

// Context methods
Context::Context( context::MainType )
    : type_{ Type::Main }
    , ctx_{ boost::context::execution_context::current() }
{
}

Context::Context( context::SchedulerType, EntryFn && fn )
    : type_{ Type::Scheduler }
    , entry_fn_{ std::move( fn ) }
    , ctx_{ [this]( void * vp ) { trampoline_( vp ); } }
{
}

Context::Context( context::WorkType, EntryFn && fn )
    : type_{ Type::Work }
    , entry_fn_{ std::move( fn ) }
    , ctx_{ [this]( void * vp ) { trampoline_( vp ); } }
    , use_count_{ 1 }
{
}

Context::~Context() noexcept
{
    BOOST_ASSERT( ! is_linked< hook::Ready >() );
    BOOST_ASSERT( ! is_linked< hook::Wait >() );
    BOOST_ASSERT( ! is_linked< hook::Sleep >() );
    BOOST_ASSERT( wait_queue_.empty() );
}

Context::Id Context::get_id() const noexcept
{
    return Id{ const_cast< Context * >( this ) };
}

void * Context::resume( void * vp ) noexcept
{
    return ctx_( vp );
}

bool Context::is_type( Type type ) const noexcept
{
    return ( static_cast< int >( type ) & static_cast< int >( type_ ) ) != 0;
}

void Context::terminate() noexcept
{
    terminated_flag_.store( true, std::memory_order_release );
}

bool Context::has_terminated() const noexcept
{
    return terminated_flag_.load( std::memory_order_acquire );
}

void Context::print_debug() noexcept
{
    std::cout << "    Context id : " << get_id() << std::endl;
    std::cout << "      -> type  : ";
    switch ( type_ ) {
    case Type::None:      std::cout << "None"; break;
    case Type::Main:      std::cout << "Main"; break;
    case Type::Scheduler: std::cout << "Scheduler"; break;
    case Type::Work:      std::cout << "Work"; break;
    case Type::Process: case Type::Static:
                          std::cout << "(invalid)"; break;
    }
    std::cout << std::endl;
    std::cout << "      -> Links :" << std::endl;
    if ( is_linked< hook::Work >() )       std::cout << "         | Work" << std::endl;
    if ( is_linked< hook::Ready >() )      std::cout << "         | Ready" << std::endl;
    if ( is_linked< hook::Wait >() )       std::cout << "         | Wait" << std::endl;
    if ( is_linked< hook::Sleep >() )      std::cout << "         | Sleep" << std::endl;
    if ( is_linked< hook::Terminated >() ) std::cout << "         | Terminated" << std::endl;
    if ( mpsc_next_.load( std::memory_order_relaxed ) )
                                           std::cout << "         | RemoteReady" << std::endl;
    std::cout << "      -> wait queue:" << std::endl;
    for ( auto& ctx : wait_queue_ ) {
        std::cout << "         | " << ctx.get_id() << std::endl;
    }
}

void Context::trampoline_( void * vp )
{
    BOOST_ASSERT( entry_fn_ != nullptr );
    entry_fn_( vp );
    BOOST_ASSERT_MSG( false, "unreachable: Context should not return from entry_func_( ).");
    throw UnreachableError{ std::make_error_code( std::errc::state_not_recoverable ), "unreachable" };
}

void Context::wait_for( Context * ctx ) noexcept
{
    BOOST_ASSERT(   ctx != nullptr );
    BOOST_ASSERT( ! ctx->is_linked< hook::Wait >() );

    link( ctx->wait_queue_ );
}

template<> detail::hook::Ready      & Context::get_hook_() noexcept { return ready_; }
template<> detail::hook::Work       & Context::get_hook_() noexcept { return work_; }
template<> detail::hook::Wait       & Context::get_hook_() noexcept { return wait_; }
template<> detail::hook::Sleep      & Context::get_hook_() noexcept { return sleep_; }
template<> detail::hook::Terminated & Context::get_hook_() noexcept { return terminated_; }

} // namespace runtime
PROXC_NAMESPACE_END

