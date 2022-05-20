#include "DataEngine.h"


DataEngine::DataEngine(const size_t bucketCount):
    m_buckets(bucketCount)
{
}

DataEngine::~DataEngine()
{
    for (ListNode* node : m_buckets)
    {
        while (node != nullptr)
        {
            ListNode* current = node;
            node = node->next;
            delete current;
        }
    }
    m_buckets.clear();
}

std::optional<std::string> DataEngine::get(const std::string_view key) const
{
    const size_t buckedIdx = m_hash(key) % m_buckets.size();
    ListNode* node = m_buckets[buckedIdx];
    while (node != nullptr)
    {
        if (node->key == key)
        {
            m_successReads.fetch_add(1, std::memory_order_acq_rel);
            return *node->value;
        }
        node = node->next;
    }
    m_failedReads.fetch_add(1, std::memory_order_acq_rel);
    return {};
}

void DataEngine::set(const std::string_view key, const std::string_view value)
{
    const size_t buckedIdx = m_hash(key) % m_buckets.size();
    ListNode* node = m_buckets[buckedIdx];

    if (node == nullptr)
    {
        m_buckets[buckedIdx] = new ListNode{ std::string(key), std::make_shared<std::string>(value), nullptr };
        return;
    }

    while (true)
    {
        if (node->key == key)
        {
            *node->value = value;
            return;
        }
        if (node->next == nullptr)
        {
            node->next = new ListNode{std::string(key), std::make_shared<std::string>(value), nullptr};
            return;
        }
        node = node->next;
    }
}

void DataEngine::enumerate(const std::function<EnumerateVisitorProc>& visitor) const
{
    for (ListNode* node : m_buckets)
    {
        while (node != nullptr)
        {
            visitor(node->key, *node->value);
            node = node->next;
        }
    }
}

DataEngine::AccessStatistics DataEngine::get_read_statistics() const
{
    // No need in full consistency here
    return { m_successReads.load(std::memory_order_acquire), m_failedReads.load(std::memory_order_acquire) };
}
