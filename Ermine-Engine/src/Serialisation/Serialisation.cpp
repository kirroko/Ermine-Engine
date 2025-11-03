/* Start Header ************************************************************************/
/*!
\file       Serialisation.cpp
\author     WEE HONG RU Curtis, h.wee, 2301266, h.wee\@digipen.edu
\date       Sep 10, 2025
\brief      Serialisation functions for Config and Scene

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Serialisation.h"
#include "Entity.h"
#include "Components.h"
#include "Renderer.h"
#include "MeshTypes.h"

#include <document.h>
#include <writer.h>
#include <prettywriter.h>
#include <stringbuffer.h>
#include <ostreamwrapper.h>
#include <istreamwrapper.h>
#include "GeometryFactory.h"


using namespace rapidjson;

using Ermine::graphics::GeometryFactory; // make intent explicit

void Ermine::Mesh::RebuildPrimitive() {
    if (primitive.type == "Cube") {
        *this = GeometryFactory::CreateCube(primitive.size.x, primitive.size.y, primitive.size.z);
        kind = MeshKind::Primitive;
    }
    else if (primitive.type == "Sphere") {
        *this = GeometryFactory::CreateSphere(primitive.size.x); // adapt to your API
        kind = MeshKind::Primitive;
    }
    else if (primitive.type == "Quad") {
        *this = GeometryFactory::CreateQuad(primitive.size.x, primitive.size.y); // adapt to your API
        kind = MeshKind::Primitive;
    }
    else {
        EE_CORE_WARN("Unknown primitive type: {}", primitive.type);
        kind = MeshKind::None;
	}
    // TODO: other primitives...
}

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


void SaveSceneToFile(const Ermine::ECS& ecs, const std::filesystem::path& path, bool pretty) {

    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            throw std::runtime_error("Failed to create directory: " + path.parent_path().string());
        }
    }

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) throw std::runtime_error("Could not open file for writing: " + path.string());
    OStreamWrapper osw(ofs);

    Document d; d.SetObject();
    auto& a = d.GetAllocator();

    Value entities(kArrayType);

    for (Ermine::EntityID id = 0; id < Ermine::MAX_ENTITIES; ++id) {
        if (!ecs.IsEntityValid(id)) continue;

        Value e(kObjectType);
        e.AddMember("id", id, a);

        Value comps(kObjectType);


        for (const std::string& name : ecs.GetComponentNames(id)) {                 // :contentReference[oaicite:1]{index=1}
            const auto* desc = ecs.GetDescriptor(name);         // :contentReference[oaicite:2]{index=2}
            if (!desc || !desc->serialize) continue;

            rapidjson::Value payload(rapidjson::kObjectType);
            desc->serialize(id, payload, a);  // <- no ECS here
            comps.AddMember(rapidjson::Value(name.c_str(), a), payload, a);
        }


        e.AddMember("components", comps, a);
        entities.PushBack(e, a);
    }

    d.AddMember("entities", entities, a);

    if (pretty) { PrettyWriter<OStreamWrapper> w(osw); w.SetIndent(' ', 2); d.Accept(w); }
    else { Writer<OStreamWrapper> w(osw); d.Accept(w); }
}


void LoadSceneFromFile(Ermine::ECS& ecs, const std::filesystem::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Could not open file for reading: " + path.string());

    IStreamWrapper isw(ifs);
    Document d; d.ParseStream(isw);
    if (d.HasParseError() || !d.IsObject())
        throw std::runtime_error("Invalid JSON file: " + path.string());

    // Defensive: ensure we have an "entities" array
    if (!d.HasMember("entities") || !d["entities"].IsArray())
        throw std::runtime_error("Invalid scene JSON (missing 'entities'): " + path.string());

    // Clear MeshManager for new scene
    auto renderer = ecs.GetSystem<Ermine::graphics::Renderer>();
    if (renderer) {
        renderer->m_MeshManager.Clear();
    }

    Ermine::AssetManager::GetInstance().ClearModelCache();

    for (auto& e : d["entities"].GetArray()) {
        if (!e.IsObject()) continue;

        Ermine::EntityID id = ecs.CreateEntity();

        if (!e.HasMember("components") || !e["components"].IsObject())
            continue;

        const auto& comps = e["components"];

        for (auto it = comps.MemberBegin(); it != comps.MemberEnd(); ++it) {
            if (!it->value.IsObject()) continue;

            const std::string compName = it->name.GetString();
            const rapidjson::Value& payload = it->value;

            // Look up the component descriptor and call its type-erased deserializer
            const auto* desc = ecs.GetDescriptor(compName);
            if (!desc || !desc->deserialize) {
                EE_CORE_WARN("Unknown or non-deserializable component '{}'; skipping.", compName.c_str());
                continue;
            }

            desc->deserialize(id, payload);

            // Keep your GUID registration behavior
            if (compName == "IDComponent") {
                auto& c = ecs.GetComponent<Ermine::IDComponent>(id);
                ecs.GetGuidRegistry().Register(id, c.guid);
            }
        }
    }

    ecs.ResyncAllSignaturesFromStorage();

    // Upload all registered meshes to GPU and build indirect draw commands
    if (renderer) {
        renderer->m_MeshManager.UploadAndBuild();
        EE_CORE_INFO("Scene loaded: MeshManager populated with {} meshes",
                     renderer->m_MeshManager.GetMeshCount());
    }
}

void SaveCurrentScene(const std::string& sceneName)
{
    // "Level01" = Resources/Scenes/Level01.scene
    filesystem::path scenePath = filesystem::path("Resources") / "Scenes" / (sceneName + ".scene");

    SaveSceneToFile(Ermine::ECS::GetInstance(), scenePath, true);
}

void LoadScene(const std::string& sceneName)
{
    filesystem::path scenePath = filesystem::path("Resources") / "Scenes" / (sceneName + ".scene");

    LoadSceneFromFile(Ermine::ECS::GetInstance(), scenePath);
}

Ermine::EntityID LoadPrefabFromFile(Ermine::ECS& ecs, const std::filesystem::path& path)
{
    if (!path.has_extension() || path.extension() != ".prefab")
    {
        EE_CORE_ERROR("LoadPrefabFromFile rejected non-prefab file: {}", path.string());
        return {}; // or return {}; or throw, your style
    }

    std::filesystem::path norm = std::filesystem::weakly_canonical(path);
    //EE_CORE_INFO("Normalized prefab path = {}", norm.string());
    std::ifstream ifs(norm, std::ios::binary);
    if (!ifs) throw std::runtime_error("Could not open file for reading: " + path.string());

    IStreamWrapper isw(ifs);
    rapidjson::Document d; d.ParseStream(isw);
    if (d.HasParseError() || !d.IsObject())
        throw std::runtime_error("Invalid JSON file: " + path.string());

    // Accept either { "entity": { "id", "components" } } or { "id", "components" }
    const rapidjson::Value* root = &d;
    if (d.HasMember("entity") && d["entity"].IsObject()) root = &d["entity"];

    if (!root->HasMember("components") || !(*root)["components"].IsObject())
        throw std::runtime_error("Invalid prefab JSON (missing 'components'): " + path.string());

    // Create a fresh entity for the prefab instance
    Ermine::EntityID id = ecs.CreateEntity();
    const rapidjson::Value& comps = (*root)["components"];

    // 1) Load IDComponent first (if present), then register GUID
    if (auto it = comps.FindMember("IDComponent");
        it != comps.MemberEnd() && it->value.IsObject())
    {
        if (const auto* desc = ecs.GetDescriptor("IDComponent"); desc && desc->deserialize) {
            desc->deserialize(id, it->value);
            auto& c = ecs.GetComponent<Ermine::IDComponent>(id);
            ecs.GetGuidRegistry().Register(id, c.guid);
        }
    }

    // 2) Load remaining components generically (any order is fine for a prefab)
    for (auto it = comps.MemberBegin(); it != comps.MemberEnd(); ++it) {
        if (!it->value.IsObject()) continue;
        const char* compName = it->name.GetString();
        if (std::strcmp(compName, "IDComponent") == 0) continue;

        if (const auto* desc = ecs.GetDescriptor(compName); desc && desc->deserialize) {
            desc->deserialize(id, it->value);
        }
    }

    // Recompute signatures so systems see this new entity
    ecs.ResyncAllSignaturesFromStorage();

    return id;
}


void SavePrefabToFile(const Ermine::ECS& ecs, Ermine::EntityID id, const std::filesystem::path& path)
{
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) throw std::runtime_error("Failed to create directory: " + path.parent_path().string());
    }

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) throw std::runtime_error("Could not open file for writing: " + path.string());
    rapidjson::OStreamWrapper osw(ofs);

    rapidjson::Document d; d.SetObject();
    auto& a = d.GetAllocator();

    rapidjson::Value e(rapidjson::kObjectType);
    e.AddMember("id", id, a);

    rapidjson::Value comps(rapidjson::kObjectType);

    for (const std::string& name : ecs.GetComponentNames(id)) {
        const auto* desc = ecs.GetDescriptor(name);
        if (!desc || !desc->serialize) continue;

        rapidjson::Value payload(rapidjson::kObjectType);
        desc->serialize(id, payload, a);
        comps.AddMember(rapidjson::Value(name.c_str(), a), payload, a);
    }

    e.AddMember("components", comps, a);
    d.AddMember("entity", e, a);

    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> w(osw);
    w.SetIndent(' ', 2);
    d.Accept(w);
}
