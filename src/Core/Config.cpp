#include "Config.h"

bool Config::Load(const std::string& filepath)
{
    m_filepath = filepath;

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        // File doesn't exist, use defaults and create it
        Save();
        return true;
    }

    try
    {
        nlohmann::json j;
        file >> j;
        m_settings = j.get<AppSettings>();
    }
    catch (const std::exception&)
    {
        // Parse error - use defaults
        return false;
    }

    m_dirty = false;
    return true;
}

bool Config::Save()
{
    return Save(m_filepath);
}

bool Config::Save(const std::string& filepath)
{
    std::ofstream file(filepath);
    if (!file.is_open())
        return false;

    nlohmann::json j = m_settings;
    file << j.dump(4);  // Pretty print with 4-space indent

    m_dirty = false;
    return true;
}
