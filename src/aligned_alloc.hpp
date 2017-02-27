
#pragma once

#include <proxc.hpp>

#include <stdlib.h> // posix_memalign

#include <new>
#include <type_traits>

PROXC_NAMESPACE_BEGIN

void* aligned_alloc(std::size_t size, std::size_t align)
{
#ifdef _WIN32
    void *ptr = _aligned_malloc(size, align);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return ptr;
#else
    void *ptr;
    if (posix_memalign(&ptr, align, size)) {
        throw std::bad_alloc();
    }
    return ptr;
#endif
}

void aligned_free(void* addr) noexcept
{
#ifdef _WIN32
    _aligned_free(addr);
#else
    free(addr);
#endif
}

template<typename T, std::size_t Align = std::alignment_of<T>::value>
class AlignedArray 
{
public:
    AlignedArray()
        : m_length(0), m_ptr(nullptr) {}

    AlignedArray(std::nullptr_t)
        : m_length(0), m_ptr(nullptr) {}

    explicit AlignedArray(std::size_t length)
        : m_length(length)
    {
        m_ptr = static_cast<T*>(aligned_alloc(length * sizeof(T), Align));
        std::size_t i;
        try {
            for (i = 0; i < m_length; ++i) {
                new(m_ptr + i) T;
            }
        } catch(...) {
            for (std::size_t j = 0; j < i; ++j) {
                m_ptr[i].~T();
                throw;
            }
        }
    }

    AlignedArray(AlignedArray&& other) noexcept
        : m_length(other.m_length), m_ptr(other.m_ptr)
    {
        
    }

    AlignedArray& operator=(std::nullptr_t)
    {
        return *this = AlignedArray();
    }

    ~AlignedArray()
    {
        for (std::size_t i = 0; i < m_length; ++i) {
            m_ptr[i].~T();
        }
        aligned_free(m_ptr);
    }

    T& operator[](std::size_t i) const
    {
        return m_ptr[i];
    }

    std::size_t size() const
    {
        return m_length;
    }

    T* get() const
    {
        return m_ptr;
    }

    explicit operator bool() const 
    {
        return m_ptr != nullptr;
    }

private:
    std::size_t m_length;
    T* m_ptr;
};

PROXC_NAMESPACE_END

