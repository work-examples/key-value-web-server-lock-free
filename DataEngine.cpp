#include "DataEngine.h"

#include <assert.h>


DataEngine::DataEngine(const size_t bucketCount):
    m_buckets(bucketCount)
{
}

DataEngine::~DataEngine()
{
    for (const AtomicNodePtr& bucket : m_buckets)
    {
        const ListNode* node = bucket.load(std::memory_order_acquire);

        while (node != nullptr)
        {
            const ListNode* current = node;
            node = node->m_next.load(std::memory_order_acquire);
            delete current;
        }
    }
    m_buckets.clear();
}

std::optional<std::string> DataEngine::get(const std::string_view key) const
{
    const size_t buckedIdx = m_hash(key) % m_buckets.size();
    const ListNode* node = m_buckets[buckedIdx].load(std::memory_order_acquire);

    while (node != nullptr)
    {
        if (node->m_key == key)
        {
            m_successReads.fetch_add(1, std::memory_order_acq_rel);

            const auto ptrValueCopy = node->get_value_copy();
            return *ptrValueCopy;
        }

        node = node->m_next.load(std::memory_order_acquire);
    }

    m_failedReads.fetch_add(1, std::memory_order_acq_rel);
    return {};
}

void DataEngine::set(const std::string_view key, const std::string_view value)
{
    const size_t buckedIdx = m_hash(key) % m_buckets.size();
    ListNode* node = m_buckets[buckedIdx].load(std::memory_order_acquire);

    if (node == nullptr)
    {
        m_buckets[buckedIdx].store(new ListNode{ std::move(std::string(key)), std::move(std::make_shared<const std::string>(value)), nullptr }, std::memory_order_release);
        return;
    }

    while (true)
    {
        if (node->m_key == key)
        {
            node->m_ptrValue.store(std::make_shared<const std::string>(value), std::memory_order_release);
            return;
        }
        if (node->m_next == nullptr)
        {
            node->m_next.store(new ListNode{std::move(std::string(key)), std::move(std::make_shared<const std::string>(value)), nullptr}, std::memory_order_release);
            return;
        }
        node = node->m_next.load(std::memory_order_acquire);
    }
}

void DataEngine::enumerate(const std::function<EnumerateVisitorProc>& visitor) const
{
    for (const AtomicNodePtr& bucket : m_buckets)
    {
        const ListNode* node = bucket.load(std::memory_order_acquire);

        while (node != nullptr)
        {
            const auto ptrValueCopy = node->get_value_copy();

            visitor(node->m_key, *ptrValueCopy);

            node = node->m_next.load(std::memory_order_acquire);
        }
    }
}

DataEngine::AccessStatistics DataEngine::get_read_statistics() const
{
    // No need in full consistency here
    return { m_successReads.load(std::memory_order_acquire), m_failedReads.load(std::memory_order_acquire) };
}
