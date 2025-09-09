/* Start Header ************************************************************************/
/*!
\file       AnimatorEditor.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       04/09/2025
\brief      This file contains the definition of the animator editor using ImGui.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Animator.h"

//#ifdef IMGUI_INCLUDED
#include "imgui.h"

namespace Ermine
{
    // Optional: ImGui helpers to inspect animator in-editor.
    // Make sure ImGui is available.

    void DrawAnimatorInspector(Animator& animator)
    {
        if (ImGui::Begin("Animator"))
        {
            if (ImGui::CollapsingHeader("Parameters"))
            {
                for (auto& kv : animator.params)
                {
                    ImGui::Text("%s", kv.first.c_str());
                    // simple display; implement editors per type
                }
            }
            if (ImGui::CollapsingHeader("Layers"))
            {
                for (int i = 0; i < (int)animator.layers.size(); ++i)
                {
                    Layer& L = animator.layers[i];
                    if (ImGui::TreeNode(("Layer##" + std::to_string(i)).c_str()))
                    {
                        ImGui::Text("Name: %s", L.name.c_str());
                        ImGui::Checkbox("Override blend", (bool*)&(L.blend_mode));
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::End();
        }
    }
}
//#endif // IMGUI_INCLUDED
