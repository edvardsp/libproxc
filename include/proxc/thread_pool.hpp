
#pragma once

#include <proxc/config.hpp>

#include <proxc/work_steal_deque.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include <boost/noncopyable.hpp>

PROXC_NAMESPACE_BEGIN

std::size_t num_available_cores()
{
    static std::size_t num_cores = []() -> std::size_t {
        std::size_t hw_con = std::thread::hardware_concurrency();
        return (hw_con > 0u) 
            ? hw_con
            : 1u;
    }();
    return num_cores;
}

template<typename Task>
class ThreadPool : boost::noncopyable
{
public:
    using Ptr = typename proxc::WorkStealDeque<Task>::Ptr;

private:
    using Popper = typename proxc::WorkStealDeque<Task>::PopperEnd;
    using Stealer = typename proxc::WorkStealDeque<Task>::StealerEnd;
    using Rng = std::default_random_engine;

    class Worker
    {
    public:
        Worker(ThreadPool& tp, std::size_t id, Popper popper, std::vector<Stealer> victims)
            : m_tp(tp)
            , m_id(id)
            , m_popper(std::move(popper))
            , m_victims(std::move(victims))
            , m_rng(std::default_random_engine{std::random_device{}()})
        {
            m_handle = std::thread([=]() {
                worker_thread();
            });
        }

        Worker(Worker&&) = default;

        ~Worker()
        {
            m_handle.join();
        }

        void worker_thread()
        {
            const auto dispatcher = m_tp.m_dispatcher;
            while (!m_tp.m_stop) {
                if (auto task = m_popper->pop()) {
                    dispatcher(std::move(task));
                    continue;
                }

                if (auto task = try_steal()) {
                    dispatcher(std::move(task));
                    continue;
                }

                {
                    std::unique_lock<std::mutex> lock(m_tp.m_mtx);
                    m_tp.m_cv.wait(lock, [this]{ 
                        return this->m_tp.m_stop || 
                               this->m_tp.m_num_ready_tasks.load(std::memory_order_relaxed) != 0; 
                    });
                    if (m_tp.m_stop) {
                        break;
                    }
                    // subtract number of ready tasks. if more than one ready tasks then notify one more
                    if (m_tp.m_num_ready_tasks.fetch_sub(std::memory_order_relaxed) > 1) {
                        lock.unlock();
                        m_tp.m_cv.notify_one();
                    }
                }
            }
        }

        void push(Ptr&& item)
        {
            m_popper->push(std::forward<Ptr>(item));
        }

    private:
        ThreadPool& m_tp;

        std::size_t m_id;
        Popper m_popper;
        std::vector<Stealer> m_victims;

        std::thread m_handle;
        Rng m_rng;

        Ptr try_steal()
        {
            // always yield once before stealing.
            std::this_thread::yield();

            std::shuffle(m_victims.begin(), m_victims.end(), m_rng);
            for (auto& victim : m_victims) {
                if (auto task = victim->steal()) {
                    return std::move(task);
                }
            }
            return Ptr{};
        }
    };

public:
    ThreadPool(std::size_t num_workers, std::function<void(Ptr)> dispatcher)
        : m_dispatcher(dispatcher)
        , m_stop(false)
        , m_num_ready_tasks(0)
    {
        m_num_workers = std::min(num_available_cores(), num_workers);

        // Create all deques
        std::vector<Popper> poppers;
        std::vector<Stealer> stealers;
        poppers.reserve(m_num_workers);
        stealers.reserve(m_num_workers);
        for (std::size_t id = 0; id < m_num_workers; ++id) {
            auto deque = proxc::WorkStealDeque<Task>::create();
            poppers.push_back(std::move(deque.first));
            stealers.push_back(std::move(deque.second));
        }

        // For each worker
        m_workers.reserve(m_num_workers);
        for (std::size_t id = 0; id < m_num_workers; ++id) {
            // create victim vector
            std::vector<Stealer> victims;
            victims.reserve(m_num_workers - 1);
            for (std::size_t other = 0; other < m_num_workers; ++other) {
                if (other != id) {
                    victims.push_back(stealers[other]);
                }
            }

            // spawn worker thread with its worker data
            m_workers.emplace_back(*this, id, std::move(poppers[id]), std::move(victims));
        }
    }

    ~ThreadPool()
    {
        m_stop = true;
        m_cv.notify_all();
    }

    void schedule(Ptr item)
    {
        std::default_random_engine gen(std::random_device{}());
        auto start = m_workers.begin(), end = m_workers.end();
        std::uniform_int_distribution<> dist(0, std::distance(start, end) - 1);
        std::advance(start, dist(gen));
        start->push(std::move(item));
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            m_num_ready_tasks.fetch_add(std::memory_order_relaxed);
        }
        m_cv.notify_one();
    }

private:
    std::function<void(Ptr)> m_dispatcher;

    std::atomic_bool m_stop;
    std::atomic<std::size_t> m_num_ready_tasks;
    std::condition_variable m_cv;
    std::mutex m_mtx;

    std::size_t m_num_workers;
    std::vector<Worker> m_workers;
};

PROXC_NAMESPACE_END

