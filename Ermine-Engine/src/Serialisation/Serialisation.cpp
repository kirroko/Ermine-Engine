#include "PreCompile.h"
#include "Serialisation.h"

using namespace rapidjson;

std::string SerializeConfig(const Config& config) {
    Document d;
    d.SetObject();
    Document::AllocatorType& allocator = d.GetAllocator();

    d.AddMember("windowWidth", config.windowWidth, allocator);
    d.AddMember("windowHeight", config.windowHeight, allocator);
    d.AddMember("fullscreen", config.fullscreen, allocator);
    d.AddMember("title", Value(config.title.c_str(), allocator), allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);

    return buffer.GetString();
}

Config DeserializeConfig(const std::string& jsonStr) {
    Config config;

    Document d;
    d.Parse(jsonStr.c_str());

    if (d.HasParseError() || !d.IsObject()) {
        throw std::runtime_error("Invalid JSON");
    }

    if (d.HasMember("windowWidth") && d["windowWidth"].IsInt()) {
        config.windowWidth = d["windowWidth"].GetInt();
    }
    if (d.HasMember("windowHeight") && d["windowHeight"].IsInt()) {
        config.windowHeight = d["windowHeight"].GetInt();
    }
    if (d.HasMember("fullscreen") && d["fullscreen"].IsBool()) {
        config.fullscreen = d["fullscreen"].GetBool();
    }
    if (d.HasMember("title") && d["title"].IsString()) {
        config.title = d["title"].GetString();
    }

    return config;
}
