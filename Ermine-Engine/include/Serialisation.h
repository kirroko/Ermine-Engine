#ifndef SERIALISATION_H
#define SERIALISATION_H

#include "PreCompile.h"
#include <filesystem>
#include <string>

struct Config {
    int windowWidth{};
    int windowHeight{};
    bool fullscreen{};
    bool maximized{};
    std::string title;
};

std::string SerializeConfig(const Config& config);
Config DeserializeConfig(const std::string& jsonStr);

void SaveConfigToFile(const Config& config, const std::filesystem::path& path, bool pretty = true);
Config LoadConfigFromFile(const std::filesystem::path& path);

//void SaveSceneToFile(std::string name, const std::filesystem::path& path, bool pretty = true);
//void LoadSceneFromFile(std::string name, const std::filesystem::path& path);

#endif // SERIALISATION_H
