
#pragma once

#include <proxc.hpp>

#include <atomic>
#include <memory>
#include <utility>

PROXC_NAMESPACE_BEGIN

namespace detail {

std::ptrdiff_t to_signed(std::size_t x)
{
    static_assert(   static_cast<std::size_t>(PTRDIFF_MAX) + 1 
                  == static_cast<std::size_t>(PTRDIFF_MIN), 
                  "Wrong integer wrapping behaviour");
    if (x > static_cast<std::size_t>(PTRDIFF_MAX)) {
            return static_cast<std::ptrdiff_t>(x - static_cast<std::size_t>(PTRDIFF_MIN)) + PTRDIFF_MIN;
    } else {
        return static_cast<std::ptrdiff_t>(x);
    }
}

} // namespace detail

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
public:
    static constexpr std::size_t default_capacity = 1024;

private:
    class CircularArray
    {
    private:
        using AtomicType  = typename std::atomic<T *>;
        using StorageType = typename std::aligned_storage_t<sizeof(AtomicType), cache_alignment>::type;

        std::size_t      size_;
        StorageType *    storage_;

    public:
        explicit CircularArray(std::size_t size)
            : size_{ size }
            , storage_{ new StorageType[size_] }
        {
            // loop over entire array and instantiate a new AtomicType
            for (std::size_t i = 0; i < size_; ++i) {
                ::new(static_cast<void *>(std::addressof(storage_[i])))
                    AtomicType{ nullptr };
            }
        }

        ~CircularArray() 
        {
            // loop over entire array and call destructor for each element
            for (std::size_t i = 0; i < size_; ++i) {
                auto some_atomic = reinterpret_cast<AtomicType *>(std::addressof(storage_[i]));
                some_atomic->~AtomicType();
            }
            delete [] storage_;
        }

        std::size_t size() const noexcept
        {
            return size_;
        }

        T * get(std::size_t index) noexcept
        {
            return reinterpret_cast<AtomicType *>
                (std::addressof(storage_[index % size_]))
                    ->load(std::memory_order_relaxed);
        }

        void put(std::size_t index, T * item) noexcept
        {
            reinterpret_cast<AtomicType *>
                (std::addressof(storage_[index % size_]))
                    ->store(item, std::memory_order_relaxed);
        }

        CircularArray* grow(std::size_t top, std::size_t bottom)
        {
            auto new_array = new CircularArray{ 2 * size_ };
            for (std::size_t i = top; i != bottom; i++) {
                new_array->put(i, get(i));
            }
            return new_array;
        }
    };

    alignas(cache_alignment) std::atomic<std::size_t>        top_{ 0 };
    alignas(cache_alignment) std::atomic<std::size_t>        bottom_{ 0 };
    alignas(cache_alignment) std::atomic<CircularArray *>    array_;
    char padding_[cacheline_length];

public:
    WorkStealDeque()
        : array_{ new CircularArray{ default_capacity } }
    {}

    ~WorkStealDeque()
    {
        delete array_.load();
    }

    WorkStealDeque(WorkStealDeque const &)            = delete;
    WorkStealDeque & operator=(WorkStealDeque const&) = delete;

    bool is_empty() const noexcept
    {
        std::size_t bottom = bottom_.load(std::memory_order_relaxed);
        std::size_t top = top_.load(std::memory_order_relaxed);
        return bottom <= top;
    }

    std::size_t capacity() const noexcept
    {
        auto array = array_.load(std::memory_order_acquire);
        return array->size();
    }

    void push(T * item)
    {
        std::size_t bottom = bottom_.load(std::memory_order_relaxed);
        std::size_t top = top_.load(std::memory_order_acquire);
        auto array = array_.load(std::memory_order_relaxed);

        // Grow array if full
        if (detail::to_signed(bottom - top) > detail::to_signed(array->size() - 1)) {
            auto new_array = array->grow(top, bottom);
            std::swap(array, new_array);
            array_.store(array, std::memory_order_release);
            delete new_array;
        }

        array->put(bottom, item);
        std::atomic_thread_fence(std::memory_order_release);
        bottom_.store(bottom + 1, std::memory_order_relaxed);
    }

    T * pop()
    {
        std::size_t bottom = bottom_.load(std::memory_order_relaxed) - 1;
        auto a = array_.load(std::memory_order_relaxed);
        bottom_.store(bottom, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        std::size_t top = top_.load(std::memory_order_relaxed);

        if (detail::to_signed(top - bottom) <= 0) { 
            // non-empty queue
            if (top == bottom) { 
                // last item in queue
                if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                    // failed race
                    bottom_.store(bottom + 1, std::memory_order_relaxed);
                    return nullptr;
                }
                bottom_.store(bottom + 1, std::memory_order_relaxed);
            }
            // Only fetch item when we know we got it.
            // This is so we don't release the item when we didn't get it.
            auto item = a->get(bottom);
            return item;
        } else { 
            // empty-queue
            bottom_.store(bottom + 1, std::memory_order_relaxed);
            return nullptr;
        }
    }

    T * steal()
    {
        // Loop while CAS fails.
        while (true) {
            // Make sure top is read before bottom
            std::size_t top = top_.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            std::size_t bottom = bottom_.load(std::memory_order_acquire);

            // Exit if deque is empty
            if (bottom <= top) {
                return nullptr;
            }

            CircularArray *array = array_.load(std::memory_order_consume);

            // Attempy to increment top
            if (top_.compare_exchange_weak(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                // Only fetch item when we know we got it.
                // This is so we dont release the item when we didn't get it.
                return array->get(top);
            }
        }
    }
};

PROXC_NAMESPACE_END

