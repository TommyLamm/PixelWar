#include "Settings.h"
#include <fstream>
#include <iostream>
#include <sstream>

GameSettings Settings::Load(const std::string& filename)
{
    GameSettings settings;
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cout << "[Settings] Config file not found, using defaults" << std::endl;
        return settings;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '='))
        {
            std::string valueStr;
            if (std::getline(iss, valueStr))
            {
                try {
                    float value = std::stof(valueStr);
                    if (key == "sensitivity") settings.sensitivity = value;
                    else if (key == "fov") settings.fov = value;
                } catch (...) {}
            }
        }
    }
    std::cout << "[Settings] Loaded: Sens=" << settings.sensitivity << ", FOV=" << settings.fov << std::endl;
    return settings;
}

void Settings::Save(const std::string& filename, const GameSettings& settings)
{
    std::ofstream file(filename);
    if (file.is_open())
    {
        file << "sensitivity=" << settings.sensitivity << "\n";
        file << "fov=" << settings.fov << "\n";
        std::cout << "[Settings] Saved" << std::endl;
    }
}
