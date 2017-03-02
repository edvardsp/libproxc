
#include <proxc.hpp>

#include <string>

#include "setup.hpp"

#include "thread_pool.hpp"

using ThreadPool = proxc::ThreadPool<std::string>;

void dispatcher(ThreadPool::Ptr)
{
}

int main()
{
    ThreadPool tp{8, dispatcher};
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
