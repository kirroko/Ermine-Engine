/* Start Header ************************************************************************/
/*!
\file       AnimationManager.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\co-author  Ridhwan Afandi, moahamedridhwan.b, 2301367, moahamedridhwan.b\@digipen.edu
\date       27/10/2025
\brief      This file contains the definition of the animation manager.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "AnimationManager.h"
#include "Components.h"

namespace Ermine::graphics
{
	/**
	 * @brief Update all entities with AnimationComponent.
	 *
	 * Multiple instances of the same model can have independent skeletal animation states:
	 * - Each entity has its own AnimationComponent with its own Animator instance
	 * - Each entity gets its own bone transform offset in the SkeletalSSBO
	 * - Bone transforms are updated independently per entity
	 *
	 * @param deltaTime Frame time step in seconds
	 */
	void AnimationManager::Update(double deltaTime)
	{
		auto& ecs = ECS::GetInstance();

		// Wait for GPU to finish reading bone data from previous frame before overwriting
		if (m_SkeletalSSBO && m_SkeletalSSBO->IsValid())
		{
			m_SkeletalSSBO->WaitForGPU();
		}

		for (auto& entity : m_Entities)
		{
			// Check if animation and model components exist
			if (!ecs.HasComponent<AnimationComponent>(entity)
				|| !ecs.HasComponent<ModelComponent>(entity))
				continue;

			auto& animComp = ecs.GetComponent<AnimationComponent>(entity);
			auto& modelComp = ecs.GetComponent<ModelComponent>(entity);

			// Check if model exists and is valid
			if (!modelComp.m_model || !modelComp.m_model->GetAssimpScene())
				continue;

			// Check if animator exists and has clips
			if (!animComp.m_animator || animComp.m_animator->GetClips().empty())
				continue;

			// Check if animation is actually playing
			if (!animComp.m_animator->GetCurrentClip())
				continue;

			// Update animator
			animComp.m_animator->Update(deltaTime, entity);

			// Update bone transforms using SkeletalSSBO
			const auto& finalBones = animComp.m_animator->GetFinalBoneMatrices();
			if (!finalBones.empty() && m_SkeletalSSBO)
			{
				// Allocate bone space if not already allocated
				if (animComp.boneTransformOffset == -1)
				{
					animComp.boneTransformOffset = m_SkeletalSSBO->AllocateBoneSpace(finalBones.size());
				}

				// Update bone transforms using persistent mapped buffer (direct memcpy, zero-copy)
				if (animComp.boneTransformOffset >= 0)
				{
					m_SkeletalSSBO->UpdateBoneTransforms(animComp.boneTransformOffset, finalBones);
				}
			}
		}
	}
}
