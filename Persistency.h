#pragma once

#include <string>


class DataEngine;


class Persistency
{
public:
    static size_t initial_load_data(DataEngine& engine, const std::string& databaseFilename);
    static size_t store_data(const DataEngine& engine, const std::string& databaseFilename);
};
