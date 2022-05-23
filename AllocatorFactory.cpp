#include "AllocatorFactory.h"

#include <shared_mutex>
#include <thread>
#include <unordered_map>

#include <assert.h>


const AllocatorFactory::DefaultAllocator& AllocatorFactory::get_current_thread_allocator()
{
    static std::shared_mutex protect;
    static std::unordered_map<std::thread::id, DefaultAllocator> allocators;

    const std::thread::id threadId = std::this_thread::get_id();

    {
        std::shared_lock<std::shared_mutex> lockForRead(protect);
        const auto iter = allocators.find(threadId);
        if (iter != allocators.end())
        {
            return iter->second;
        }
    }

    const size_t heapId = static_cast<size_t>(std::hash<std::thread::id>{}(threadId));
    const DefaultAllocator newAllocator(heapId);

    {
        std::unique_lock<std::shared_mutex> lockForWrite(protect);
        const auto result = allocators.emplace(threadId, newAllocator);
        assert(result.second && "Insertion should always succeed since nobody else except current thread can insert the same key");

        return result.first->second;
    }
}
