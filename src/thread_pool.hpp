
#pragma once

#include <proxc.hpp>

#include <algorithm>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <utility>
#include <vector>

#include <boost/noncopyable.hpp>

#include <iostream>

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

    struct WorkerData 
    {
        WorkerData(std::size_t id_, Popper popper_, std::vector<Stealer> victims_)
            : id(id_)
            , popper(std::move(popper_))
            , victims(std::move(victims_))
            //, rand(std::random_device{}())
        {}

        WorkerData(WorkerData&&) = default;

        std::size_t id;
        Popper popper;
        std::vector<Stealer> victims;
        //Rng rand;
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
            auto worker_data = WorkerData{
                id,
                std::move(poppers[id]),
                std::move(victims)
            };
            m_workers.emplace_back([this,&worker_data]() { 
                worker_thread(std::move(worker_data)); 
            });
            //m_workers[id].detach();
        }
    }

    ~ThreadPool() 
    {
        for (auto& worker : m_workers) {
            worker.join();
        }
    }

private:
    std::condition_variable m_cv;
    std::mutex m_mtx;

    std::size_t m_num_workers;
    std::vector<std::thread> m_workers;

    void worker_thread(WorkerData worker_data)
    {
        auto popper = std::move(worker_data.popper);
        for (;;) {
            if (auto task = popper->pop()) {
                std::cout << *task << std::endl;
                continue;
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(500));
    }

    void steal()
    {
        
    }

};

PROXC_NAMESPACE_END

