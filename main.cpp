#include "DataEngine.h"
#include "HttpServer.h"
#include "Logger.h"
#include "Persistency.h"

#include <cstdint>
#include <future>
#include <string>
#include <string_view>
#include <thread>


int main(const int argc, const char* const* const argv)
{
    const Logger::LogLevel logLevel = Logger::LogLevel::Debug;
    Logger::SetLogLevel(logLevel);

    LOG_INFO << "main: begin" << std::endl;
    const std::string arg1 = argc > 1 ? argv[1] : "";

    // =========================================================
    // ===   Configuration:
    // =========================================================

    const size_t        expectedElementCount = 1000 * 1000;
    const size_t        hashMapBucketCount = expectedElementCount * 2;
    const std::string   listenHost = "127.0.0.1";
    const std::uint16_t listenPort = 8000;
    const std::string   databaseFilename = "database.json";
    const bool          logEachRequest = arg1 != "--no-logs";

    // =========================================================

    DataEngine engine(hashMapBucketCount);

    const bool lock_free = engine.is_lock_free();
    if (lock_free)
    {
        LOG_INFO << "main: Engine implementation is lock free (except memory allocations)" << std::endl;
    }
    else
    {
        LOG_WARN << "main: WARNING! Engine implementation IS NOT LOCK-FREE!" << std::endl;
    }

    LOG_INFO << "main: load data..." << std::endl;
    const size_t loadedRecordCount = Persistency::initial_load_data(engine, databaseFilename);
    LOG_INFO << "main: loaded " << loadedRecordCount << " DB records from file " << databaseFilename << std::endl;

    {
        LOG_INFO << "main: listening connections: begin" << std::endl;

        HttpServer server;
        server.run(listenHost, listenPort, engine, logEachRequest);

        LOG_INFO << "main: listening connections: end" << std::endl;
    }

    LOG_INFO << "main: save data to file before process exit..." << std::endl;
    const size_t savedRecordCount = Persistency::store_data(engine, databaseFilename);
    LOG_INFO << "main: saved " << savedRecordCount << " DB records to file " << databaseFilename << std::endl;

    LOG_INFO << "main: end" << std::endl;

    return 0;
}
