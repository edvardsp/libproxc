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

#include <chrono>
#include <tuple>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/detail/delegate.hpp>
#include <proxc/detail/hook.hpp>
#include <proxc/detail/queue.hpp>
#include <proxc/detail/spinlock.hpp>
#include <proxc/detail/traits.hpp>

#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/context/execution_context_v1.hpp>

PROXC_NAMESPACE_BEGIN

namespace hook = detail::hook;

// forward declaration
class Alt;

namespace runtime {

// forward declaration
class Scheduler;

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
        Id( Context * ctx ) noexcept : ptr_{ ctx } {}

        bool operator == ( Id const & other ) const noexcept { return ptr_ == other.ptr_; }
        bool operator != ( Id const & other ) const noexcept { return ptr_ != other.ptr_; }
        bool operator <= ( Id const & other ) const noexcept { return ptr_ <= other.ptr_; }
        bool operator >= ( Id const & other ) const noexcept { return ptr_ >= other.ptr_; }
        bool operator <  ( Id const & other ) const noexcept { return ptr_ < other.ptr_; }
        bool operator >  ( Id const & other ) const noexcept { return ptr_ > other.ptr_; }

        explicit operator bool() const noexcept { return ptr_ != nullptr; }
        bool operator ! () const noexcept { return ptr_ == nullptr; }

        template<typename CharT, typename TraitsT>
        friend std::basic_ostream< CharT, TraitsT > &
        operator << ( std::basic_ostream< CharT, TraitsT > & os, Id const & id )
        {
            if ( id ) { return os << id.ptr_; }
            else      { return os << "invalid id"; }
        }
    };

    using ContextT = boost::context::execution_context;
    using TimePointT = std::chrono::steady_clock::time_point;

    using EntryFn     = detail::delegate< void( void * ) >;

private:
    Type    type_;

    std::atomic< bool >    terminated_flag_{ false };

    EntryFn     entry_fn_{ nullptr };
    ContextT    ctx_;

    Scheduler *    scheduler_{ nullptr };

    // intrusive_ptr friend methods and counter
    friend void intrusive_ptr_add_ref( Context * ctx ) noexcept;
    friend void intrusive_ptr_release( Context * ctx ) noexcept;
    std::size_t    use_count_{ 0 };

public:
    TimePointT     time_point_{ TimePointT::max() };
    Alt *             alt_{ nullptr };

    detail::Spinlock    splk_;

    // Intrusive hooks
    hook::Ready         ready_{};
    hook::Work          work_{};
    hook::Sleep         sleep_{};
    hook::Terminated    terminated_{};

    // Wait queue must be context specific, as this differs from
    // from context to context
    hook::Wait    wait_{};
    using WaitQueue = detail::queue::ListQueue<
        Context, detail::hook::Wait, & Context::wait_
    >;
    WaitQueue     wait_queue_{};

    // Mpsc queue intrusive link
    std::atomic< Context * > mpsc_next_{ nullptr };

public:
    // constructors and destructor
    explicit Context( context::MainType );
    explicit Context( context::SchedulerType, EntryFn && );
    explicit Context( context::WorkType, EntryFn && );

    ~Context() noexcept;

    // make non copy-able
    Context( Context const & )             = delete;
    Context & operator=( Context const & ) = delete;

    // general methods
    Id get_id() const noexcept;
    void * resume( void * = nullptr ) noexcept;

    bool is_type( Type ) const noexcept;

    void terminate() noexcept;
    bool has_terminated() const noexcept;

    void print_debug() noexcept;

private:
    [[noreturn]]
    void trampoline_( void * );

    // Intrusive hook methods
private:
    template<typename Hook>
    Hook & get_hook_() noexcept;

public:
    template<typename Hook>
    bool is_linked() noexcept;
    template<typename ... Ts>
    void link( boost::intrusive::list< Ts ... > & list ) noexcept;
    template<typename ... Ts>
    void link( boost::intrusive::multiset< Ts ... > & set ) noexcept;
    template<typename Hook>
    void unlink() noexcept;
    void wait_for( Context * ) noexcept;
};

// Intrusive list methods.
template<typename Hook>
bool Context::is_linked() noexcept
{
    return get_hook_< Hook >().is_linked();
}

template<typename ... Ts>
void Context::link( boost::intrusive::list< Ts ... > & list ) noexcept
{
    BOOST_ASSERT( ! is_linked< typename boost::intrusive::list< Ts ... >::value_traits::hook_type >() );
    list.push_back( *this);
}

template<typename ... Ts>
void Context::link( boost::intrusive::multiset< Ts ... > & list ) noexcept
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

} // namespace runtime
PROXC_NAMESPACE_END

