#include "DataEngine.h"


std::optional<std::string> DataEngine::get(const std::string_view name) const
{
    const std::string strName(name);

    const std::shared_lock lock(m_protectData); // read-only lock

    const auto iter = m_data.find(strName);
    if (iter == m_data.cend())
    {
        m_failedReads.fetch_add(1, std::memory_order_acq_rel);
        return {};
    }

    m_successReads.fetch_add(1, std::memory_order_acq_rel);

    return iter->second;
}

void DataEngine::set(const std::string_view name, const std::string_view value)
{
    std::string strName(name);

    const std::lock_guard lock(m_protectData); // write lock

    const auto iter = m_data.find(strName);
    if (iter == m_data.cend())
    {
        m_data.emplace(std::move(strName), value);
    }
    else
    {
        iter->second = value;
    }
}

void DataEngine::enumerate(const std::function<EnumerateVisitorProc>& visitor) const
{
    const std::shared_lock lock(m_protectData); // read-only lock

    for (const auto& name_value : m_data)
    {
        visitor(name_value.first, name_value.second);
    }
}

DataEngine::AccessStatistics DataEngine::get_read_statistics() const
{
    // No need in full consistency here
    return { m_successReads.load(std::memory_order_acquire), m_failedReads.load(std::memory_order_acquire) };
}
