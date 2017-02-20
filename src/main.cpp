
#include <iostream>
#include <vector>

#include <boost/coroutine/coroutine.hpp>

#include <proxc/proxc.hpp>

using proc = boost::coroutines::symmetric_coroutine<void>;

std::vector<int>
merge(const std::vector<int>& a, const std::vector<int>& b)
{
    std::vector<int> c;
    std::size_t idx_a = 0, idx_b = 0;
    proc::call_type *other_a = 0, *other_b = 0;

    proc::call_type coro_a(
        [&](proc::yield_type& yield) {
            while (idx_a < a.size()) {
                if (b[idx_b] < a[idx_a]) {
                    yield(*other_b);
                }
                c.push_back(a[idx_a++]);
            }
            while (idx_b < b.size()) {
                c.push_back(b[idx_b++]);
            }
        });

    proc::call_type coro_b(
        [&](proc::yield_type& yield) {
            while (idx_b < b.size()) {
                if (a[idx_a] < b[idx_b]) {
                    yield(*other_a);
                }
                c.push_back(b[idx_b++]);
            }
            while (idx_a < a.size()) {
                c.push_back(a[idx_a++]);
            }
        });

    other_a = &coro_a;
    other_b = &coro_b;

    coro_a();

    return c;
}

void
print(std::vector<int>& v) {
    for (const auto i : v) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
}

int main()
{
    std::cout << get_name() << std::endl;
    std::vector<int> a = { 1, 5, 6, 10 };
    std::vector<int> b = { 2, 4, 7, 8, 9, 13 };
    std::vector<int> c = merge(a, b);

    print(a);
    print(b);
    print(c);

    return 0;
}
