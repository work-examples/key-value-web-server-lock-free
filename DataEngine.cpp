#include "DataEngine.h"
#include "AllocatorFactory.h"

#include "utils/stl.h"

#include <assert.h>


DataEngine::DataEngine(const size_t bucketCount):
    m_buckets(bucketCount)
{
}

DataEngine::~DataEngine()
{
    for (const AtomicNodePtr& bucket : m_buckets)
    {
        ListNode* node = bucket.load(std::memory_order_acquire);

        while (node != nullptr)
        {
            ListNode* current = node;
            node = node->m_next.load(std::memory_order_acquire);
            current->delete_self();
        }
    }
    m_buckets.clear();
}

std::optional<DataEngine::String> DataEngine::get(const std::string_view key) const
{
    const size_t buckedIdx = Hash()(key) % m_buckets.size();
    const ListNode* node = m_buckets[buckedIdx].load(std::memory_order_acquire);

    while (node != nullptr)
    {
        if (node->m_key == key)
        {
            m_successReads.fetch_add(1, std::memory_order_acq_rel);

            const auto ptrValueCopy = node->get_value_const_ref();
            return *ptrValueCopy;
        }

        node = node->m_next.load(std::memory_order_acquire);
    }

    m_failedReads.fetch_add(1, std::memory_order_acq_rel);
    return {};
}

DataEngine::NodeUniquePtr DataEngine::create_node(const std::string_view key, const std::string_view value)
{
    ListNode::NodeAllocator allocator = AllocatorFactory::get_allocator<ListNode>();

    NodeDeleter* deleter = [](ListNode* const ptr)
    {
        ptr->delete_self();
        return;
    };

    ListNode* raw_ptr = std_extra::allocate_new(
        allocator,
        allocator,
        String(key, allocator),
        std::allocate_shared<const String>(allocator, String(value, allocator))
    );

    NodeUniquePtr ptrNewNode = std::unique_ptr<ListNode, NodeDeleter*>(raw_ptr, deleter);
    return ptrNewNode;
}

void DataEngine::set(const std::string_view key, const std::string_view value)
{
    NodeUniquePtr ptrNewNode = create_node(key, value);

    const size_t buckedIdx = Hash()(key) % m_buckets.size();
    ListNode* node = nullptr;

    const bool exchangedFirst = m_buckets[buckedIdx].compare_exchange_strong(node, ptrNewNode.get(), std::memory_order_acq_rel, std::memory_order_acquire);
    if (exchangedFirst)
    {
        // we have put the first element in the bucket
        ptrNewNode.release(); // do not own the node any more. Its owner is the bucket list now.
        return;
    }
    assert(node != nullptr);

    while (true)
    {
        if (node->m_key == key)
        {
            node->m_ptrValue.store(ptrNewNode->m_ptrValue, std::memory_order_release);
            return; // ptrNewNode is deallocated automatically here
        }

        ListNode* nextNode = nullptr;
        const bool exchanged = node->m_next.compare_exchange_strong(nextNode, ptrNewNode.get(), std::memory_order_acq_rel, std::memory_order_acquire);
        if (exchanged)
        {
            // we have put the element at the end of the bucket list
            ptrNewNode.release(); // do not own the node any more. Its owner is the bucket list now.
            return;
        }
        assert(nextNode != nullptr);
        node = nextNode;
    }
}

void DataEngine::enumerate(const std::function<EnumerateVisitorProc>& visitor) const
{
    for (const AtomicNodePtr& bucket : m_buckets)
    {
        const ListNode* node = bucket.load(std::memory_order_acquire);

        while (node != nullptr)
        {
            const auto ptrValueCopy = node->get_value_const_ref();

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
