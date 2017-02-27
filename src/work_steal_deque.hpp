
#pragma once

#include <proxc.hpp>

#include <atomic>
#include <memory>

#include "aligned_alloc.hpp"

PROXC_NAMESPACE_BEGIN

std::ptrdiff_t to_signed(std::size_t x)
{
    static_assert(static_cast<std::size_t>(PTRDIFF_MAX) + 1 == static_cast<std::size_t>(PTRDIFF_MIN), "Wrong integer wrapping behaviour");
    if (x > static_cast<std::size_t>(PTRDIFF_MAX)) {
            return static_cast<std::ptrdiff_t>(x - static_cast<std::size_t>(PTRDIFF_MIN)) + PTRDIFF_MIN;
    } else {
        return static_cast<std::ptrdiff_t>(x);
    }
}

// Chase-Lev work-steal deque
//
// Dynamic Circular Work-Stealing Deque
// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.170.1097&rep=rep1&type=pdf
//
// Correct and Efﬁcient Work-Stealing for Weak Memory Models
// http://www.di.ens.fr/~zappa/readings/ppopp13.pdf
template<typename T, unsigned int Cap = 32>
class WorkStealDeque
{
public:
    using Ptr = std::unique_ptr<T>;
    static const std::size_t default_capacity = Cap;

private:
    class CircularArray
    {
    private:
        proxc::AlignedArray<Ptr, 64> m_items;
        std::unique_ptr<CircularArray> m_prev;

    public:
        CircularArray(std::size_t n)
            : m_items(n) {}

        std::size_t size() const
        {
            return m_items.size();
        }

        Ptr get(std::size_t index)
        {
            return std::move(m_items[index & (size() - 1)]);
        }

        void put(std::size_t index, Ptr item)
        {
            m_items[index & (size() - 1)] = std::move(item);
        }

        CircularArray* grow(std::size_t top, std::size_t bottom)
        {
            CircularArray *new_array = new CircularArray(size() * 2);
            new_array->m_prev.reset(this);
            for (std::size_t i = top; i != bottom; i++) {
                new_array->put(i, get(i));
            }
            return new_array;
        }
    };

    std::atomic<CircularArray*> m_array;
    std::atomic<std::size_t> m_top, m_bottom;

public:
    WorkStealDeque()
        : m_array(new CircularArray(Cap)), m_top(0), m_bottom(0)
    {
        static_assert(Cap && !(Cap & (Cap - 1)), "Deque capacity must be a power of 2");
    }

    ~WorkStealDeque()
    {
        std::size_t bottom = m_bottom.load(std::memory_order_relaxed);
        std::size_t top = m_top.load(std::memory_order_relaxed);
        CircularArray *array = m_array.load(std::memory_order_relaxed);
        for (std::size_t i = top; i != bottom; ++i) {
            // TODO free all residing tasks
        }
        delete array;
    }

    std::size_t size() const
    {
        std::size_t top = m_top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        std::size_t bottom = m_bottom.load(std::memory_order_acquire);
        if (top <= bottom) {
            return bottom - top;
        } else {
            return capacity() + bottom - top;
        }
    }

    std::size_t capacity() const
    {
        CircularArray *array = m_array.load(std::memory_order_acquire);
        return array->size();
    }

    void push(Ptr item)
    {
        std::size_t bottom = m_bottom.load(std::memory_order_relaxed);
        std::size_t top = m_top.load(std::memory_order_acquire);
        CircularArray *array = m_array.load(std::memory_order_relaxed);

        // Grow array if full
        if (to_signed(bottom - top) > to_signed(array->size() - 1)) {
            array = array->grow(top, bottom);
            m_array.store(array, std::memory_order_release);
        }

        array->put(bottom, std::move(item));
        std::atomic_thread_fence(std::memory_order_release);
        m_bottom.store(bottom + 1, std::memory_order_relaxed);
    }

    Ptr pop()
    {
        std::size_t bottom = m_bottom.load(std::memory_order_relaxed) - 1;
        CircularArray * a = m_array.load(std::memory_order_relaxed);
        m_bottom.store(bottom, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        std::size_t top = m_top.load(std::memory_order_relaxed);

        if (to_signed(top - bottom) <= 0) { 
            // non-empty queue
            if (top == bottom) { 
                // last item in queue
                if (!m_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                    // failed race
                    m_bottom.store(bottom + 1, std::memory_order_relaxed);
                    return Ptr{};
                }
                m_bottom.store(bottom + 1, std::memory_order_relaxed);
            }
            // Only fetch item when we know we got it.
            // This is so we don't release the item when we didn't get it.
            auto item = a->get(bottom);
            return item;
        } else { 
            // empty-queue
            m_bottom.store(bottom + 1, std::memory_order_relaxed);
            return Ptr{};
        }
    }

    Ptr steal()
    {
        // Loop while CAS fails.
        while (true) {
            // Make sure top is read before bottom
            std::size_t top = m_top.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            std::size_t bottom = m_bottom.load(std::memory_order_acquire);

            // Exit if deque is empty
            if (to_signed(bottom - top) <= 0) {
                return Ptr{};
            }

            CircularArray *array = m_array.load(std::memory_order_consume);

            // Attempy to increment top
            if (m_top.compare_exchange_weak(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                // Only fetch item when we know we got it.
                // This is so we dont release the item when we didn't get it.
                auto item = array->get(top);
                return std::move(item);
            }
        }
    }
};

PROXC_NAMESPACE_END

