/* Start Header ************************************************************************/
/*!
\file       AnimatorSystem.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       04/09/2025
\brief      This file contains the declaration of the animator system.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "Components.h"

namespace Ermine
{
    // forward declarations for the engine ECS types
    struct SkeletonComponent
    {
        // Your engine's skeleton data
        std::vector<Trans> local_bind_pose; // local transforms in bind

        int boneCount() const { return (int)local_bind_pose.size(); }

        void applyLocalPose(const std::vector<Trans>& local_pose)
        {
            // implement conversion local->skin matrices & upload to GPU
            // This is engine specific — placeholder
        }
    };

    class AnimatorSystem
    {
    public:
        void update(float dt);

        // In a real engine you'd query the ECS. Here we expose a simple list for example.
        struct EntityEntry
        {
            AnimatorComponent* anim;
            SkeletonComponent* skel;
        };

        std::vector<EntityEntry> entries;
    };
}
