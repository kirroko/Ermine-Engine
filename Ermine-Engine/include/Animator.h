/* Start Header ************************************************************************/
/*!
\file       Animator.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       22/09/2025
\brief      This file contains the declaration of the animator.
            This file is used for the animator core logic.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "AnimationClip.h"
#include "Model.h"
#include <assimp/scene.h>

namespace Ermine::graphics
{
    class Animator {
    public:
        Animator(Model* model);

        void PlayAnimation(const AnimationClip* clip, bool loop = true);
        void Update(double deltaTime);

        const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return m_FinalBoneMatrices; }
        static AnimationClip LoadAnimation(const aiAnimation* anim);

    private:
        void CalculateBoneTransform(const aiNode* node, const glm::mat4& parentTransform);

        Model* m_Model;
        const AnimationClip* m_CurrentClip = nullptr;

        double m_CurrentTime = 0.0;
        bool m_Loop = true;

        std::vector<glm::mat4> m_FinalBoneMatrices; // one per bone
    };
}
