#pragma once

#include <cstdint>
#include <memory>


template <typename T>
class SeparateHeapAllocator {
public:
    friend class AllocatorFactory;

public:
    using value_type = T;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using propagate_on_container_move_assignment = std::true_type;

    using HeapID = std::size_t;

public:
    constexpr SeparateHeapAllocator(const SeparateHeapAllocator& other) noexcept : m_heap(other.m_heap)
    {
    }

    template <class T2>
    constexpr SeparateHeapAllocator(const SeparateHeapAllocator<T2>& other) noexcept : m_heap(other.native_heap_handle())
    {
    }

    constexpr SeparateHeapAllocator& operator=(const SeparateHeapAllocator&) = default;

    [[nodiscard]] constexpr T* allocate(const size_t count)
    {
        // TODO: implement separate m_heap-based allocations. So users of different heaps do not block each other
        return std::allocator<T>().allocate(count);
    }

    constexpr void deallocate(T* const ptr, const size_t count)
    {
        // TODO: implement separate m_heap-based allocations. So users of different heaps do not block each other
        std::allocator<T>().deallocate(ptr, count);
    }

public:
    constexpr HeapID native_heap_handle() const
    {
        return m_heap;
    }

protected:
    explicit constexpr SeparateHeapAllocator(const HeapID heap) noexcept : m_heap(heap) {}

protected:
    HeapID              m_heap = 0; // placement for future separate real heap handler
};
