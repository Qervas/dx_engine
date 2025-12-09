#include "Config.h"
#include <algorithm>
#include <cctype>

std::string Config::Trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\r\n\"");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n\",");
    return str.substr(start, end - start + 1);
}

bool Config::ParseBool(const std::string& value)
{
    std::string v = Trim(value);
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    return v == "true" || v == "1" || v == "yes";
}

int Config::ParseInt(const std::string& value)
{
    try {
        return std::stoi(Trim(value));
    } catch (...) {
        return 0;
    }
}

float Config::ParseFloat(const std::string& value)
{
    try {
        return std::stof(Trim(value));
    } catch (...) {
        return 0.0f;
    }
}

bool Config::Load(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        return false;
    }

    std::string line;
    std::string currentSection;

    while (std::getline(file, line))
    {
        line = Trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '/' || line[0] == '#')
            continue;

        // Check for section (JSON-style object key)
        if (line.find("\"graphics\"") != std::string::npos)
        {
            currentSection = "graphics";
            continue;
        }
        else if (line.find("\"display\"") != std::string::npos)
        {
            currentSection = "display";
            continue;
        }
        else if (line.find("\"debug\"") != std::string::npos)
        {
            currentSection = "debug";
            continue;
        }

        // Skip braces
        if (line == "{" || line == "}" || line == "},")
            continue;

        // Parse key-value pairs
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;

        std::string key = Trim(line.substr(0, colonPos));
        std::string value = Trim(line.substr(colonPos + 1));

        // Remove trailing comma if present
        if (!value.empty() && value.back() == ',')
            value.pop_back();
        value = Trim(value);

        // Apply to appropriate section
        if (currentSection == "graphics")
        {
            if (key == "postProcessEnabled") m_settings.graphics.postProcessEnabled = ParseBool(value);
            else if (key == "bloomEnabled") m_settings.graphics.bloomEnabled = ParseBool(value);
            else if (key == "bloomIntensity") m_settings.graphics.bloomIntensity = ParseFloat(value);
            else if (key == "bloomThreshold") m_settings.graphics.bloomThreshold = ParseFloat(value);
            else if (key == "toneMappingMode") m_settings.graphics.toneMappingMode = ParseInt(value);
            else if (key == "exposure") m_settings.graphics.exposure = ParseFloat(value);
            else if (key == "gamma") m_settings.graphics.gamma = ParseFloat(value);
            else if (key == "ssaoEnabled") m_settings.graphics.ssaoEnabled = ParseBool(value);
            else if (key == "ssaoRadius") m_settings.graphics.ssaoRadius = ParseFloat(value);
            else if (key == "ssaoIntensity") m_settings.graphics.ssaoIntensity = ParseFloat(value);
        }
        else if (currentSection == "display")
        {
            if (key == "vsyncEnabled") m_settings.display.vsyncEnabled = ParseBool(value);
            else if (key == "fullscreen") m_settings.display.fullscreen = ParseBool(value);
            else if (key == "windowWidth") m_settings.display.windowWidth = ParseInt(value);
            else if (key == "windowHeight") m_settings.display.windowHeight = ParseInt(value);
        }
        else if (currentSection == "debug")
        {
            if (key == "wireframeEnabled") m_settings.debug.wireframeEnabled = ParseBool(value);
            else if (key == "debugRenderingEnabled") m_settings.debug.debugRenderingEnabled = ParseBool(value);
        }
    }

    file.close();
    return true;
}

bool Config::Save(const std::string& filepath)
{
    std::ofstream file(filepath);
    if (!file.is_open())
    {
        return false;
    }

    auto boolStr = [](bool v) { return v ? "true" : "false"; };

    file << "{\n";

    // Graphics section
    file << "    \"graphics\": {\n";
    file << "        \"postProcessEnabled\": " << boolStr(m_settings.graphics.postProcessEnabled) << ",\n";
    file << "        \"bloomEnabled\": " << boolStr(m_settings.graphics.bloomEnabled) << ",\n";
    file << "        \"bloomIntensity\": " << m_settings.graphics.bloomIntensity << ",\n";
    file << "        \"bloomThreshold\": " << m_settings.graphics.bloomThreshold << ",\n";
    file << "        \"toneMappingMode\": " << m_settings.graphics.toneMappingMode << ",\n";
    file << "        \"exposure\": " << m_settings.graphics.exposure << ",\n";
    file << "        \"gamma\": " << m_settings.graphics.gamma << ",\n";
    file << "        \"ssaoEnabled\": " << boolStr(m_settings.graphics.ssaoEnabled) << ",\n";
    file << "        \"ssaoRadius\": " << m_settings.graphics.ssaoRadius << ",\n";
    file << "        \"ssaoIntensity\": " << m_settings.graphics.ssaoIntensity << "\n";
    file << "    },\n";

    // Display section
    file << "    \"display\": {\n";
    file << "        \"vsyncEnabled\": " << boolStr(m_settings.display.vsyncEnabled) << ",\n";
    file << "        \"fullscreen\": " << boolStr(m_settings.display.fullscreen) << ",\n";
    file << "        \"windowWidth\": " << m_settings.display.windowWidth << ",\n";
    file << "        \"windowHeight\": " << m_settings.display.windowHeight << "\n";
    file << "    },\n";

    // Debug section
    file << "    \"debug\": {\n";
    file << "        \"wireframeEnabled\": " << boolStr(m_settings.debug.wireframeEnabled) << ",\n";
    file << "        \"debugRenderingEnabled\": " << boolStr(m_settings.debug.debugRenderingEnabled) << "\n";
    file << "    }\n";

    file << "}\n";

    file.close();
    return true;
}
