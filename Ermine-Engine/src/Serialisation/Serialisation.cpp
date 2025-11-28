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
#include "Physics.h"


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
    else if (primitive.type == "Cone") {
        // primitive.size.x stores diameter (radius * 2), so divide by 2 to get actual radius
        // primitive.size.y = height
        *this = GeometryFactory::CreateCone(primitive.size.x / 2.0f, primitive.size.y);
        kind = MeshKind::Primitive;
    }
    else {
        EE_CORE_WARN("Unknown primitive type: {}", primitive.type);
        kind = MeshKind::None;
    }
    // TODO: other primitives...
}

namespace Ermine {
    static inline EntityID FindByGuid(const Guid& g) {
        return ECS::GetInstance().GetGuidRegistry().FindEntity(g);
    }

    void ResolveHierarchyGuids(Ermine::ECS& ecs)
    {
        // 1) Clear runtime links
        for (EntityID id = 0; id < Ermine::MAX_ENTITIES; ++id) {
            if (!ecs.IsEntityValid(id) || !ecs.HasComponent<HierarchyComponent>(id)) continue;
            auto& h = ecs.GetComponent<HierarchyComponent>(id);
            h.parent = HierarchyComponent::INVALID_PARENT;
            h.children.clear();
        }

        // 2) Rebuild from GUIDs
        for (EntityID id = 0; id < Ermine::MAX_ENTITIES; ++id) {
            if (!ecs.IsEntityValid(id) || !ecs.HasComponent<HierarchyComponent>(id)) continue;
            auto& h = ecs.GetComponent<HierarchyComponent>(id);

            if (h.parentGuid.IsValid()) {
                const EntityID p = FindByGuid(h.parentGuid);
                if (ecs.IsEntityValid(p) && ecs.HasComponent<HierarchyComponent>(p)) {
                    h.parent = p;
                    ecs.GetComponent<HierarchyComponent>(p).children.push_back(id);
                }
                else {
                    h.parent = HierarchyComponent::INVALID_PARENT; // dangling -> root
                }
            }

            // Optional symmetry (not strictly required if parent is authoritative)
            for (const Guid& cg : h.childrenGuids) {
                const EntityID cid = FindByGuid(cg);
                if (ecs.IsEntityValid(cid) && ecs.HasComponent<HierarchyComponent>(cid)) {
                    // ensure child has me as parent
                    ecs.GetComponent<HierarchyComponent>(cid).parent = id;
                    h.children.push_back(cid);
                }
            }
        }

        // 3) Recompute depth + dirties
        auto computeDepth = [&](EntityID n) {
            int d = 0;
            std::unordered_set<EntityID> seen;
            EntityID cur = n;
            while (ecs.IsEntityValid(cur)) {
                const auto& h = ecs.GetComponent<HierarchyComponent>(cur);
                if (h.parent == HierarchyComponent::INVALID_PARENT) break;
                if (!seen.insert(cur).second) break; // cycle guard
                cur = h.parent; ++d;
            }
            return d;
            };

        for (EntityID id = 0; id < Ermine::MAX_ENTITIES; ++id) {
            if (!ecs.IsEntityValid(id) || !ecs.HasComponent<HierarchyComponent>(id)) continue;
            auto& h = ecs.GetComponent<HierarchyComponent>(id);
            h.depth = computeDepth(id);
            h.isDirty = true;
            h.worldTransformDirty = true;
        }
    }
}

static void CollectSubtree(const Ermine::ECS& ecs, Ermine::EntityID root, std::vector<Ermine::EntityID>& out)
{
    if (!ecs.IsEntityValid(root)) return;
    std::vector<Ermine::EntityID> stack{ root };
    while (!stack.empty()) {
        Ermine::EntityID e = stack.back(); stack.pop_back();
        if (!ecs.IsEntityValid(e)) continue;
        out.push_back(e);
        if (ecs.HasComponent<Ermine::HierarchyComponent>(e)) {
            const auto& h = ecs.GetComponent<Ermine::HierarchyComponent>(e);
            for (Ermine::EntityID c : h.children) stack.push_back(c);
        }
    }
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

        if (ecs.HasComponent<Ermine::IDComponent>(id)) {
            rapidjson::Value idPayload(rapidjson::kObjectType);
            const auto& c = ecs.GetComponent<Ermine::IDComponent>(id);

            const std::string guid_str = c.guid.ToString();
            idPayload.AddMember(
                rapidjson::Value("guid", a),
                rapidjson::Value(guid_str.c_str(), (rapidjson::SizeType)guid_str.size(), a),
                a
            );

            comps.AddMember(rapidjson::Value("IDComponent", a), idPayload, a);
        }

        for (const std::string& name : ecs.GetComponentNames(id)) {
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
                if (name == "ScriptsComponent" && ecs.HasComponent<Ermine::ScriptsComponent>(id))
                {
                    auto& scs = ecs.GetComponent<Ermine::ScriptsComponent>(id);
                    scs.Serialize(payload, a);   // <-- this already writes fields
                    wrote = true;
                }
            }

            if (wrote)
                comps.AddMember(rapidjson::Value(name.c_str(), a), payload, a);
        }

        e.AddMember("components", comps, a);
        entities.PushBack(e, a);
    }

    d.AddMember("entities", entities, a);


    if (auto renderer = ecs.GetSystem<Ermine::graphics::Renderer>()) {
        renderer->SyncToGlobalGraphics();

        rapidjson::Value ggJson(rapidjson::kObjectType);
        renderer->m_GlobalGraphics.Serialize(ggJson, a);
        d.AddMember("globalGraphics", ggJson, a);
    }

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

        if (comps.HasMember("IDComponent") && comps["IDComponent"].IsObject()) {
            const auto& payload = comps["IDComponent"];

            Ermine::Guid g =
                (payload.HasMember("guid") && payload["guid"].IsString())
                ? Ermine::Guid::FromString(payload["guid"].GetString())
                : Ermine::Guid::New(); // fallback for old files

            if (ecs.HasComponent<Ermine::IDComponent>(id)) {
                auto& c = ecs.GetComponent<Ermine::IDComponent>(id);
                c.guid = g;
            }
            else {
                ecs.AddComponent<Ermine::IDComponent>(id, Ermine::IDComponent{ g });
            }
            ecs.GetGuidRegistry().Register(id, g);
        }

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
            }
            else
            {
                // --- Custom fallbacks ---
                if (compName == "ScriptsComponent")
                {
                    // Ensure component exists
                    if (!ecs.HasComponent<Ermine::ScriptsComponent>(id))
                        ecs.AddComponent<Ermine::ScriptsComponent>(id, Ermine::ScriptsComponent{});

                    auto& scs = ecs.GetComponent<Ermine::ScriptsComponent>(id);

                    // Let the component handle scripts + fields
                    scs.Deserialize(payload);

                    // NOTE: Do NOT call AttachAll here if you have a ScriptSystem that does it later.
                    // If you prefer immediate instances after load, you *can* do:
                    scs.AttachAll(id);

                    handled = true;
                }
                else if (compName == "Script")
                {
                    // Optional legacy single-Script -> ScriptsComponent upgrade
                    if (payload.HasMember("class") && payload["class"].IsString())
                    {
                        const std::string cls = payload["class"].GetString();
                        if (!ecs.HasComponent<Ermine::ScriptsComponent>(id))
                            ecs.AddComponent<Ermine::ScriptsComponent>(id, Ermine::ScriptsComponent{});
                        ecs.GetComponent<Ermine::ScriptsComponent>(id).Add(cls, id);
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

    Ermine::ResolveHierarchyGuids(ecs);

    // Upload all registered meshes to GPU and build indirect draw commands
    if (renderer) {
        renderer->m_MeshManager.UploadAndBuild();
        EE_CORE_INFO("Scene loaded: MeshManager populated with {} meshes",
            renderer->m_MeshManager.GetMeshCount());

        if (d.HasMember("globalGraphics") && d["globalGraphics"].IsObject()) {
            renderer->m_GlobalGraphics.Deserialize(d["globalGraphics"]);
            renderer->ApplyFromGlobalGraphics();
        }
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
    Ermine::ECS::GetInstance().GetSystem<Ermine::Physics>()->UpdatePhysicList();
    Ermine::ECS::GetInstance().GetSystem<Ermine::graphics::Renderer>()->MarkDrawDataForRebuild();
}

Ermine::EntityID LoadPrefabFromFile(Ermine::ECS& ecs, const std::filesystem::path& path)
{
    if (!path.has_extension() || path.extension() != ".prefab") {
        EE_CORE_ERROR("LoadPrefabFromFile rejected non-prefab file: {}", path.string());
        return {};
    }

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Could not open file for reading: " + path.string());

    IStreamWrapper isw(ifs);
    Document d; d.ParseStream(isw);
    if (d.HasParseError() || !d.IsObject())
        throw std::runtime_error("Invalid JSON file: " + path.string());
    if (!d.HasMember("entities") || !d["entities"].IsArray())
        throw std::runtime_error("Invalid prefab JSON (missing 'entities'): " + path.string());

    const auto& ents = d["entities"];

    // oldGuidStr -> new EntityID
    std::unordered_map<std::string, Ermine::EntityID> oldGuidToNew;
    oldGuidToNew.reserve(ents.Size());

    // ---------- Pass 1: create entities, ensure single IDComponent, assign NEW GUIDs ----------
    for (auto& e : ents.GetArray()) {
        if (!e.IsObject()) continue;
        if (!e.HasMember("components") || !e["components"].IsObject()) continue;
        const auto& comps = e["components"];

        std::string oldGuid;
        if (comps.HasMember("IDComponent")) {
            const auto& idc = comps["IDComponent"];
            if (idc.IsObject() && idc.HasMember("guid") && idc["guid"].IsString())
                oldGuid = idc["guid"].GetString();
        }

        Ermine::EntityID newEntity = ecs.CreateEntity();

        // Avoid double-adding IDComponent (some engines add it in CreateEntity)
        Ermine::Guid newGuid = Ermine::Guid::New();
        if (ecs.HasComponent<Ermine::IDComponent>(newEntity)) {
            ecs.GetComponent<Ermine::IDComponent>(newEntity).guid = newGuid;
        }
        else {
            ecs.AddComponent<Ermine::IDComponent>(newEntity, Ermine::IDComponent{ newGuid });
        }
        ecs.GetGuidRegistry().Unregister(newEntity); // safe even if absent
        ecs.GetGuidRegistry().Register(newEntity, newGuid);

        if (!oldGuid.empty())
            oldGuidToNew.emplace(oldGuid, newEntity);
    }

    auto remapEntityByOldGuid = [&](const std::string& old) -> Ermine::EntityID {
        auto it = oldGuidToNew.find(old);
        return it != oldGuidToNew.end() ? it->second : Ermine::HierarchyComponent::INVALID_PARENT;
        };

    Ermine::EntityID rootEntity = 0;

    // ---------- Pass 2: deserialize everything else; remap Hierarchy parentGuid ----------
    for (auto& e : ents.GetArray()) {
        if (!e.IsObject()) continue;
        if (!e.HasMember("components") || !e["components"].IsObject()) continue;
        const auto& comps = e["components"];

        // resolve our new entity from saved old guid
        std::string myOldGuid;
        if (comps.HasMember("IDComponent")) {
            const auto& idc = comps["IDComponent"];
            if (idc.IsObject() && idc.HasMember("guid") && idc["guid"].IsString())
                myOldGuid = idc["guid"].GetString();
        }
        Ermine::EntityID entity = remapEntityByOldGuid(myOldGuid);
        if (!ecs.IsEntityValid(entity)) continue;

        for (auto it = comps.MemberBegin(); it != comps.MemberEnd(); ++it) {
            if (!it->value.IsObject()) continue;
            const std::string compName = it->name.GetString();
            const auto& payload = it->value;

            if (compName == "IDComponent") continue; // already handled in pass 1

            // HierarchyComponent: set parentGuid by remapping old parent guid to the new entity's guid
            if (compName == "HierarchyComponent") {
                Ermine::Guid parentGuid{};
                if (payload.HasMember("parentGuid") && payload["parentGuid"].IsString()) {
                    const std::string oldParentGuid = payload["parentGuid"].GetString();
                    Ermine::EntityID newParent = remapEntityByOldGuid(oldParentGuid);
                    if (newParent != Ermine::HierarchyComponent::INVALID_PARENT &&
                        ecs.HasComponent<Ermine::IDComponent>(newParent)) {
                        parentGuid = ecs.GetComponent<Ermine::IDComponent>(newParent).guid;
                    }
                }

                // Avoid duplicate add
                if (ecs.HasComponent<Ermine::HierarchyComponent>(entity)) {
                    auto& h = ecs.GetComponent<Ermine::HierarchyComponent>(entity);
                    h.parentGuid = parentGuid;
                    h.parent = Ermine::HierarchyComponent::INVALID_PARENT;
                    h.children.clear();
                    h.depth = 0;
                    h.isDirty = true;
                    h.worldTransformDirty = true;
                }
                else {
                    Ermine::HierarchyComponent h;
                    h.parentGuid = parentGuid;
                    ecs.AddComponent<Ermine::HierarchyComponent>(entity, h);
                }

                if (!parentGuid.IsValid() && rootEntity == 0) rootEntity = entity;
                continue;
            }

            // ScriptsComponent (array)
            if (compName == "ScriptsComponent") {
                std::vector<std::string> classNames;
                if (payload.HasMember("scripts") && payload["scripts"].IsArray()) {
                    const auto arr = payload["scripts"].GetArray();
                    classNames.reserve(arr.Size());
                    for (const auto& v : arr) {
                        if (!v.IsObject()) continue;
                        const auto m = v.FindMember("class");
                        if (m != v.MemberEnd() && m->value.IsString())
                            classNames.emplace_back(m->value.GetString());
                    }
                }
                else if (payload.HasMember("class") && payload["class"].IsString()) {
                    // legacy single-class form
                    classNames.emplace_back(payload["class"].GetString());
                }

                if (!ecs.HasComponent<Ermine::ScriptsComponent>(entity))
                    ecs.AddComponent<Ermine::ScriptsComponent>(entity, Ermine::ScriptsComponent{});
                auto& scs = ecs.GetComponent<Ermine::ScriptsComponent>(entity);
                for (const auto& cls : classNames) scs.Add(cls, entity);
                continue;
            }

            // Single Script (legacy)
            if (compName == "Script") {
                if (payload.HasMember("class") && payload["class"].IsString()) {
                    const std::string cls = payload["class"].GetString();
                    // Prefer canonical ScriptsComponent container:
                    if (!ecs.HasComponent<Ermine::ScriptsComponent>(entity))
                        ecs.AddComponent<Ermine::ScriptsComponent>(entity, Ermine::ScriptsComponent{});
                    ecs.GetComponent<Ermine::ScriptsComponent>(entity).Add(cls, entity);
                }
                continue;
            }

            // Generic path
            if (const auto* desc = ecs.GetDescriptor(compName); desc && desc->deserialize) {
                desc->deserialize(entity, payload);
            }
            else {
                EE_CORE_WARN("Prefab component '{}' has no deserializer; skipping.", compName.c_str());
            }
        }
    }

    // finalize
    ecs.ResyncAllSignaturesFromStorage();
    Ermine::ResolveHierarchyGuids(ecs);

    //ecs.GetSystem<Ermine::HierarchySystem>()->ForceUpdateAllTransforms();
    ecs.GetSystem<Ermine::Physics>()->UpdatePhysicList();

    // Return detected root; fallback to first created if none marked as root
    if (rootEntity != 0) return rootEntity;
    if (!oldGuidToNew.empty()) return oldGuidToNew.begin()->second;
    return {};
}



void SavePrefabToFile(const Ermine::ECS& ecs, Ermine::EntityID root, const std::filesystem::path& path)
{
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

    // collect subtree
    std::vector<Ermine::EntityID> toProcess{ root };
    std::vector<Ermine::EntityID> all;
    while (!toProcess.empty()) {
        Ermine::EntityID e = toProcess.back();
        toProcess.pop_back();
        if (!ecs.IsEntityValid(e)) continue;
        all.push_back(e);

        if (ecs.HasComponent<Ermine::HierarchyComponent>(e)) {
            const auto& h = ecs.GetComponent<Ermine::HierarchyComponent>(e);
            for (auto c : h.children)
                toProcess.push_back(c);
        }
    }

    for (Ermine::EntityID id : all) {
        Value e(kObjectType);
        Value comps(kObjectType);

        // always write IDComponent
        if (ecs.HasComponent<Ermine::IDComponent>(id)) {
            const auto& c = ecs.GetComponent<Ermine::IDComponent>(id);
            std::string guid_str = c.guid.ToString();
            Value idPayload(kObjectType);
            idPayload.AddMember("guid", Value(guid_str.c_str(), (rapidjson::SizeType)guid_str.size(), a), a);
            comps.AddMember("IDComponent", idPayload, a);
        }

        // write all other components like the scene
        for (const std::string& name : ecs.GetComponentNames(id)) {
            // ... inside: for (const std::string& name : ecs.GetComponentNames(id)) {
            const auto* desc = ecs.GetDescriptor(name);
            rapidjson::Value payload(rapidjson::kObjectType);
            bool wrote = false;

            // Prefer generic serializer
            if (desc && desc->serialize) {
                desc->serialize(id, payload, a);
                wrote = true;
            }

            // --- Prefab fallback: ScriptsComponent (array of classes)
            if (!wrote && name == "ScriptsComponent" && ecs.HasComponent<Ermine::ScriptsComponent>(id)) {
                const auto& scs = ecs.GetComponent<Ermine::ScriptsComponent>(id);
                rapidjson::Value arr(rapidjson::kArrayType);
                for (const auto& sc : scs.scripts) {
                    rapidjson::Value obj(rapidjson::kObjectType);
                    obj.AddMember(rapidjson::Value("class", a),
                        rapidjson::Value(sc.m_className.c_str(), a), a);
                    arr.PushBack(obj, a);
                }
                payload.AddMember(rapidjson::Value("scripts", a), arr, a);
                wrote = true;
            }

            // --- Prefab fallback: single Script
            if (!wrote && name == "Script" && ecs.HasComponent<Ermine::Script>(id)) {
                const auto& s = ecs.GetComponent<Ermine::Script>(id);
                payload.AddMember(rapidjson::Value("class", a),
                    rapidjson::Value(s.m_className.c_str(), a), a);
                wrote = true;
            }

            if (wrote) {
                comps.AddMember(rapidjson::Value(name.c_str(), a), payload, a);
            }
        }

        e.AddMember("components", comps, a);
        entities.PushBack(e, a);
    }

    d.AddMember("entities", entities, a);

    PrettyWriter<OStreamWrapper> w(osw);
    w.SetIndent(' ', 2);
    d.Accept(w);
}

