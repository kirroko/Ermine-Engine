#pragma once
#include "PreCompile.h"
#include "ImguiUIWindow.h"
#include "ECS.h"

namespace Ermine
{
    class InspectorGUI : public ImGUIWindow
    {
    public:
        InspectorGUI();
        InspectorGUI(EntityID entity, std::string name = "Inspector");

        void SetEntity(EntityID entity);
		EntityID GetEntity() const { return m_entity; }

        void Render() override;  // defined in .cpp

    private:
        EntityID m_entity;
    };
}
