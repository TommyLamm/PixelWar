#pragma once
#include <string>

struct GameSettings
{
    float sensitivity = 0.1f;
    float fov = 71.0f;
};

class Settings
{
public:
    static GameSettings Load(const std::string& filename);
    static void Save(const std::string& filename, const GameSettings& settings);
};
