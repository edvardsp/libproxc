
#pragma once

#include <atomic>
#include <chrono>
#include <random>
#include <thread>

#include <proxc/config.hpp>

#include <proxc/detail/cpu_relax.hpp>

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-private-field"
#endif

PROXC_NAMESPACE_BEGIN

class Spinlock
{
private:
    enum class State {
        Locked,
        Unlocked,
    };

    enum {
        MAX_TESTS = 100,
    };

    alignas(cache_alignment) std::atomic< State > state_{ State::Unlocked };
    char                                          padding_[cacheline_length];

public:
    Spinlock() noexcept = default;

    // make non-copyable
    Spinlock(Spinlock const &) = delete;
    Spinlock & operator = (Spinlock const &) = delete;

    void lock() noexcept {
        std::size_t n_collisions = 0;
        for (;;) {
            std::size_t n_tests = 0;

            while (state_.load( std::memory_order_relaxed ) == State::Locked) {
                if (n_tests < MAX_TESTS) {
                    ++n_tests;
                    cpu_relax();

                } else if (n_tests < MAX_TESTS + 20) {
                    ++n_tests;
                    static constexpr std::chrono::microseconds us0{ 0 };
                    std::this_thread::sleep_for( us0 );

                } else {
                    std::this_thread::yield();
                }
            }

            if (State::Locked == state_.exchange( State::Locked, std::memory_order_acquire )) {
                static thread_local std::minstd_rand rng;
                const std::size_t z = std::uniform_int_distribution< std::size_t >
                    { 0, static_cast< std::size_t >(1) << n_collisions }( rng );
                ++n_collisions;
                for (std::size_t i = 0; i < z; ++i) {
                    cpu_relax();
                }

            } else {
                break;
            }
        }
    }

    void unlock() noexcept {
        state_.store( State::Unlocked, std::memory_order_release );
    }
};

PROXC_NAMESPACE_END

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic pop
#endif

