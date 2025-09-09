/* Start Header ************************************************************************/
/*!
\file       AnimatorSystem.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       04/09/2025
\brief      This file contains the definition of the animator system.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "AnimatorSystem.h"

namespace Ermine
{
    void AnimatorSystem::update(float dt)
    {
        for (auto& e : entries)
        {
            if (!e.anim || !e.skel || !e.anim->animator) continue;
            auto& anim = *e.anim->animator;
            int bc = e.skel->boneCount();
            if (bc != anim.skeleton_bone_count)
            {
                anim.setSkeletonBoneCount(bc);
                anim.setBasePose(e.skel->local_bind_pose);
            }
            anim.update(dt);
            e.skel->applyLocalPose(anim.out_pose);
        }
    }
}
