/* Start Header ************************************************************************/
/*!
\file       FSMEditor.cpp
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       06/10/2025
\brief      This file contains definitions for imgui window FSM editor.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "FSMEditor.h"

namespace Ermine
{
    void FSMEditorImGUI::CreateNode(const std::string& name)
    {
        if (m_SelectedEntity == 0)
            return;

        ScriptNode node;
        node.id = m_nextNodeId++;
        node.name = name;
        node.isAttached = false;
        node.scriptClassName = "";

        m_entityNodes[m_SelectedEntity].push_back(std::move(node));
    }

    void FSMEditorImGUI::Render()
    {
        if (!ImNodes::GetCurrentContext())
        {
            ImNodes::CreateContext();
            ImNodes::StyleColorsDark();
        }

        ImGui::Begin(Name().c_str());

        if (m_SelectedEntity == 0)
        {
            ImGui::Text("No entity selected.");
            ImGui::End();
            return;
        }

        auto& scriptNodes = m_entityNodes[m_SelectedEntity];
        auto& links = m_entityLinks[m_SelectedEntity];

        ImGui::InputText("New Node Name", m_newNodeName, IM_ARRAYSIZE(m_newNodeName));
        ImGui::SameLine();
        if (ImGui::Button("Add Node"))
        {
            if (strlen(m_newNodeName) > 0)
            {
                CreateNode(m_newNodeName);
                m_newNodeName[0] = '\0';
            }
        }
        ImGui::Separator();

        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(m_SelectedEntity);

        // (Optional) sync m_links from fsm.transitions
        links.clear();

        for (auto& [fromScript, toScript] : fsm.scriptTransitions)
        {
            int fromId = -1, toId = -1;
            for (auto& s : scriptNodes) {
                if (&s == fromScript) fromId = s.id;
                if (&s == toScript)   toId = s.id;
            }

            if (fromId != -1 && toId != -1)
                links.emplace_back(fromId * 10, toId * 10 + 1);
        }

        // Draw editor
        ImNodes::BeginNodeEditor();

        for (auto& snode : scriptNodes)
        {
            ImNodes::BeginNode(snode.id);
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted((snode.name + " (Script)").c_str());
            ImNodes::EndNodeTitleBar();

            // Script attach UI
            if (snode.isAttached)
                ImGui::Text("Script: %s", snode.scriptClassName.c_str());
            else if (ImGui::Button(("Attach Script##" + std::to_string(snode.id)).c_str()))
                ImGui::OpenPopup(("AttachScriptPopup" + std::to_string(snode.id)).c_str());

            if (ImGui::BeginPopup(("AttachScriptPopup" + std::to_string(snode.id)).c_str()))
            {
                static char scriptName[128] = "";
                ImGui::InputText("Class Name", scriptName, IM_ARRAYSIZE(scriptName));
                if (ImGui::Button("Confirm"))
                {
                    snode.isAttached = true;
                    snode.scriptClassName = scriptName;

                    if (!snode.scriptClassName.empty())
                    {
                        //auto sc = std::make_unique<scripting::ScriptClass>("", snode.scriptClassName);
                        //snode.instance = std::make_unique<scripting::ScriptInstance>(
                        //    std::move(sc),
                        //    m_SelectedEntity // or the ECS entity currently selected
                        //);
                        snode.isAttached = true;
                    }

                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImNodes::BeginInputAttribute(snode.id * 10 + 1);
            ImGui::Text("In");
            ImNodes::EndInputAttribute();

            ImNodes::BeginOutputAttribute(snode.id * 10);
            ImGui::Text("Out");
            ImNodes::EndOutputAttribute();

            ImNodes::EndNode();
        }

        //int linkId = 1;
        //for (auto& link : m_links)
        //    ImNodes::Link(linkId++, link.first * 10, link.second * 10 + 1);

        int linkId = 1;
        for (auto& link : links)
            ImNodes::Link(linkId++, link.first, link.second);
        ImNodes::EndNodeEditor();

        // Handle new link
        int startAttr, endAttr;
        if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
        {
            // normalize so 'from' is an output pin and 'to' is an input pin
            auto is_output = [](int attr) { return (attr % 10) == 0; };   // id*10
            auto is_input = [](int attr) { return (attr % 10) == 1; };   // id*10+1

            int fromAttr = startAttr;
            int toAttr = endAttr;
            if (is_input(fromAttr) && is_output(toAttr))
                std::swap(fromAttr, toAttr);

            // derive node IDs
            int fromId = fromAttr / 10;
            int toId = (toAttr - 1) / 10;

            // find nodes
            ScriptNode* fromScriptNode = nullptr;
            ScriptNode* toScriptNode = nullptr;

            for (auto& s : scriptNodes) {
                if (s.id == fromId) fromScriptNode = &s;
                if (s.id == toId)   toScriptNode = &s;
            }

            auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(m_SelectedEntity);

            if (fromScriptNode && toScriptNode)
                fsm.scriptTransitions[fromScriptNode] = toScriptNode;

            // store ATTRIBUTE IDs (pins) for drawing
            links.emplace_back(fromAttr, toAttr);
        }

        ImGui::End();
    }
}