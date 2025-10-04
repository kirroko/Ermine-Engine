/* Start Header ************************************************************************/
/*!
\file       Serialisation.h
\author     WEE HONG RU Curtis, h.wee, 2301266, h.wee\@digipen.edu
\date       Sep 10, 2025
\brief      Serialisation functions for Config and Scene

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#ifndef SERIALISATION_H
#define SERIALISATION_H

#include "PreCompile.h"
#include <filesystem>
#include <string>
#include "ECS.h"

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

void SaveSceneToFile(const Ermine::ECS& ecs, const std::filesystem::path& path, bool pretty);
void LoadSceneFromFile(Ermine::ECS& ecs, const std::filesystem::path& path);

void SaveCurrentScene(const std::string& sceneName);
void LoadScene(const std::string& sceneName);

#endif // SERIALISATION_H
