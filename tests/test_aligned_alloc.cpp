
#include <proxc.hpp>

#include <string>

#include "setup.hpp"

#include "aligned_alloc.hpp"

void test_aligned_alloc()
{
    const std::size_t align = 1024;

    void *ptr = proxc::aligned_alloc(4096, align);
    throw_assert(ptr, "aligned_alloc returned nullptr");

    auto uptr = reinterpret_cast<std::uintptr_t>(ptr);
    throw_assert((uptr % static_cast<std::uintptr_t>(align)) == 0, "ptr `" << ptr << "` is not aligned `" << align << "`");

    proxc::aligned_free(ptr);
}

void test_aligned_array()
{
    const std::size_t align = 64;
    const std::size_t size = 1024;
    
    {
        proxc::AlignedArray<int, align> array{};
        throw_assert(array.size() == 0, "array length is not zero: " << array.size());
        throw_assert(array.get() == nullptr, "array ptr is not nullptr: " << array.get());
        throw_assert(!array, "array bool operator does not equal false");
    }

    {
        proxc::AlignedArray<int, align> array{nullptr};
        throw_assert(array.size() == 0, "array length is not zero: " << array.size());
        throw_assert(array.get() == nullptr, "array ptr is not nullptr: " << array.get());
        throw_assert(!array, "array bool operator does not equal false");
    }

    {
        proxc::AlignedArray<int, align> array{size};
        throw_assert(array.size() == size, "array size does not match size " << size);
        throw_assert(array.get() != nullptr, "array ptr is nullptr");
        throw_assert(array, "array bool operator does not equal true");
        
        const std::size_t index = size % 10;
        const std::size_t value = 42;
        throw_assert(array[index] == int{}, "default array value `" << array[index] << "` is not equal default constructor value `" << int{} << "`");
        array[index] = value;
        throw_assert(array[index] == value, "yielded value `" << array[index] << "` is not the same as stored value `" << value << "`");
        throw_assert(array[index+1] == int{}, "default array value `" << array[index+1] << "` other than index is not equal default constructor value `" << int{} << "`");
    }

    {
        proxc::AlignedArray<std::string, align> array{size};
        std::string message = "test";
        array[0] = message;
        proxc::AlignedArray<std::string, align> other{std::move(array)};
        throw_assert(other.size() == size, "new array size `" << array.size() << "`does not equal moved size `" << size << "`");
        throw_assert(other[0] == message, "new array at 0 `" << other[0] << "` does not contain message `" << message << "`");
        throw_assert(array.size() == 0, "moved array does not have length 0 `" << array.size() << "`");
        throw_assert(array.get() == nullptr, "moved array is not nullptr `" << array.get() << "`");
    }
}

int main()
{
    test_aligned_alloc();
    test_aligned_array();
}

