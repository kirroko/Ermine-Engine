#include "PreCompile.h"
#include "Serialisation.h"
#include <document.h>
#include <writer.h>
#include <prettywriter.h>
#include <stringbuffer.h>
#include <ostreamwrapper.h>
#include <istreamwrapper.h>

#include "GameObjectFactory.h"

using namespace rapidjson;

std::string SerializeConfig(const Config& config) {
    Document d;
    d.SetObject();
    auto& allocator = d.GetAllocator();

    d.AddMember("windowWidth", config.windowWidth, allocator);
    d.AddMember("windowHeight", config.windowHeight, allocator);
    d.AddMember("fullscreen", config.fullscreen, allocator);
    d.AddMember("maximized", config.maximized, allocator);
    d.AddMember("title", Value(config.title.c_str(), allocator), allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);

    return buffer.GetString();
}

Config DeserializeConfig(const std::string& jsonStr) {
    Document d;
    d.Parse(jsonStr.c_str());

    if (d.HasParseError() || !d.IsObject()) {
        throw std::runtime_error("Invalid JSON string");
    }

    Config config{};
    if (d.HasMember("windowWidth") && d["windowWidth"].IsInt())
        config.windowWidth = d["windowWidth"].GetInt();
    if (d.HasMember("windowHeight") && d["windowHeight"].IsInt())
        config.windowHeight = d["windowHeight"].GetInt();
    if (d.HasMember("fullscreen") && d["fullscreen"].IsBool())
        config.fullscreen = d["fullscreen"].GetBool();
    if (d.HasMember("maximized") && d["maximized"].IsBool())
        config.maximized = d["maximized"].GetBool();
    if (d.HasMember("title") && d["title"].IsString())
        config.title = d["title"].GetString();

    return config;
}

void SaveConfigToFile(const Config& config, const std::filesystem::path& path, bool pretty)
{

    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            throw std::runtime_error("Failed to create directory: " + path.parent_path().string());
        }
    }

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Could not open file for writing: " + path.string());
    }

    rapidjson::OStreamWrapper osw(ofs);
    if (pretty) {
        rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
        rapidjson::Document d; d.SetObject(); auto& a = d.GetAllocator();

        if (!config.maximized)
        {
            d.AddMember("windowWidth", config.windowWidth, a);
            d.AddMember("windowHeight", config.windowHeight, a);
        }
        else
        {
            d.AddMember("windowWidth", 1920, a);
            d.AddMember("windowHeight", 1080, a);
        }

        d.AddMember("fullscreen", config.fullscreen, a);
        d.AddMember("maximized", config.maximized, a);
        d.AddMember("title", rapidjson::Value(config.title.c_str(), a), a);
        d.Accept(writer);
    }
    else {
        rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
        rapidjson::Document d; d.SetObject(); auto& a = d.GetAllocator();
        if (!config.maximized)
        {
            d.AddMember("windowWidth", config.windowWidth, a);
            d.AddMember("windowHeight", config.windowHeight, a);
        }
        else
        {
            d.AddMember("windowWidth", 1920, a);
            d.AddMember("windowHeight", 1080, a);
        }
        d.AddMember("fullscreen", config.fullscreen, a);
        d.AddMember("maximized", config.maximized, a);
        d.AddMember("title", rapidjson::Value(config.title.c_str(), a), a);
        d.Accept(writer);
    }
}

Config LoadConfigFromFile(const std::filesystem::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Could not open file for reading: " + path.string());
    }

    IStreamWrapper isw(ifs);
    Document d;
    d.ParseStream(isw);

    if (d.HasParseError() || !d.IsObject()) {
        throw std::runtime_error("Invalid JSON file: " + path.string());
    }

    Config config{};
    if (d.HasMember("windowWidth") && d["windowWidth"].IsInt())
        config.windowWidth = d["windowWidth"].GetInt();
    if (d.HasMember("windowHeight") && d["windowHeight"].IsInt())
        config.windowHeight = d["windowHeight"].GetInt();
    if (d.HasMember("fullscreen") && d["fullscreen"].IsBool())
        config.fullscreen = d["fullscreen"].GetBool();
    if (d.HasMember("maximized") && d["maximized"].IsBool())
        config.maximized = d["maximized"].GetBool();
    if (d.HasMember("title") && d["title"].IsString())
        config.title = d["title"].GetString();

    return config;
}

//void SaveSceneToFile(std::string name, const std::filesystem::path& path, bool pretty)
//{
//    if (path.has_parent_path()) {
//        std::error_code ec;
//        std::filesystem::create_directories(path.parent_path(), ec);
//        if (ec) {
//            throw std::runtime_error("Failed to create directory: " + path.parent_path().string());
//        }
//    }
//
//    std::ofstream ofs(path, std::ios::binary);
//    if (!ofs) {
//        throw std::runtime_error("Could not open file for writing: " + path.string());
//    }
//
//    //for (int i = 0; i < Ermine::object::GameObjectFactory::m_GameObjects.size(); ++i)
//    //{
//    //    //SaveObject(root, GAMEOBJECTFACTORY.GetGameObjects()[i]);
//
//    //    rapidjson::OStreamWrapper osw(ofs);
//    //    if (pretty) {
//    //        rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
//    //        rapidjson::Document d; d.SetObject(); auto& a = d.GetAllocator();
//    //        d.AddMember("name", rapidjson::Value(Ermine::object::GameObjectFactory::m_GameObjects[i], a), a);
//    //        d.AddMember("x", 1, a);
//    //        d.AddMember("y", 2, a);
//    //        d.AddMember("z", 3, a);
//    //        d.Accept(writer);
//    //    }
//    //    else {
//    //        rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
//    //        rapidjson::Document d; d.SetObject(); auto& a = d.GetAllocator();
//    //        d.AddMember("name", rapidjson::Value(name.c_str(), a), a);
//    //        d.AddMember("x", 1, a);
//    //        d.AddMember("y", 2, a);
//    //        d.AddMember("z", 3, a);
//    //        d.Accept(writer);
//    //    }
//    //}
//}

//void LoadSceneFromFile(std::string name, const std::filesystem::path& path)
//{
//    std::ifstream ifs(path, std::ios::binary);
//    if (!ifs) {
//        throw std::runtime_error("Could not open file for reading: " + path.string());
//    }
//
//    IStreamWrapper isw(ifs);
//    Document d;
//    d.ParseStream(isw);
//
//    if (d.HasParseError() || !d.IsObject()) {
//        throw std::runtime_error("Invalid JSON file: " + path.string());
//    }
//
//    Ermine::object::GameObjectFactory::CreateGameObject(d["name"].GetString());
//    //if (d.HasMember("windowWidth") && d["windowWidth"].IsInt())
//    //    config.windowWidth = d["windowWidth"].GetInt();
//    //if (d.HasMember("maximized") && d["maximized"].IsBool())
//    //    config.maximized = d["maximized"].GetBool();
//    //if (d.HasMember("title") && d["title"].IsString())
//    //    config.title = d["title"].GetString();
//
//    //return config;
//}
