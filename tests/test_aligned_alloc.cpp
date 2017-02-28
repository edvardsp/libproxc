
#include <proxc.hpp>

#include <string>

#include "setup.hpp"

#include "aligned_alloc.hpp"

void test_aligned_alloc()
{
    const std::size_t align = 1024;

    void *ptr = proxc::aligned_alloc(4096, align);
    throw_assert(!!ptr, "aligned_alloc returned nullptr");

    auto uptr = reinterpret_cast<std::uintptr_t>(ptr);
    throw_assert_eq(uptr % static_cast<std::uintptr_t>(align), 0u, "ptr is not aligned");

    proxc::aligned_free(ptr);
}

void test_aligned_array()
{
    const std::size_t align = 64;
    const std::size_t size = 1024;
    
    {
        proxc::AlignedArray<int, align> array{};
        throw_assert_eq(array.size(), 0u, "array length is not zero");
        throw_assert(array.get() == nullptr, "array ptr is not nullptr");
        throw_assert(!array, "array bool operator does not equal false");
    }

    {
        proxc::AlignedArray<int, align> array{nullptr};
        throw_assert_eq(array.size(), 0u, "array length is not zero");
        throw_assert(array.get() == nullptr, "array ptr is not nullptr");
        throw_assert(!array, "array bool operator does not equal false");
    }

    {
        proxc::AlignedArray<int, align> array{size};
        throw_assert_eq(array.size(), size, "array size does not match size");
        throw_assert(array.get() != nullptr, "array ptr is nullptr");
        throw_assert(array, "array bool operator does not equal true");
        
        const std::size_t index = size % 10;
        const auto value = 42;
        throw_assert_eq(array[index], int{}, "default array value is not equal to default constructor value");
        array[index] = value;
        throw_assert_eq(array[index], value, "yielded value is not the same as stored value");
        throw_assert_eq(array[index + 1], int{}, "default array value other than index is not equal to default constructor value");
    }

    {
        proxc::AlignedArray<std::string, align> array{size};
        std::string message = "test";
        array[0] = message;
        proxc::AlignedArray<std::string, align> other{std::move(array)};
        throw_assert_eq(other.size(), size, "new array size does not equal moved size");
        throw_assert_eq(other[0], message, "new array does not contain message");
        throw_assert_eq(array.size(), 0u, "moved array does not have length 0");
        throw_assert(array.get() == nullptr, "moved array is not nullptr");
    }
}

int main()
{
    test_aligned_alloc();
    test_aligned_array();
}

