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
        e.AddMember("id", static_cast<uint64_t>(id), a);

        Value comps(kObjectType);

        for (auto& name : ecs.GetComponentNames(id)) {
            // Transform
            if (name == "Transform" && ecs.HasComponent<Ermine::Transform>(id)) {
                Value t(kObjectType);
                ecs.GetComponent<Ermine::Transform>(id).Serialize(t, a);
                comps.AddMember(Value("Transform", a), t, a);
            }

            // ObjectMetaData
            if (name == "ObjectMetaData" && ecs.HasComponent<Ermine::ObjectMetaData>(id)) {
                Value m(kObjectType);
                ecs.GetComponent<Ermine::ObjectMetaData>(id).Serialize(m, a);
                comps.AddMember(Value("ObjectMetaData", a), m, a);
            }

            // Light
            if (name == "Light" && ecs.HasComponent<Ermine::Light>(id)) {
                Value l(kObjectType);
                ecs.GetComponent<Ermine::Light>(id).Serialize(l, a);
                comps.AddMember(Value("Light", a), l, a);
            }

            // Mesh
            if (name == "Mesh" && ecs.HasComponent<Ermine::Mesh>(id)) {
                Value m(kObjectType);
                ecs.GetComponent<Ermine::Mesh>(id).Serialize(m, a);
                comps.AddMember(Value("Mesh", a), m, a);
            }

            // Material
            if (name == "Material" && ecs.HasComponent<Ermine::Material>(id)) {
                Value m(kObjectType);
                ecs.GetComponent<Ermine::Material>(id).Serialize(m, a);
                comps.AddMember(Value("Material", a), m, a);
            }

			// ModelComponent
            if (name == "ModelComponent" && ecs.HasComponent<Ermine::ModelComponent>(id)) {
                Value l(kObjectType);
                ecs.GetComponent<Ermine::ModelComponent>(id).Serialize(l, a);
                comps.AddMember(Value("ModelComponent", a), l, a);
            }

            // PhysicsComponent
            if (name == "PhysicComponent" && ecs.HasComponent<Ermine::PhysicComponent>(id)) {
                Value l(kObjectType);
                ecs.GetComponent<Ermine::PhysicComponent>(id).Serialize(l, a);
                comps.AddMember(Value("PhysicComponent", a), l, a);
            }

            // AudioComponent
            if (name == "AudioComponent" && ecs.HasComponent<Ermine::AudioComponent>(id)) {
                Value l(kObjectType);
                ecs.GetComponent<Ermine::AudioComponent>(id).Serialize(l, a);
                comps.AddMember(Value("AudioComponent", a), l, a);
            }

            // GlobalAudioComponent
            if (name == "GlobalAudioComponent" && ecs.HasComponent<Ermine::GlobalAudioComponent>(id)) {
                Value l(kObjectType);
                ecs.GetComponent<Ermine::GlobalAudioComponent>(id).Serialize(l, a);
                comps.AddMember(Value("GlobalAudioComponent", a), l, a);
            }

            // AnimationComponent
            if (name == "AnimationComponent" && ecs.HasComponent<Ermine::AnimationComponent>(id)) {
                Value l(kObjectType);
                ecs.GetComponent<Ermine::AnimationComponent>(id).Serialize(l, a);
                comps.AddMember(Value("AnimationComponent", a), l, a);
            }

            // Particle
            if (name == "Particle" && ecs.HasComponent<Ermine::Particle>(id)) {
                Value l(kObjectType);
                ecs.GetComponent<Ermine::Particle>(id).Serialize(l, a);
                comps.AddMember(Value("Particle", a), l, a);
            }

            // HierarchyComponent
            if (name == "HierarchyComponent" && ecs.HasComponent<Ermine::HierarchyComponent>(id)) {
                Value l(kObjectType);
                ecs.GetComponent<Ermine::HierarchyComponent>(id).Serialize(l, a);
                comps.AddMember(Value("HierarchyComponent", a), l, a);
            }
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

    for (auto& e : d["entities"].GetArray()) {
        if (!e.IsObject()) continue;

        Ermine::EntityID id = ecs.CreateEntity();

        if (!e.HasMember("components") || !e["components"].IsObject())
            continue;

        const auto& comps = e["components"];

        // Transform
        if (comps.HasMember("Transform") && comps["Transform"].IsObject()) {
            if (!ecs.HasComponent<Ermine::Transform>(id))
                ecs.AddComponent<Ermine::Transform>(id, Ermine::Transform{});

            auto& c = ecs.GetComponent<Ermine::Transform>(id);
            c.Deserialize(comps["Transform"]);
        }

        // ObjectMetaData
        if (comps.HasMember("ObjectMetaData") && comps["ObjectMetaData"].IsObject()) {
            if (!ecs.HasComponent<Ermine::ObjectMetaData>(id))
                ecs.AddComponent<Ermine::ObjectMetaData>(id, Ermine::ObjectMetaData{});

            auto& m = ecs.GetComponent<Ermine::ObjectMetaData>(id);
            m.Deserialize(comps["ObjectMetaData"]);
        }

        // Light
        if (comps.HasMember("Light") && comps["Light"].IsObject()) {
            if (!ecs.HasComponent<Ermine::Light>(id))
                ecs.AddComponent<Ermine::Light>(id, Ermine::Light{});

            auto& l = ecs.GetComponent<Ermine::Light>(id);
            l.Deserialize(comps["Light"]);
        }

        // Mesh
        if (comps.HasMember("Mesh") && comps["Mesh"].IsObject()) {
            if (!ecs.HasComponent<Ermine::Mesh>(id))
                ecs.AddComponent<Ermine::Mesh>(id, Ermine::Mesh{});

            auto& m = ecs.GetComponent<Ermine::Mesh>(id);
            m.Deserialize(comps["Mesh"]);
        }

        // Material
        if (comps.HasMember("Material") && comps["Material"].IsObject()) {
            if (!ecs.HasComponent<Ermine::Material>(id))
                ecs.AddComponent<Ermine::Material>(id, Ermine::Material{});

            auto& m = ecs.GetComponent<Ermine::Material>(id);
            m.Deserialize(comps["Material"]);
        }

        // ModelComponent
        if (comps.HasMember("ModelComponent") && comps["ModelComponent"].IsObject()) {
            if (!ecs.HasComponent<Ermine::ModelComponent>(id))
                ecs.AddComponent<Ermine::ModelComponent>(id, Ermine::ModelComponent{});

            auto& m = ecs.GetComponent<Ermine::ModelComponent>(id);
            m.Deserialize(comps["ModelComponent"]);
        }

        // PhysicComponent
        if (comps.HasMember("PhysicComponent") && comps["PhysicComponent"].IsObject()) {
            if (!ecs.HasComponent<Ermine::PhysicComponent>(id))
                ecs.AddComponent<Ermine::PhysicComponent>(id, Ermine::PhysicComponent{});

            auto& m = ecs.GetComponent<Ermine::PhysicComponent>(id);
            m.Deserialize(comps["PhysicComponent"]);
        }

        // AudioComponent
        if (comps.HasMember("AudioComponent") && comps["AudioComponent"].IsObject()) {
            if (!ecs.HasComponent<Ermine::AudioComponent>(id))
                ecs.AddComponent<Ermine::AudioComponent>(id, Ermine::AudioComponent{});

            auto& m = ecs.GetComponent<Ermine::AudioComponent>(id);
            m.Deserialize(comps["AudioComponent"]);
        }

        // GlobalAudioComponent
        if (comps.HasMember("GlobalAudioComponent") && comps["GlobalAudioComponent"].IsObject()) {
            if (!ecs.HasComponent<Ermine::GlobalAudioComponent>(id))
                ecs.AddComponent<Ermine::GlobalAudioComponent>(id, Ermine::GlobalAudioComponent{});

            auto& m = ecs.GetComponent<Ermine::GlobalAudioComponent>(id);
            m.Deserialize(comps["GlobalAudioComponent"]);
        }

        // AnimationComponent
        if (comps.HasMember("AnimationComponent") && comps["AnimationComponent"].IsObject()) {
            if (!ecs.HasComponent<Ermine::AnimationComponent>(id))
                ecs.AddComponent<Ermine::AnimationComponent>(id, Ermine::AnimationComponent{});

            auto& m = ecs.GetComponent<Ermine::AnimationComponent>(id);
            m.Deserialize(comps["AnimationComponent"]);
        }

        // Particle
        if (comps.HasMember("Particle") && comps["Particle"].IsObject()) {
            if (!ecs.HasComponent<Ermine::Particle>(id))
                ecs.AddComponent<Ermine::Particle>(id, Ermine::Particle{});

            auto& m = ecs.GetComponent<Ermine::Particle>(id);
            m.Deserialize(comps["Particle"]);
        }

        // HierarchyComponent 
        if (comps.HasMember("HierarchyComponent") && comps["HierarchyComponent"].IsObject()) {
            if (!ecs.HasComponent<Ermine::HierarchyComponent>(id))
                ecs.AddComponent<Ermine::HierarchyComponent>(id, Ermine::HierarchyComponent{});

            auto& m = ecs.GetComponent<Ermine::HierarchyComponent>(id);
            m.Deserialize(comps["HierarchyComponent"]);
        }

        // (If you later add more components, repeat this pattern.)
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