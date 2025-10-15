#pragma once
#include <string>

namespace Ermine
{
    struct ScriptNode
    {
        int id;
        std::string name;
        std::string scriptClassName;
        bool isAttached = false;

        std::unique_ptr<scripting::ScriptInstance> instance;

        // Default constructor
        ScriptNode() = default;

        // Move constructor / assignment
        ScriptNode(ScriptNode&&) noexcept = default;
        ScriptNode& operator=(ScriptNode&&) noexcept = default;

        // Delete copy (unique_ptr makes this necessary anyway)
        ScriptNode(const ScriptNode&) = delete;
        ScriptNode& operator=(const ScriptNode&) = delete;

        void CreateInstance(EntityID entity)
        {
            EE_CORE_INFO("ScriptNode::CreateInstance(%d): attached=%d, class=%s",
                entity, isAttached, scriptClassName.c_str());

            if (!isAttached || scriptClassName.empty()) return;
            auto sc = std::make_unique<scripting::ScriptClass>("", scriptClassName);
            instance = std::make_unique<scripting::ScriptInstance>(std::move(sc), entity);

            EE_CORE_INFO("ScriptNode::CreateInstance: instance created");
        }

        // map to public lifecycle (no params)
        void OnEnter() { if (instance) instance->Start(); }
        void OnUpdate() { if (instance) instance->Update(); }
        void OnExit() { if (instance) instance->OnDisable(); /* instance reset optional */ }
    };
}
