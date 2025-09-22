/* Start Header ************************************************************************/
/*!
\file       AnimationClip.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       22/09/2025
\brief      This file contains the declaration of the animation clip structure.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace Ermine::graphics
{
    // Key frame structure
    struct Keyframe {
        double time; // In ticks
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
    };

    // Bone animation structure
    struct BoneAnimation {
        std::string boneName;
        std::vector<Keyframe> keys;  // Keyframes sorted by time
    };

    // Animation clip structure
    struct AnimationClip {
        std::string name;
        double duration = 0.0;       // In ticks
        double ticksPerSecond = 25.0; // FBX often uses 25 or 30

        std::unordered_map<std::string, BoneAnimation> boneAnimations;
    };
}
