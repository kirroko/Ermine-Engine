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

/**
 * @brief Serialise config file
 * @param config struct
 */
std::string SerializeConfig(const Config& config);

/**
 * @brief Deserialise config files
 * @param string file name
 */
Config DeserializeConfig(const std::string& jsonStr);

/**
 * @brief Save config to json file
 * @param config struct
 * @param file path
 * @param prettywriter bool
 */
void SaveConfigToFile(const Config& config, const std::filesystem::path& path, bool pretty = true);

/**
 * @brief Load config from json file
 * @param file path
 */
Config LoadConfigFromFile(const std::filesystem::path& path);

/**
 * @brief Save scene to json file
 * @param ecs reference
 * @param file path
 * @param prettywriter bool
 */
void SaveSceneToFile(const Ermine::ECS& ecs, const std::filesystem::path& path, bool pretty);

/**
 * @brief Load scene from json file
 * @param ecs reference
 * @param file path
 */
void LoadSceneFromFile(Ermine::ECS& ecs, const std::filesystem::path& path);

/**
 * @brief Save current scene calls SaveSceneToFile
 * @param string scene name
 */
void SaveCurrentScene(const std::string& sceneName);

/**
 * @brief Load current scene calls LoadSceneFromFile
 * @param string scene name
 */
void LoadScene(const std::string& sceneName);

#endif // SERIALISATION_H
