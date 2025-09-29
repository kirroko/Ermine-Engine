/* Start Header ************************************************************************/
/*!
\file       Animator.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       27/09/2025
\brief      This file contains the definition of the animator class. It is responsible for
            loading animation clips from an Assimp scene, managing playback state,
            updating bone transforms per frame, providing final matrices for GPU skinning
            in the renderer.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Animator.h"
#include "Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Ermine::graphics
{
    /**
     * @brief Construct an Animator for a given model.
     * @param model The skinned model to animate
     */
    Animator::Animator(std::shared_ptr<Model> model) : m_Model(std::move(model))
    {
        m_Scene = m_Model->GetAssimpScene(); // cache scene for hierarchy traversal
        m_FinalBoneMatrices.resize(m_Model->GetBoneCount(), glm::mat4(1.0f));
        
        LoadAnimations(); // Load all animation clips
        PlayAnimation(0, true); // play first clip, temp
    }

    /**
     * @brief Load all animations from an Assimp scene.
     * @param scene The Assimp scene containing animations
     */
    void Animator::LoadAnimations()
    {
        m_Clips.clear();
        if (!m_Scene) return;

        for (unsigned int i = 0; i < m_Scene->mNumAnimations; ++i)
        {
            m_Clips.push_back(LoadAnimation(m_Scene->mAnimations[i]));
            EE_CORE_INFO("Animation clip loaded: {0}", m_Scene->mAnimations[i]->mName.C_Str());
        }
    }

    /**
     * @brief Build a single AnimationClip from Assimp data.
     * @param anim Assimp animation
     * @return Converted AnimationClip
     */
    AnimationClip Animator::LoadAnimation(const aiAnimation* anim)
    {
        AnimationClip clip;
        clip.name = anim->mName.C_Str();
        clip.duration = anim->mDuration;
        clip.ticksPerSecond = (anim->mTicksPerSecond != 0.0)
            ? anim->mTicksPerSecond : 25.0;

        for (unsigned int i = 0; i < anim->mNumChannels; ++i)
        {
            aiNodeAnim* channel = anim->mChannels[i];
            BoneAnimation boneAnim;
            boneAnim.boneName = channel->mNodeName.C_Str();

            // Collect all unique times
            std::vector<double> times;
            for (unsigned int j = 0; j < channel->mNumPositionKeys; ++j)
                times.push_back(channel->mPositionKeys[j].mTime);
            for (unsigned int j = 0; j < channel->mNumRotationKeys; ++j)
                times.push_back(channel->mRotationKeys[j].mTime);
            for (unsigned int j = 0; j < channel->mNumScalingKeys; ++j)
                times.push_back(channel->mScalingKeys[j].mTime);

            std::sort(times.begin(), times.end());
            times.erase(std::unique(times.begin(), times.end()), times.end());

            // Build merged keyframes
            for (double t : times)
            {
                Keyframe kf;
                kf.time = t;

                // position
                if (channel->mNumPositionKeys > 0) {
                    aiVector3D pos = channel->mPositionKeys[0].mValue;
                    for (unsigned int j = 0; j < channel->mNumPositionKeys - 1; ++j) {
                        if (t < channel->mPositionKeys[j + 1].mTime) {
                            double dt = channel->mPositionKeys[j + 1].mTime - channel->mPositionKeys[j].mTime;
                            double factor = (dt > 0.0) ? (t - channel->mPositionKeys[j].mTime) / dt : 0.0;
                            aiVector3D start = channel->mPositionKeys[j].mValue;
                            aiVector3D end = channel->mPositionKeys[j + 1].mValue;
                            pos = start + static_cast<float>(factor) * (end - start);
                            break;
                        }
                    }
                    kf.position = glm::vec3(pos.x, pos.y, pos.z);
                }
                else
                    kf.position = glm::vec3(0.0f);

                // rotation
                if (channel->mNumRotationKeys > 0) {
                    aiQuaternion rot = channel->mRotationKeys[0].mValue;
                    for (unsigned int j = 0; j < channel->mNumRotationKeys - 1; ++j) {
                        if (t < channel->mRotationKeys[j + 1].mTime) {
                            double dt = channel->mRotationKeys[j + 1].mTime - channel->mRotationKeys[j].mTime;
                            double factor = (dt > 0.0) ? (t - channel->mRotationKeys[j].mTime) / dt : 0.0;
                            aiQuaternion start = channel->mRotationKeys[j].mValue;
                            aiQuaternion end = channel->mRotationKeys[j + 1].mValue;
                            aiQuaternion result;
                            aiQuaternion::Interpolate(result, start, end, static_cast<float>(factor));
                            result.Normalize();
                            rot = result;
                            break;
                        }
                    }
                    kf.rotation = glm::quat(rot.w, rot.x, rot.y, rot.z);
                }
                else
                    kf.rotation = glm::quat(1, 0, 0, 0);

                // scaling
                if (channel->mNumScalingKeys > 0) {
                    aiVector3D scl = channel->mScalingKeys[0].mValue;
                    for (unsigned int j = 0; j < channel->mNumScalingKeys - 1; ++j) {
                        if (t < channel->mScalingKeys[j + 1].mTime) {
                            double dt = channel->mScalingKeys[j + 1].mTime - channel->mScalingKeys[j].mTime;
                            double factor = (dt > 0.0) ? (t - channel->mScalingKeys[j].mTime) / dt : 0.0;
                            aiVector3D start = channel->mScalingKeys[j].mValue;
                            aiVector3D end = channel->mScalingKeys[j + 1].mValue;
                            scl = start + static_cast<float>(factor) * (end - start);
                            break;
                        }
                    }
                    kf.scale = glm::vec3(scl.x, scl.y, scl.z);
                }
                else
                    kf.scale = glm::vec3(1.0f);

                boneAnim.keys.push_back(std::move(kf));
            }

            clip.boneAnimations[boneAnim.boneName] = std::move(boneAnim);
        }

        return clip;
    }

    /**
     * @brief Clear all loaded animations.
     */
    void Animator::ClearAnimations()
    {
        m_Clips.clear();
        m_CurrentClip = nullptr;
        m_CurrentTime = 0.0;
        m_Paused = false;
    }

    /**
     * @brief Play an animation by index.
     * @param index Index into loaded clips
     * @param loop Whether to loop playback
     */
    void Animator::PlayAnimation(size_t index, bool loop)
    {
        if (index >= m_Clips.size()) return;
        m_CurrentClip = &m_Clips[index];
        m_CurrentTime = 0.0;
        m_Loop = loop;
        EE_CORE_INFO("Animation clip played: {0}", m_CurrentClip->name);
    }

    /**
     * @brief Play an animation by name.
     * @param name Animation name
     * @param loop Whether to loop playback
     */
    void Animator::PlayAnimation(const std::string& name, bool loop)
    {
        for (auto& clip : m_Clips) {
            if (clip.name == name) {
                m_CurrentClip = &clip;
                m_CurrentTime = 0.0;
                m_Loop = loop;
                EE_CORE_INFO("Animation clip played: {0}", clip.name);
                return;
            }
        }
    }

    /**
     * @brief Stop the current animation.
     * Resets to the beginning and clears the current clip.
     */
    void Animator::StopAnimation()
    {
        m_CurrentClip = nullptr;
        m_CurrentTime = 0.0;
        m_Paused = false;
        EE_CORE_INFO("Animation clip stopped");
    }

    /**
     * @brief Pause the current animation.
     * Does nothing if no animation is playing.
     */
    void Animator::PauseAnimation()
    {
        if (m_CurrentClip)
        {
            m_Paused = true;
            EE_CORE_INFO("Animation clip paused");
        }
    }

    /**
     * @brief Resume the current animation if paused.
     * Does nothing if not paused or no animation is playing.
     */
    void Animator::ResumeAnimation()
    {
        if (m_CurrentClip && m_Paused)
        {
            m_Paused = false;
            EE_CORE_INFO("Animation clip resumed");
        }
    }

    /**
     * @brief Advance animation and update bone transforms.
     * @param deltaTime Time step in seconds
     */
    void Animator::Update(double deltaTime)
    {
        if (!m_CurrentClip || !m_Scene || !m_Scene->mRootNode) return;
        if (m_Paused) return;

        // Advance animation time in ticks
        double ticksPerSecond = m_CurrentClip->ticksPerSecond != 0.0
            ? m_CurrentClip->ticksPerSecond : 25.0;
        m_CurrentTime += deltaTime * ticksPerSecond;

        // Check animation looping
        if (m_Loop)
            m_CurrentTime = fmod(m_CurrentTime, m_CurrentClip->duration);
        else if (m_CurrentTime > m_CurrentClip->duration)
            m_CurrentTime = m_CurrentClip->duration; // clamp

        // Reset bone matrices
        std::fill(m_FinalBoneMatrices.begin(), m_FinalBoneMatrices.end(), glm::mat4(1.0f));

        // Start recursion at root
        CalculateBoneTransform(m_Scene->mRootNode, glm::mat4(1.0f));

        // Push back results to the model
        m_Model->SetBoneTransforms(m_FinalBoneMatrices);
    }

    /**
     * @brief Recursively calculate bone transforms by traversing Assimp node hierarchy.
     * @param node Current node
     * @param parentTransform Parent's transform
     */
    void Animator::CalculateBoneTransform(const aiNode* node, const glm::mat4& parentTransform)
    {
        std::string nodeName(node->mName.C_Str());
        glm::mat4 nodeTransform = m_Model->ToGlm(node->mTransformation);

        const BoneAnimation* boneAnim = nullptr;
        if (m_CurrentClip) {
            auto it = m_CurrentClip->boneAnimations.find(nodeName);
            if (it != m_CurrentClip->boneAnimations.end())
                boneAnim = &it->second;
        }

        if (boneAnim && !boneAnim->keys.empty())
        {
            // Find keyframes around current time
            const auto& keys = boneAnim->keys;
            size_t i = 0;
            for (; i < keys.size() - 1; ++i) {
                if (m_CurrentTime < keys[i + 1].time) break;
            }

            const Keyframe& curr = keys[i];
            const Keyframe& next = (i + 1 < keys.size()) ? keys[i + 1] : curr;

            double delta = next.time - curr.time;
            double factor = (delta > 0.0) ? (m_CurrentTime - curr.time) / delta : 0.0;

            glm::vec3 translation = glm::mix(curr.position, next.position, static_cast<float>(factor));
            glm::quat rotation = glm::slerp(curr.rotation, next.rotation, static_cast<float>(factor));
            glm::vec3 scale = glm::mix(curr.scale, next.scale, static_cast<float>(factor));

            nodeTransform = glm::translate(glm::mat4(1.0f), translation)
                * glm::mat4_cast(rotation)
                * glm::scale(glm::mat4(1.0f), scale);
        }

        glm::mat4 globalTransform = parentTransform * nodeTransform;

        const auto& boneMap = m_Model->GetBoneMapping();
        auto it = boneMap.find(nodeName);
        if (it != boneMap.end())
        {
            int boneIndex = it->second;
            m_FinalBoneMatrices[boneIndex] = globalTransform * m_Model->GetBoneOffsets()[boneIndex];
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            CalculateBoneTransform(node->mChildren[i], globalTransform);
        }
    }
}
