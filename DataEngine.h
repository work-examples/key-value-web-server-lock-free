#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>


class DataEngine
{
public:
    using IntegerCounter = std::uint64_t;

    struct AccessStatistics
    {
        IntegerCounter  m_successOperations = 0;
        IntegerCounter  m_failedOperations = 0;
    };

public:
    std::optional<std::string> get(const std::string_view name) const;

    void set(const std::string_view name, const std::string_view value);

    using EnumerateVisitorProc = void(const std::string_view name, const std::string_view value);

    void enumerate(const std::function<EnumerateVisitorProc>& visitor) const;

    AccessStatistics get_read_statistics() const;

protected:
    using DataCollection = std::unordered_map<std::string, std::string>;

    mutable std::shared_mutex   m_protectData;
    DataCollection              m_data;

    // Global statistics:
    mutable std::atomic<IntegerCounter> m_successReads = ATOMIC_VAR_INIT(0);
    mutable std::atomic<IntegerCounter> m_failedReads  = ATOMIC_VAR_INIT(0);
};
