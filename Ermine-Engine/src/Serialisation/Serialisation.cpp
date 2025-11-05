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
            const auto* desc = ecs.GetDescriptor(name);

            rapidjson::Value payload(rapidjson::kObjectType);
            bool wrote = false;

            // Prefer the generic serializer if present
            if (desc && desc->serialize)
            {
                desc->serialize(id, payload, a);
                wrote = true;
            }
            else
            {
                // --- Custom fallbacks ---
                //if (name == "IDComponent" && ecs.HasComponent<Ermine::IDComponent>(id))
                //{
                //    const auto& c = ecs.GetComponent<Ermine::IDComponent>(id);
                //    const std::string guid_str = c.guid.ToString();
                //    payload.AddMember(rapidjson::Value("guid", a),
                //        rapidjson::Value(guid_str.c_str(), a), a);
                //    wrote = true;
                //}
                if (name == "Script" && ecs.HasComponent<Ermine::Script>(id))
                {
                    const auto& s = ecs.GetComponent<Ermine::Script>(id);
                    payload.AddMember(rapidjson::Value("class", a),
                        rapidjson::Value(s.m_className.c_str(), a), a);
                    // TODO: add more script state here if you later expose it
                    wrote = true;
                }
            }

            // Only write if we actually produced a payload
            if (wrote)
            {
                comps.AddMember(rapidjson::Value(name.c_str(), a), payload, a);
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
            bool handled = false;

            // Prefer the generic deserializer if present
            if (desc && desc->deserialize)
            {
                desc->deserialize(id, payload);
                handled = true;

                // If the generic path handled IDComponent, keep your registry hookup:
                if (compName == "IDComponent")
                {
                    auto& c = ecs.GetComponent<Ermine::IDComponent>(id);
                    ecs.GetGuidRegistry().Register(id, c.guid);
                }
            }
            else
            {
                // --- Custom fallbacks ---
                //if (compName == "IDComponent")
                //{
                //    Ermine::Guid g =
                //        (payload.HasMember("guid") && payload["guid"].IsString())
                //        ? Ermine::Guid::FromString(payload["guid"].GetString())
                //        : Ermine::Guid::New(); // backward-compatible

                //    ecs.AddComponent<Ermine::IDComponent>(id, Ermine::IDComponent{ g });
                //    ecs.GetGuidRegistry().Register(id, g);
                //    handled = true;
                //}
                if (compName == "Script")
                {
                    if (payload.HasMember("class") && payload["class"].IsString())
                    {
                        const std::string cls = payload["class"].GetString();
                        ecs.AddComponent<Ermine::Script>(id, Ermine::Script(cls, id));
                        // TODO: post-load hook if you have one, e.g. ScriptSystem::OnAdded(id);
                        handled = true;
                    }
                }
            }

            if (!handled)
            {
                EE_CORE_WARN("Unknown or non-deserializable component '{}'; skipping.", compName.c_str());
            }
        }
    }

    ecs.ResyncAllSignaturesFromStorage();

    // Setup model mesh children and link entityIDs after all entities are loaded
    auto modelSystem = ecs.GetSystem<Ermine::graphics::ModelSystem>();
    auto hierarchySystem = ecs.GetSystem<Ermine::HierarchySystem>();

    if (modelSystem && hierarchySystem && renderer) {
        EE_CORE_INFO("Setting up model mesh children for {} entities", modelSystem->m_Entities.size());

        for (auto entity : modelSystem->m_Entities) {
            if (!ecs.HasComponent<Ermine::ModelComponent>(entity)) continue;

            auto& modelComp = ecs.GetComponent<Ermine::ModelComponent>(entity);
            if (!modelComp.m_model) continue;

            const auto& model = modelComp.m_model;
            const aiScene* scene = model->GetAssimpScene();
            if (!scene) {
                EE_CORE_WARN("Model for entity {} has no aiScene", entity);
                continue;
            }

            const auto& meshes = model->GetMeshes();
            EE_CORE_INFO("Entity {}: Model '{}' has {} meshes", entity, model->GetName(), meshes.size());

            int childrenProcessed = 0;
            for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
                const auto& meshData = meshes[meshIndex];
                const std::string& meshID = meshData.meshID;

                // Get material index from aiScene
                if (meshIndex >= scene->mNumMeshes) {
                    EE_CORE_WARN("Mesh index {} >= scene->mNumMeshes {}", meshIndex, scene->mNumMeshes);
                    continue;
                }
                aiMesh* aiMsh = scene->mMeshes[meshIndex];
                if (!aiMsh) {
                    EE_CORE_WARN("aiMesh at index {} is null", meshIndex);
                    continue;
                }
                uint32_t matIndex = aiMsh->mMaterialIndex;
                if (matIndex >= scene->mNumMaterials) {
                    EE_CORE_WARN("Material index {} >= scene->mNumMaterials {}, skipping mesh {}", matIndex, scene->mNumMaterials, meshID);
                    continue;
                }

                // Find or create child entity with matching name
                Ermine::EntityID childEntity = 0;
                const std::string expectedChildName = "Mesh_" + meshID;

                if (ecs.HasComponent<Ermine::HierarchyComponent>(entity)) {
                    auto& hierarchy = ecs.GetComponent<Ermine::HierarchyComponent>(entity);
                    for (Ermine::EntityID child : hierarchy.children) {
                        if (ecs.HasComponent<Ermine::ObjectMetaData>(child)) {
                            auto& metadata = ecs.GetComponent<Ermine::ObjectMetaData>(child);
                            if (metadata.name == expectedChildName) {
                                childEntity = child;
                                break;
                            }
                        }
                    }
                }

                // Create child if it doesn't exist
                if (childEntity == 0) {
                    childEntity = ecs.CreateEntity();
                    ecs.AddComponent<Ermine::HierarchyComponent>(childEntity, Ermine::HierarchyComponent());
                    ecs.AddComponent<Ermine::Transform>(childEntity, Ermine::Transform());
                    ecs.AddComponent<Ermine::ObjectMetaData>(childEntity, Ermine::ObjectMetaData(expectedChildName, "Mesh", true));
                    hierarchySystem->SetParent(childEntity, entity, true);
                    EE_CORE_INFO("Created new child entity {} for mesh {}", childEntity, meshID);
                }
                else {
                    EE_CORE_INFO("Found existing child entity {} for mesh {}, reloading material", childEntity, meshID);
                }

                // Only create material if child doesn't already have one from the scene file
                if (!ecs.HasComponent<Ermine::Material>(childEntity)) {
                    aiMaterial* aiMat = scene->mMaterials[matIndex];
                    auto materialPtr = std::make_shared<Ermine::graphics::Material>();
                    // Don't load template - start with empty material

                    aiString texPath;

                    // Albedo
                    if (aiMat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS ||
                        aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                        std::string texPathStr = std::string(texPath.C_Str());
                        std::replace(texPathStr.begin(), texPathStr.end(), '\\', '/');
                        auto albedoTex = Ermine::AssetManager::GetInstance().LoadTexture("../Resources/Textures/" + texPathStr);
                        if (albedoTex) {
                            materialPtr->SetTexture("materialAlbedoMap", albedoTex);
                            materialPtr->SetBool("materialHasAlbedoMap", true);
                        }
                    }

                    // Normal
                    if (aiMat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
                        std::string texPathStr = std::string(texPath.C_Str());
                        std::replace(texPathStr.begin(), texPathStr.end(), '\\', '/');
                        auto normalTex = Ermine::AssetManager::GetInstance().LoadTexture("../Resources/Textures/" + texPathStr);
                        if (normalTex) {
                            materialPtr->SetTexture("materialNormalMap", normalTex);
                            materialPtr->SetBool("materialHasNormalMap", true);
                        }
                    }

                    // Roughness
                    if (aiMat->GetTexture(aiTextureType_SHININESS, 0, &texPath) == AI_SUCCESS) {
                        std::string texPathStr = std::string(texPath.C_Str());
                        std::replace(texPathStr.begin(), texPathStr.end(), '\\', '/');
                        auto roughnessTex = Ermine::AssetManager::GetInstance().LoadTexture("../Resources/Textures/" + texPathStr);
                        if (roughnessTex) {
                            materialPtr->SetTexture("materialRoughnessMap", roughnessTex);
                            materialPtr->SetBool("materialHasRoughnessMap", true);
                        }
                    }

                    // Metallic
                    if (aiMat->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS) {
                        std::string texPathStr = std::string(texPath.C_Str());
                        std::replace(texPathStr.begin(), texPathStr.end(), '\\', '/');
                        auto metallicTex = Ermine::AssetManager::GetInstance().LoadTexture("../Resources/Textures/" + texPathStr);
                        if (metallicTex) {
                            materialPtr->SetTexture("materialMetallicMap", metallicTex);
                            materialPtr->SetBool("materialHasMetallicMap", true);
                        }
                    }

                    // UV transform with V-flip
                    aiUVTransform uvTransform;
                    if (aiMat->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, 0), uvTransform) == AI_SUCCESS) {
                        materialPtr->SetUVScale(Ermine::Vec2(uvTransform.mScaling.x, -uvTransform.mScaling.y));
                        materialPtr->SetUVOffset(Ermine::Vec2(uvTransform.mTranslation.x, 1.0f - uvTransform.mTranslation.y));
                    }
                    else {
                        materialPtr->SetUVScale(Ermine::Vec2(1.0f, -1.0f));
                        materialPtr->SetUVOffset(Ermine::Vec2(0.0f, 1.0f));
                    }

                    // Add new material component
                    ecs.AddComponent<Ermine::Material>(childEntity, Ermine::Material(materialPtr));
                    EE_CORE_INFO("Added material to child entity {} (not in scene file)", childEntity);
                }
                else {
                    EE_CORE_INFO("Child entity {} already has material from scene file, preserving it", childEntity);
                }

                childrenProcessed++;
            }

            EE_CORE_INFO("Processed {} children for entity {}", childrenProcessed, entity);
        }
    }
    else {
        if (!modelSystem) EE_CORE_ERROR("ModelSystem is null during scene load");
        if (!hierarchySystem) EE_CORE_ERROR("HierarchySystem is null during scene load");
        if (!renderer) EE_CORE_ERROR("Renderer is null during scene load");
    }

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
        return {};
    }

    std::filesystem::path norm = std::filesystem::weakly_canonical(path);
    std::ifstream ifs(norm, std::ios::binary);
    if (!ifs) throw std::runtime_error("Could not open file for reading: " + path.string());

    IStreamWrapper isw(ifs);
    rapidjson::Document d; d.ParseStream(isw);
    if (d.HasParseError() || !d.IsObject())
        throw std::runtime_error("Invalid JSON file: " + path.string());

    // Accept either { "entity": { ... } } or a flat object
    const rapidjson::Value* root = &d;
    if (d.HasMember("entity") && d["entity"].IsObject()) root = &d["entity"];

    if (!root->HasMember("components") || !(*root)["components"].IsObject())
        throw std::runtime_error("Invalid prefab JSON (missing 'components'): " + path.string());

    // Create a fresh entity for the prefab instance
    Ermine::EntityID id = ecs.CreateEntity();
    const rapidjson::Value& comps = (*root)["components"];

    // Load all components. Prefer generic descriptor; otherwise custom Script fallback.
    for (auto it = comps.MemberBegin(); it != comps.MemberEnd(); ++it)
    {
        if (!it->value.IsObject()) continue;
        const char* compName = it->name.GetString();

        if (const auto* desc = ecs.GetDescriptor(compName); desc && desc->deserialize)
        {
            desc->deserialize(id, it->value);
        }
        else if (std::strcmp(compName, "Script") == 0)
        {
            // Fallback for Script: expect { "class": "<ClassName>" }
            const rapidjson::Value& payload = it->value;
            if (payload.HasMember("class") && payload["class"].IsString())
            {
                const std::string cls = payload["class"].GetString();
                ecs.AddComponent<Ermine::Script>(id, Ermine::Script(cls, id));
            }
            else
            {
                EE_CORE_WARN("Prefab Script missing 'class' string; skipping.");
            }
        }
        else
        {
            EE_CORE_WARN("Prefab component '{}' has no deserializer; skipping.", compName);
        }
    }

    // Ensure signatures are up to date
    ecs.ResyncAllSignaturesFromStorage();
    return id;
}



void SavePrefabToFile(const Ermine::ECS& ecs, Ermine::EntityID id, const std::filesystem::path& path)
{
    if (path.has_parent_path())
    {
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

    for (const std::string& name : ecs.GetComponentNames(id))
    {
        const auto* desc = ecs.GetDescriptor(name);
        rapidjson::Value payload(rapidjson::kObjectType);
        bool wrote = false;

        if (desc && desc->serialize)
        {
            desc->serialize(id, payload, a);
            wrote = true;
        }
        else if (name == "Script" && ecs.HasComponent<Ermine::Script>(id))
        {
            const auto& s = ecs.GetComponent<Ermine::Script>(id);
            payload.AddMember(rapidjson::Value("class", a),
                rapidjson::Value(s.m_className.c_str(), a), a);
            wrote = true;
        }

        if (wrote)
            comps.AddMember(rapidjson::Value(name.c_str(), a), payload, a);
        else
            EE_CORE_WARN("Prefab save: component '{}' has no serializer; skipping.", name.c_str());
    }

    e.AddMember("components", comps, a);
    d.AddMember("entity", e, a);

    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> w(osw);
    w.SetIndent(' ', 2);
    d.Accept(w);
}
