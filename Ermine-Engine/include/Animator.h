/* Start Header ************************************************************************/
/*!
\file       Animator.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       04/09/2025
\brief      This file contains the declaration of the animator.
            This file is used for the animator core logic.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "AnimatorTypes.h"

namespace Ermine
{
    // Blend Job
    struct BlendJob
    {
        bool active = false;
        int from_state = -1;
        int to_state = -1;
        float duration = 0.2f;
        float elapsed = 0.0f;
    };

    // Animator class
    class Animator
    {
    public:
        Animator() = default;
        explicit Animator(int bone_count);

        // configuration
        std::vector<Layer> layers;
        std::unordered_map<std::string, ParamVal> params;

        // skeleton info
        int skeleton_bone_count = 0;
        std::vector<Trans> base_pose; // bind/rest pose

        // outputs
        std::vector<Trans> out_pose; // final local transforms

        // runtime
        std::vector<BlendJob> blend_jobs; // one per layer

        // API
        void setSkeletonBoneCount(int n);
        void setBasePose(const std::vector<Trans>& bind_pose);

        void update(float dt);

    private:
        bool evalCondition(const Condition& c) const;
        void advanceStateTime(Layer& layer, int state_index, float dt);
        void samplePoseInto(const Layer& layer, int state_index, std::vector<Trans>& out, float weight);
        void applyLayerToOutPose(const Layer& layer, const std::vector<Trans>& layer_pose);
    };
}
