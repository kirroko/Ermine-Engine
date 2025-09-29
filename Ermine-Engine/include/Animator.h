/* Start Header ************************************************************************/
/*!
\file       Animator.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       27/09/2025
\brief      This file contains the declaration of the animator class. It is responsible for
            loading animation clips from an Assimp scene, managing playback state,
            updating bone transforms per frame, providing final matrices for GPU skinning
            in the renderer.

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
    /**
     * @brief Controls playback of skeletal animations for a model.
     */
    class Animator {
    public:
        /**
         * @brief Construct an Animator for a given model.
         * @param model The skinned model to animate
         */
        explicit Animator(std::shared_ptr<Model> model);

        /**
         * @brief Load all animations from an Assimp scene.
         * @param scene The Assimp scene containing animations
         */
        void LoadAnimations();

        /**
         * @brief Build a single AnimationClip from Assimp data.
         * @param anim Assimp animation
         * @return Converted AnimationClip
         */
        static AnimationClip LoadAnimation(const aiAnimation* anim);
        
        /**
         * @brief Clear all loaded animations.
         */
        void ClearAnimations();

        /**
         * @brief Play an animation by index.
         * @param index Index into loaded clips
         * @param loop Whether to loop playback
         */
        void PlayAnimation(size_t index, bool loop = true);

        /**
         * @brief Play an animation by name.
         * @param name Animation name
         * @param loop Whether to loop playback
         */
        void PlayAnimation(const std::string& name, bool loop = true);
        
        /**
         * @brief Stop the current animation.
         * Resets to the beginning and clears the current clip.
         */
        void StopAnimation();
        
        /**
         * @brief Pause the current animation.
         * Does nothing if no animation is playing.
         */
        void PauseAnimation();

        /**
         * @brief Resume the current animation if paused.
         * Does nothing if not paused or no animation is playing.
         */
        void ResumeAnimation();

        /**
         * @brief Advance animation and update bone transforms.
         * @param deltaTime Time step in seconds
         */
        void Update(double deltaTime);

        // @return All loaded animation clips
        const std::vector<AnimationClip>& GetClips() const { return m_Clips; }

        // @return Current animation clip, or nullptr if none
        const AnimationClip* GetCurrentClip() const { return m_CurrentClip; }

        // @return Returns true if currently paused
        bool IsPaused() const { return m_Paused; }
        
        // @return Final bone matrices to upload to GPU
        const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return m_FinalBoneMatrices; }

        const std::shared_ptr<Model>& GetModel() const { return m_Model; }

    private:
        std::shared_ptr<Model> m_Model;               // The model to animate
        const aiScene* m_Scene = nullptr;             // Cached Assimp scene
        
        std::vector<AnimationClip> m_Clips;           // All animation clips
        const AnimationClip* m_CurrentClip = nullptr; // Currently playing clip

        double m_CurrentTime = 0.0;                   // Current time in ticks
        bool m_Loop = true;                           // Looping flag
        bool m_Paused = false;                        // Pausing flag

        std::vector<glm::mat4> m_FinalBoneMatrices;   // Final transforms per bone

        /**
         * @brief Recursively calculate bone transforms by traversing Assimp node hierarchy.
         * @param node Current node
         * @param parentTransform Parent's transform
         */
        void CalculateBoneTransform(const aiNode* node, const glm::mat4& parentTransform);
    };
}
