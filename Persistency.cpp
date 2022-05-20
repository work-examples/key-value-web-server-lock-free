#include "Persistency.h"

#include "DataEngine.h"
#include "DataSerializer.h"
#include "Logger.h"


size_t Persistency::initial_load_data(DataEngine& engine, const std::string& databaseFilename)
{
    size_t recordCount = 0;
    auto loadVisitor = [&engine, &recordCount](const std::string_view name, const std::string_view value)
    {
        engine.initial_set(name, value);
        ++recordCount;
        return;
    };

    const bool ok = DataSerializer::load(databaseFilename, loadVisitor);
    if (!ok)
    {
        LOG_ERROR << "DataSerializer::load() failed" << std::endl;
        return recordCount;
    }

    return recordCount;
}

size_t Persistency::store_data(const DataEngine& engine, const std::string& databaseFilename)
{
    size_t recordCount = 0;
    DataSerializer::Document document;

    const std::function<DataEngine::EnumerateVisitorProc> visitor =
        [&document, &recordCount](const std::string_view name, const std::string_view value)
    {
        document.add(name, value);
        ++recordCount;
        return;
    };

    engine.enumerate(visitor);

    const bool ok = DataSerializer::save(databaseFilename, document);
    if (!ok)
    {
        LOG_ERROR << "DataSerializer::save() failed" << std::endl;
        return recordCount;
    }

    return recordCount;
}
