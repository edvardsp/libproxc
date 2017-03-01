
#include <proxc.hpp>

#include <algorithm>
#include <chrono>
#include <future>
#include <thread>
#include <tuple>
#include <vector>

#include "setup.hpp"

#include "work_steal_deque.hpp"

void test_to_signed()
{
    std::vector<std::ptrdiff_t> ptrs{ 0, 1, 1000, -1, -1000, PTRDIFF_MIN, PTRDIFF_MAX };
    for (auto ptr : ptrs) {
        auto ptr_eq = static_cast<std::size_t>(ptr);
        auto ptr_to = proxc::to_signed(ptr_eq);
        throw_assert(ptr == ptr_to, "signed_to is not idempotent, `" << ptr << "` => `" << ptr_to << "`");
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

void test_work_steal_deque_growing()
{
    auto only_pop   = proxc::WorkStealDeque<Object>::create();
    auto only_steal = proxc::WorkStealDeque<Object>::create();

    const auto capacity = proxc::WorkStealDeque<Object>::default_capacity;
    auto num_items = capacity * 2;

    for (std::size_t i = 0; i < capacity; ++i) {
        only_pop.first->push(std::make_unique<TestObject<std::size_t>>(i));
        only_steal.first->push(std::make_unique<TestObject<std::size_t>>(i));

        throw_assert_eq(only_pop.first->capacity(),   capacity, "inconsistent capacity");
        throw_assert_eq(only_pop.first->size(),       i + 1,    "inconsistent sizes");
        throw_assert_eq(only_steal.first->capacity(), capacity, "inconsistent capacity");
        throw_assert_eq(only_steal.first->size(),     i + 1,    "inconsistent sizes");
    }
    //  both deques contains (cap-1, cap-2, ..., 1, 0)

    for (std::size_t i = capacity; i < num_items; ++i) {
        only_pop.first->push(std::make_unique<TestObject<std::size_t>>(i));
        only_steal.first->push(std::make_unique<TestObject<std::size_t>>(i));

        throw_assert_eq(only_pop.first->capacity(), 2 * capacity, "inconsistent capacity");
        throw_assert_eq(only_pop.first->size(),     i + 1,        "inconsistent sizes");
        throw_assert_eq(only_steal.first->capacity(), 2 * capacity, "inconsistent capacity");
        throw_assert_eq(only_steal.first->size(),     i + 1,        "inconsistent sizes");
    }
    //  both deques contains (2*cap-1, 2*cap-2, ..., 1, 0)

    for (std::size_t i = 0; i < num_items; ++i) {
        auto pop_item   = only_pop.first->pop();
        auto steal_item = only_steal.second->steal();
        throw_assert(pop_item != nullptr,   "item is nullptr, i = " << i);
        throw_assert(steal_item != nullptr, "item is nullptr, i = " << i);

        auto pop_value   = pop_item->get();
        auto steal_value = steal_item->get();
        throw_assert_eq(pop_value,   2 * capacity - i - 1, "items does not match");
        throw_assert_eq(steal_value, i,                    "items does not match");
    }
}

void test_work_steal_deque_fill_and_deplete()
{
    auto only_pop = proxc::WorkStealDeque<Object>::create();
    auto only_steal = proxc::WorkStealDeque<Object>::create();

    const auto capacity  = proxc::WorkStealDeque<Object>::default_capacity;
    const auto num_items = capacity;

    {
        throw_assert_eq(only_pop.first->capacity(),   capacity, "inconsistent capacity");
        throw_assert_eq(only_pop.first->size(),       0u,       "inconsistent sizes");
        throw_assert_eq(only_steal.first->capacity(), capacity, "inconsistent capacity");
        throw_assert_eq(only_steal.first->size(),     0u,       "inconsistent sizes");
    }

    for (std::size_t i = 0; i < num_items; ++i) {
        only_pop.first->push(std::make_unique<TestObject<std::size_t>>(i));
        only_steal.first->push(std::make_unique<TestObject<std::size_t>>(i));

        throw_assert_eq(only_pop.first->capacity(),   capacity, "inconsistent capacity");
        throw_assert_eq(only_pop.first->size(),       i + 1,    "inconsistent capacity");
        throw_assert_eq(only_steal.first->capacity(), capacity, "inconsistent sizes");
        throw_assert_eq(only_steal.first->size(),     i + 1,    "inconsistent sizes");
    }

    for (std::size_t i = 0; i < num_items; ++i) {
        auto pop_item   = only_pop.first->pop();
        auto steal_item = only_steal.second->steal();
    
        throw_assert_eq(only_pop.first->capacity(),   capacity,          "inconsistent capacity");
        throw_assert_eq(only_pop.first->size(),       num_items - i - 1, "inconsistent sizes");
        throw_assert_eq(only_steal.first->capacity(), capacity,          "inconsistent capacity");
        throw_assert_eq(only_steal.first->size(),     num_items - i - 1, "inconsistent sizes");

        throw_assert(pop_item != nullptr,   "item is nullptr");
        throw_assert(steal_item != nullptr, "item is nullptr");
        auto pop_value   = pop_item.get()->get();
        auto steal_value = steal_item.get()->get();
        throw_assert_eq(pop_value,   num_items - i - 1, "items does not match");
        throw_assert_eq(steal_value, i,                 "items does not match");
    }

    {
        throw_assert_eq(only_pop.first->capacity(),   capacity, "inconsistent capacity");
        throw_assert_eq(only_pop.first->size(),       0u,       "inconsistent sizes");
        throw_assert_eq(only_steal.first->capacity(), capacity, "inconsistent capacity");
        throw_assert_eq(only_steal.first->size(),     0u,       "inconsistent sizes");
    }
}

void test_work_steal_deque_single_thread() 
{
    auto deque = proxc::WorkStealDeque<Object>::create();

    {
        throw_assert(deque.first->pop()    == nullptr, "empty deque does not return nullptr on pop");
        throw_assert(deque.second->steal() == nullptr, "empty deque does not return nullptr on steal");
    }

    deque.first->push(std::make_unique<Object>(1337)); // (1337)
    deque.first->push(std::make_unique<Object>(10));   // (10, 1337)
    deque.first->push(std::make_unique<Object>(42));   // (42, 10, 1337)

    {
        auto item = deque.first->pop();                            // (10, 1337)
        throw_assert(item != nullptr, "non-empty deque returns nullptr on pop");
        auto value = item->get();
        throw_assert_eq(value, 42u, "popped item does not match content");
    }
    
    {
        auto item = deque.second->steal();                          // (10)
        throw_assert(item != nullptr, "non-empty deque returns nullptr on steal");
        auto value = item->get();
        throw_assert_eq(value, 1337u, "popped item does not match content");
    }

    {
        auto item = deque.second->steal();                          // (*empty*)
        throw_assert(item != nullptr, "non-empty deque returns nullptr on steal");
        auto value = item->get();
        throw_assert_eq(value, 10u, "popped item does not match content");
    }

    {
        throw_assert(deque.first->pop()    == nullptr, "empty deque does not return nullptr on pop");
        throw_assert(deque.second->steal() == nullptr, "empty deque does not return nullptr on steal");
    }
}

template<typename T, unsigned int C>
int worker_func(typename proxc::WorkStealDeque<T, C>::StealerEnd stealer)
{
    int steals = 0;
    while (stealer->size() != 0) {
        auto item = stealer->steal();
        if (item != nullptr) {
            steals++;
        }
    }
    return steals;
}

void test_work_steal_deque_multiple_threads()
{
    const std::size_t num_workers = std::thread::hardware_concurrency() - 1;
    const std::size_t num_items = 1000000;

    // populate deque
    auto deque = proxc::WorkStealDeque<Object, 1 << 20>::create();
    auto popper = std::move(deque.first);
    for (std::size_t i = 0; i < num_items; i++) {
        popper->push(std::make_unique<Object>(i));
    }

    // start workers
    std::vector<std::future<int>> values;
    for (std::size_t i = 0; i < num_workers; ++i) {
        values.push_back(std::async(
            std::launch::async,
            &worker_func<Object, 1<<20>, deque.second));
    }
    
    // pop from deque while it is non-empty
    int n_pops = 0;
    while (popper->size() != 0) {
        auto item = popper->pop();
        if (item != nullptr) {
            n_pops += 1;
        }
    }

    // get the results from the workers
    int total_steals = n_pops;
    for (auto& value : values) {
        auto n_steals = value.get();
        total_steals += n_steals;
    }
    throw_assert_eq(total_steals, static_cast<int>(num_items), "total steals does not equal total items");
}

int main()
{
    test_to_signed();
    test_work_steal_deque_fill_and_deplete();
    test_work_steal_deque_growing();
    test_work_steal_deque_single_thread();
    test_work_steal_deque_multiple_threads();
}

