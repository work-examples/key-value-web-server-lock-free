#pragma once

#include "Allocator.h"


class AllocatorFactory
{
public:
    template <class T>
    static SeparateHeapAllocator<T> get_allocator()
    {
        // TODO: implement selecting random allocator from a bunch of thread allocators
        // or return lock-free global allocator
        // or ...
        return get_current_thread_allocator();
    }

protected:
    using DefaultAllocator = SeparateHeapAllocator<char>;

    static const DefaultAllocator& get_current_thread_allocator();
};
