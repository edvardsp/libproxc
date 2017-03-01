
#pragma once

#include <proxc.hpp>

#include <algorithm>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include <boost/noncopyable.hpp>

#include "work_steal_deque.hpp"

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
private:
    using Popper = typename proxc::WorkStealDeque<Task>::PopperEnd;
    using Stealer = typename proxc::WorkStealDeque<Task>::StealerEnd;
    using Rng = std::default_random_engine;

    class Worker
    {
    public:
        Worker(std::size_t id, Popper popper, std::vector<Stealer> stealer)
            : m_id(id)
            , m_popper(std::move(popper))
            , m_stealer(std::move(stealer))
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
            for (;;) {
                if (auto task = m_popper->pop()) {
                    std::cout << *task << std::endl;
                    continue;
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(500));
        }

    private:
        std::size_t m_id;
        Popper m_popper;
        std::vector<Stealer> m_stealer;

        std::thread m_handle;
    };

public:
    ThreadPool(std::size_t num_workers)
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

            // spawn worker thread with its worker data, and detach
            m_workers.emplace_back(id, std::move(poppers[id]), std::move(victims));
        }
    }

    ~ThreadPool() {}

private:
    std::condition_variable m_cv;
    std::mutex m_mtx;

    std::size_t m_num_workers;
    std::vector<Worker> m_workers;
};

PROXC_NAMESPACE_END

