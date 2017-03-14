
#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>

#include <boost/context/execution_context_v1.hpp>

PROXC_NAMESPACE_BEGIN

namespace detail {

struct ContextInitializer 
{
    thread_local static Context * running_;
    thread_local static std::size_t counter_;

    ContextInitializer()
    {
        if (counter_++ == 0) {
            // allocating main_context and scheduler next to each other
            //   *cp
            //    |
            //    V
            //    [main_context][scheduler]
            constexpr std::size_t mem_size = sizeof(Context) + sizeof(Scheduler);
            char * cp = new char[mem_size];
            auto main_context = new(cp) Context{ Context::main_context };
            auto scheduler = new(cp + sizeof(Context)) Scheduler{};
            auto scheduler_context = Context::make_scheduler_context(scheduler);

            main_context->set_scheduler(scheduler);
            scheduler->attach_main_context(main_context);
            scheduler->attach_scheduler_context(scheduler_context);
            running_ = main_context;
        }

    }

    ~ContextInitializer()
    {
        if (--counter_ == 0) {
            auto main_context = running_;
            auto scheduler = main_context->get_scheduler();
            // TODO add assert for this actually being main_context?

            scheduler->~Scheduler();
            main_context->~Context();
            char * cp = reinterpret_cast<char *>(main_context);
            delete cp;
        }
    }
};

thread_local Context * ContextInitializer::running_;
thread_local std::size_t ContextInitializer::counter_;

} // namespace detail

const Context::MainContext Context::main_context{};
const Context::WorkContext Context::work_context{};

Context * Context::running() noexcept
{
    //thread_local static boost::context::detail::activation_record_initializer execution_context_initializer__;
    thread_local static detail::ContextInitializer context_initializer__;
    return detail::ContextInitializer::running_;
}

void Context::reset_running() noexcept
{
    detail::ContextInitializer::running_ = nullptr;
}

Context * Context::make_scheduler_context(Scheduler * scheduler)
{
    BOOST_ASSERT(scheduler != nullptr);
    return new Context{
        scheduler_context,
        scheduler
    };

}

Context::Context(MainContext)
    : ctx_{ boost::context::execution_context::current()  }
{
}

Context::Context(SchedulerContext, Scheduler * scheduler)
    : ctx_{ [this,scheduler](void * vp) {
        BOOST_ASSERT(scheduler != nullptr);
        Context * context = static_cast<Context *>(vp);
        if (context != nullptr) {
            running()->schedule(context);
        }
        scheduler->run();
        BOOST_ASSERT_MSG(false, "scheduler context already terminated");
    } }
{
}

Context::~Context()
{
}

Scheduler * Context::get_scheduler() noexcept
{
    return scheduler_;
}

void Context::set_scheduler(Scheduler * scheduler) noexcept
{
    scheduler_ = scheduler;
}

void Context::terminate() noexcept
{

    // cleanup context
    scheduler_->terminate(this);
}

void Context::resume() noexcept
{
    Context * previous = this;
    std::swap(detail::ContextInitializer::running_, previous);
    auto response_ctx = static_cast<Context *>( ctx_(nullptr) );
    if (response_ctx != nullptr) {
        running()->schedule(response_ctx);
    }
    
}

void Context::resume(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    Context * previous = this;
    std::swap(detail::ContextInitializer::running_, previous);
    auto response_ctx = static_cast<Context *>( ctx_(ctx) );
    if (response_ctx != nullptr) {
        running()->schedule(response_ctx);
    }
    
}

template<> detail::hook::Ready & Context::get_hook_() noexcept { return ready_; }
template<> detail::hook::Work &  Context::get_hook_() noexcept { return work_; }
template<> detail::hook::Wait &  Context::get_hook_() noexcept { return wait_; }
template<> detail::hook::Sleep & Context::get_hook_() noexcept { return sleep_; }

PROXC_NAMESPACE_END

