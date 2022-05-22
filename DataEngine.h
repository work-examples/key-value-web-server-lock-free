#pragma once

#include "Allocator.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "utils/stl.h" // for std::atomic<std::shared_ptr> in non-compatible compilers


class DataEngine
{
public:
    using IntegerCounter = std::uint64_t;

    template<typename T>
    using Allocator = SeparateHeapAllocator<T>;
    using String = std::basic_string<char, std::char_traits<char>, Allocator<char>>;

    struct AccessStatistics
    {
        IntegerCounter  m_successOperations = 0;
        IntegerCounter  m_failedOperations  = 0;
    };

public:
    DataEngine(const size_t bucketCount);
    ~DataEngine();

    bool is_lock_free() const;

    std::optional<String> get(const std::string_view key) const;

    void set(const std::string_view key, const std::string_view value);

    using EnumerateVisitorProc = void(const std::string_view key, const std::string_view value);

    void enumerate(const std::function<EnumerateVisitorProc>& visitor) const;

    AccessStatistics get_read_statistics() const;

protected:
    class ListNode
    {
    public:
        using NodeAllocator = Allocator<ListNode>;
        using AtomicSharedConstStringPtr = std::atomic<std::shared_ptr<const String>>;

    public:
        const String                    m_key;
        AtomicSharedConstStringPtr      m_ptrValue;
        std::atomic<ListNode*>          m_next = nullptr;
        NodeAllocator                   m_allocator;

        //static_assert(AtomicSharedConstStringPtr::is_always_lock_free);
        static_assert(std::atomic<ListNode*>::is_always_lock_free);

    public:
        ListNode(const NodeAllocator& allocator, String&& key, std::shared_ptr<const String>&& ptrValue) :
            m_key(key), m_ptrValue(ptrValue), m_allocator(allocator)
        {
        }

        void delete_self()
        {
            NodeAllocator alloc(m_allocator);
            std::allocator_traits<NodeAllocator>::destroy(alloc, this);
            std::allocator_traits<NodeAllocator>::deallocate(alloc, this, 1);
        }

        std::shared_ptr<const String> get_value_const_ref() const
        {
            return m_ptrValue.load(std::memory_order_acquire);
        }
    };

    using AtomicNodePtr = std::atomic<ListNode*>;
    using Hash = std::hash<std::string_view>;
    using NodeDeleter = void(ListNode* const ptr);
    using NodeUniquePtr = std::unique_ptr<ListNode, NodeDeleter*>;

protected:
    NodeUniquePtr create_node(const std::string_view key, const std::string_view value);

protected:
    std::vector<AtomicNodePtr>          m_buckets;

    // Global statistics:
    mutable std::atomic<IntegerCounter> m_successReads = 0;
    mutable std::atomic<IntegerCounter> m_failedReads  = 0;

    static_assert(AtomicNodePtr::is_always_lock_free);
    static_assert(std::atomic<IntegerCounter>::is_always_lock_free);
};
