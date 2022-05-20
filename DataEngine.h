#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


class DataEngine
{
public:
    using IntegerCounter = std::uint64_t;

    struct AccessStatistics
    {
        IntegerCounter  m_successOperations = 0;
        IntegerCounter  m_failedOperations  = 0;
    };

public:
    DataEngine(const size_t bucketCount);
    ~DataEngine();

    std::optional<std::string> get(const std::string_view key) const;

    void set(const std::string_view key, const std::string_view value);

    using EnumerateVisitorProc = void(const std::string_view key, const std::string_view value);

    void enumerate(const std::function<EnumerateVisitorProc>& visitor) const;

    AccessStatistics get_read_statistics() const;

protected:
    struct ListNode
    {
        using SafeSharedStringPtr = std::atomic<std::shared_ptr<const std::string>>;

        const std::string               m_key;
        SafeSharedStringPtr             m_ptrValue;
        std::atomic<ListNode*>          m_next = ATOMIC_VAR_INIT(nullptr);

        std::shared_ptr<const std::string> get_value_copy() const
        {
            return m_ptrValue.load(std::memory_order_acquire);
        }
    };

    using AtomicNodePtr = std::atomic<ListNode*>;

    std::vector<AtomicNodePtr>          m_buckets;
    std::hash<std::string_view>         m_hash;

    // Global statistics:
    mutable std::atomic<IntegerCounter> m_successReads = ATOMIC_VAR_INIT(0);
    mutable std::atomic<IntegerCounter> m_failedReads  = ATOMIC_VAR_INIT(0);
};
