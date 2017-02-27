
#pragma once

#include <proxc.hpp>

#include <memory>
#include <atomic>

#include "aligned_alloc.hpp"

PROXC_NAMESPACE_BEGIN

// Chase-Lev work-steal deque
//
// Dynamic Circular Work-Stealing Deque
// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.170.1097&rep=rep1&type=pdf
//
// Correct and Efﬁcient Work-Stealing for Weak Memory Models
// http://www.di.ens.fr/~zappa/readings/ppopp13.pdf
template<typename T>
class WorkStealDeque
{
    class CircularArray
    {
    public:
        CircularArray(std::size_t n)
            : m_items(n) {}

        std::size_t size() const
        {
            return m_items.size();
        }

        T* get(std::size_t index)
        {
            return m_items[index % (size() - 1)];
        }

        void put(std::size_t index, T* item)
        {
            m_items[index & (size() - 1)] = item;
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

    private:
        proxc::AlignedArray<T*, 64> m_items;
        std::unique_ptr<CircularArray> m_prev;
    };

    static std::ptrdiff_t to_signed(std::size_t x)
    {
        static_assert(static_cast<std::size_t>(PTRDIFF_MAX) + 1 == static_cast<std::size_t>(PTRDIFF_MIN), "Wrong integer wrapping behaviour");
        if (x > static_cast<std::size_t>(PTRDIFF_MAX)) {
                return static_cast<std::ptrdiff_t>(x - static_cast<std::size_t>(PTRDIFF_MIN)) + PTRDIFF_MIN;
        } else {
            return static_cast<std::ptrdiff_t>(x);
        }
    }

public:
    WorkStealDeque()
        : m_array(new CircularArray(32)), m_top(0), m_bottom(0) {}

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

    void push(T item)
    {
        std::size_t bottom = m_bottom.load(std::memory_order_relaxed);
        std::size_t top = m_top.load(std::memory_order_acquire);
        CircularArray *array = m_array.load(std::memory_order_relaxed);

        // Grow array if full
        if (to_signed(bottom - top) >= to_signed(array->size())) {
            array = array->grow(top, bottom);
            m_array.store(array, std::memory_order_release);
        }

        array->put(bottom, item.as_ptr());
        std::atomic_thread_fence(std::memory_order_release);
        m_bottom.store(bottom + 1, std::memory_order_relaxed);
    }

    T pop()
    {
        std::size_t bottom = m_bottom.load(std::memory_order_relaxed);
        std::size_t top = m_top.load(std::memory_order_relaxed);

        // Exit if deque is empty
        if (to_signed(bottom - top) <= 0) {
            return T();
        }

        bottom -= 1;

        // Bottom must be stored before top is read
        m_bottom.store(bottom, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        top = m_top.load(std::memory_order_relaxed);

        // If deque is empty, restore bottom and exit
        if (to_signed(bottom - top) < 0) {
            m_bottom.store(bottom + 1, std::memory_order_relaxed);
            return T();
        }

        // Fetch item from deque
        CircularArray *array = m_array.load(std::memory_order_relaxed);
        T *item = array->get(bottom);

        // If this was the last item, check for races
        if (bottom == top) {
            if (!m_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                m_bottom.store(bottom + 1, std::memory_order_relaxed);
                return T();
            }
            m_bottom.store(bottom + 1, std::memory_order_relaxed);
        }
        return T::from_ptr(item);
    }

    T steal()
    {
        // Loop while CAS fails.
        while (true) {
            // Make sure top is read before bottom
            std::size_t top = m_top.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            std::size_t bottom = m_bottom.load(std::memory_order_acquire);

            // Exit if deque is empty
            if (to_signed(bottom - top) <= 0) {
                return T();
            }

            // Fetch item from deque
            CircularArray *array = m_array.load(std::memory_order_consume);
            T *item = array->get(top);

            // Attempy to increment top
            if (m_top.compare_exchange_weak(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                return T::from_ptr(item);
            }
        }
    }

private:
    std::atomic<CircularArray*> m_array;
    std::atomic<std::size_t> m_top, m_bottom;
};

PROXC_NAMESPACE_END

