/* Start Header ************************************************************************/
/*!
\file       Animator.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       22/09/2025
\brief      This file contains the definition of the animator.
            This file is used for the animator core logic.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Animator.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Ermine::graphics
{
    // -------------------- HELPER FUNCTIONS --------------------
    static glm::vec3 ToGlm(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
    static glm::quat ToGlm(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }
    static int FindKeyIndex(const std::vector<Keyframe>& keys, double time)
    {
        for (int i = (int)keys.size() - 1; i >= 0; --i)
            if (time >= keys[i].time) return i;
        return 0;
    }
    static glm::vec3 LerpVec3(const glm::vec3& a, const glm::vec3& b, float t) { return glm::mix(a, b, t); }
    static glm::quat SlerpQuat(const glm::quat& a, const glm::quat& b, float t) { return glm::slerp(a, b, t); }
    // --------------------------------------------------

    Animator::Animator(Model* model) : m_Model(model)
    {
        m_FinalBoneMatrices.resize(model->GetBoneCount(), glm::mat4(1.0f));
    }

    void Animator::PlayAnimation(const AnimationClip* clip, bool loop)
    {
        m_CurrentClip = clip;
        m_CurrentTime = 0.0;
        m_Loop = loop;
    }


    void Animator::Update(double deltaTime)
    {
        if (!m_CurrentClip) return;

        double ticksPerSecond = m_CurrentClip->ticksPerSecond != 0.0
            ? m_CurrentClip->ticksPerSecond : 25.0;
        m_CurrentTime += deltaTime * ticksPerSecond;

        if (m_Loop)
            m_CurrentTime = fmod(m_CurrentTime, m_CurrentClip->duration);

        std::fill(m_FinalBoneMatrices.begin(), m_FinalBoneMatrices.end(), glm::mat4(1.0f));
        // TODO: you need to cache aiScene rootNode somewhere (in Model or Animator)
        //CalculateBoneTransform(scene->mRootNode, glm::mat4(1.0f));
    }

    AnimationClip Animator::LoadAnimation(const aiAnimation* anim)
    {
        AnimationClip clip;
        clip.name = anim->mName.C_Str();
        clip.duration = anim->mDuration;
        clip.ticksPerSecond = anim->mTicksPerSecond != 0.0 ? anim->mTicksPerSecond : 25.0;

        for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
            aiNodeAnim* channel = anim->mChannels[c];
            BoneAnimation boneAnim;
            boneAnim.boneName = channel->mNodeName.C_Str();

            unsigned int maxKeys = std::max({ channel->mNumPositionKeys,
                                              channel->mNumRotationKeys,
                                              channel->mNumScalingKeys });

            for (unsigned int k = 0; k < maxKeys; ++k) {
                Keyframe key;
                key.time = 0.0;
                key.position = glm::vec3(0.0f);
                key.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                key.scale = glm::vec3(1.0f);

                if (k < channel->mNumPositionKeys) {
                    key.time = channel->mPositionKeys[k].mTime;
                    key.position = ToGlm(channel->mPositionKeys[k].mValue);
                }
                if (k < channel->mNumRotationKeys) {
                    key.time = channel->mRotationKeys[k].mTime;
                    key.rotation = ToGlm(channel->mRotationKeys[k].mValue);
                }
                if (k < channel->mNumScalingKeys) {
                    key.time = channel->mScalingKeys[k].mTime;
                    key.scale = ToGlm(channel->mScalingKeys[k].mValue);
                }

                boneAnim.keys.push_back(key);
            }

            clip.boneAnimations[boneAnim.boneName] = std::move(boneAnim);
        }
        return clip;
    }

    void Animator::CalculateBoneTransform(const aiNode* node, const glm::mat4& parentTransform)
    {
        std::string nodeName(node->mName.C_Str());
        glm::mat4 nodeTransform = m_Model->ToGlm(node->mTransformation);

        if (m_CurrentClip && m_CurrentClip->boneAnimations.count(nodeName)) {
            const BoneAnimation& boneAnim = m_CurrentClip->boneAnimations.at(nodeName);
            double time = m_CurrentTime;

            glm::vec3 pos(0.0f);
            glm::quat rot(1, 0, 0, 0);
            glm::vec3 scl(1.0f);

            if (!boneAnim.keys.empty()) {
                int index = FindKeyIndex(boneAnim.keys, time);
                int next = std::min(index + 1, (int)boneAnim.keys.size() - 1);

                double t1 = boneAnim.keys[index].time;
                double t2 = boneAnim.keys[next].time;
                float factor = (t2 == t1) ? 0.0f : (float)((time - t1) / (t2 - t1));

                pos = LerpVec3(boneAnim.keys[index].position, boneAnim.keys[next].position, factor);
                rot = SlerpQuat(boneAnim.keys[index].rotation, boneAnim.keys[next].rotation, factor);
                scl = LerpVec3(boneAnim.keys[index].scale, boneAnim.keys[next].scale, factor);
            }
            nodeTransform = glm::translate(glm::mat4(1.0f), pos) *
                glm::toMat4(rot) *
                glm::scale(glm::mat4(1.0f), scl);
        }

        glm::mat4 globalTransform = parentTransform * nodeTransform;

        auto it = m_Model->GetBoneMapping().find(nodeName);
        if (it != m_Model->GetBoneMapping().end()) {
            int boneIndex = it->second;
            m_FinalBoneMatrices[boneIndex] = globalTransform * m_Model->GetBoneOffsets()[boneIndex];
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
            CalculateBoneTransform(node->mChildren[i], globalTransform);
    }
}
