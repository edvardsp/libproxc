
#include <proxc/config.hpp>

#include <proxc/work_steal_deque.hpp>

#include <algorithm>
#include <chrono>
#include <future>
#include <thread>
#include <tuple>
#include <vector>

#include "setup.hpp"


void test_to_signed()
{
    std::vector<std::ptrdiff_t> ptrs{ 0, 1, 1000, -1, -1000, PTRDIFF_MIN, PTRDIFF_MAX };
    for (auto ptr : ptrs) {
        auto ptr_eq = static_cast<std::size_t>(ptr);
        auto ptr_to = proxc::detail::to_signed(ptr_eq);
        throw_assert_equ(ptr, ptr_to, "signed_to is not idempotent");
    }
}

template<typename T>
class TestObject 
{
private:
    T m_data;

public:
    TestObject()
        : m_data(T{}) {}

    TestObject(T data)
        : m_data(data) {}

    T get() const
    {
        return m_data;
    }

    bool operator==(const TestObject& other) const
    {
        return m_data == other.m_data;
    }
};

using Object = TestObject<std::size_t>;

void test_reserve_capacity()
{
    proxc::WorkStealDeque<Object> deque;

    throw_assert_equ(deque.capacity(), deque.default_capacity, "default capacity is wrong");
    
    deque.reserve(deque.default_capacity - 1);

    throw_assert_equ(deque.capacity(), deque.default_capacity, "capacity should not have changed");

    deque.reserve(deque.default_capacity * 10);

    throw_assert_equ(deque.capacity(), deque.default_capacity * 10, "capacity does not match reserved capacity");
}

void test_deque_growing()
{
    proxc::WorkStealDeque<Object> only_pop;
    proxc::WorkStealDeque<Object> only_steal;

    const auto capacity = proxc::WorkStealDeque<Object>::default_capacity;
    auto num_items = capacity * 2;

    for (std::size_t i = 0; i < capacity; ++i) {
        only_pop.push(new Object{ i });
        only_steal.push(new Object{ i });

        throw_assert_equ(only_pop.capacity(),   capacity, "inconsistent capacity");
        throw_assert_equ(only_steal.capacity(), capacity, "inconsistent capacity");
        throw_assert( ! only_pop.is_empty(),             "should not be empty");
        throw_assert( ! only_steal.is_empty(),           "should not be empty");
    }
    //  both deques contains (cap-1, cap-2, ..., 1, 0)

    for (std::size_t i = capacity; i < num_items; ++i) {
        only_pop.push(new Object{ i });
        only_steal.push(new Object{ i });

        throw_assert_equ(only_pop.capacity(),   2 * capacity, "inconsistent capacity");
        throw_assert_equ(only_steal.capacity(), 2 * capacity, "inconsistent capacity");
        throw_assert( ! only_pop.is_empty(),                 "should not be empty");
        throw_assert( ! only_steal.is_empty(),               "should not be empty");
    }
    //  both deques contains (2*cap-1, 2*cap-2, ..., 1, 0)

    for (std::size_t i = 0; i < num_items; ++i) {
        auto pop_item   = only_pop.pop();
        auto steal_item = only_steal.steal();
        throw_assert(pop_item   != nullptr, "item is nullptr");
        throw_assert(steal_item != nullptr, "item is nullptr");

        auto pop_value   = pop_item->get();
        auto steal_value = steal_item->get();
        throw_assert_equ(pop_value,   2 * capacity - i - 1, "items does not match");
        throw_assert_equ(steal_value, i,                    "items does not match");

        delete pop_item;
        delete steal_item;
    }
}

void test_deque_fill_and_deplete()
{
    proxc::WorkStealDeque<Object> only_pop;
    proxc::WorkStealDeque<Object> only_steal;

    const auto capacity  = proxc::WorkStealDeque<Object>::default_capacity;
    const auto num_items = capacity;

    {
        throw_assert_equ(only_pop.capacity(),   capacity, "inconsistent capacity");
        throw_assert(only_pop.is_empty(),                "should be empty");
        throw_assert_equ(only_steal.capacity(), capacity, "inconsistent capacity");
        throw_assert(only_steal.is_empty(),              "should be empty");
    }

    for (std::size_t i = 0; i < num_items; ++i) {
        only_pop.push(new Object{ i });
        only_steal.push(new Object { i });

        throw_assert_equ(only_pop.capacity(),   capacity, "inconsistent capacity");
        throw_assert( ! only_pop.is_empty(),             "should not be empty");
        throw_assert_equ(only_steal.capacity(), capacity, "inconsistent sizes");
        throw_assert( ! only_steal.is_empty(),           "should not be empty");
    }

    for (std::size_t i = 0; i < num_items; ++i) {
        throw_assert( ! only_pop.is_empty(),             "should not be empty");
        throw_assert( ! only_steal.is_empty(),           "should not be empty");

        auto pop_item   = only_pop.pop();
        auto steal_item = only_steal.steal();
    
        throw_assert_equ(only_pop.capacity(),   capacity, "inconsistent capacity");
        throw_assert_equ(only_steal.capacity(), capacity, "inconsistent capacity");

        throw_assert(pop_item   != nullptr, "item is nullptr");
        throw_assert(steal_item != nullptr, "item is nullptr");

        auto pop_value   = pop_item->get();
        auto steal_value = steal_item->get();

        throw_assert_equ(pop_value,   num_items - i - 1, "items does not match");
        throw_assert_equ(steal_value, i,                 "items does not match");

        delete pop_item;
        delete steal_item;
    }

    {
        throw_assert_equ(only_pop.capacity(),   capacity, "inconsistent capacity");
        throw_assert_equ(only_steal.capacity(), capacity, "inconsistent capacity");
        throw_assert(only_pop.is_empty(),                "should be empty");
        throw_assert(only_steal.is_empty(),              "should be empty");
    }
}

void test_single_deque_single_thread() 
{
    proxc::WorkStealDeque<Object> deque;

    {
        throw_assert(deque.is_empty(),         "should be empty");
        throw_assert(deque.pop()   == nullptr, "empty deque does not return nullptr on pop");
        throw_assert(deque.steal() == nullptr, "empty deque does not return nullptr on steal");
    }

    std::vector<std::size_t> values = { 1337, 10, 42, 0 };
    for (auto value : values) {
        deque.push(new Object{ value });
    }

    for (auto rit = values.rbegin(); rit != values.rend(); ++rit) {
        auto value = *rit;
        throw_assert( ! deque.is_empty(), "should not be empty");
        auto item = deque.pop();                           
        throw_assert(item != nullptr, "non-empty deque returns nullptr on pop");
        auto item_value = item->get();
        throw_assert_equ(item_value, value, "popped item does not match content");
        delete item;
    }

    {
        throw_assert(deque.is_empty(),         "should be empty");
        throw_assert(deque.pop()   == nullptr, "empty deque does not return nullptr on pop");
        throw_assert(deque.steal() == nullptr, "empty deque does not return nullptr on steal");
    }
}

template<typename T>
int worker_func(typename proxc::WorkStealDeque<T> * deque)
{
    int steals = 0;
    while ( ! deque->is_empty()) {
        auto item = deque->steal();
        if (item != nullptr) {
            steals++;
            delete item;
        }
    }
    return steals;
}

void test_single_deque_multiple_threads()
{
    const std::size_t num_workers = std::thread::hardware_concurrency() - 1;
    const std::size_t num_items = 1000000;

    // populate deque
    proxc::WorkStealDeque<Object> deque;
    deque.reserve(num_items);
    for (std::size_t i = 0; i < num_items; i++) {
        deque.push(new Object{ i });
    }

    // start workers
    std::vector<std::future<int>> values;
    for (std::size_t i = 0; i < num_workers; ++i) {
        values.push_back(std::async(
            std::launch::async,
            & worker_func<Object>, & deque));
    }
    
    // pop from deque while it is non-empty
    int n_pops = 0;
    while ( ! deque.is_empty() ) {
        auto item = deque.pop();
        if (item != nullptr) {
            n_pops += 1;
            delete item;
        }
    }

    // get the results from the workers
    int total_steals = n_pops;
    for (auto& value : values) {
        auto n_steals = value.get();
        total_steals += n_steals;
    }
    throw_assert_equ(total_steals, static_cast<int>(num_items), "total steals does not equal total items");
}

int main()
{
    test_to_signed();
    test_reserve_capacity();
    test_deque_growing();
    test_deque_fill_and_deplete();
    test_single_deque_single_thread();
    test_single_deque_multiple_threads();
}

