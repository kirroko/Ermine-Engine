/* Start Header ************************************************************************/
/*!
\file       AnimationManager.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       27/09/2025
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
	 * @param deltaTime Frame time step in seconds
	 */
	void AnimationManager::Update(double deltaTime)
	{
		auto& ecs = ECS::GetInstance();

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
			animComp.m_animator->Update(deltaTime);

			// Ensure bone transforms vector is sized correctly
			const auto& finalBones = animComp.m_animator->GetFinalBoneMatrices();
			if (!finalBones.empty())
				modelComp.m_model->SetBoneTransforms(finalBones);
			modelComp.m_model->SetBoneTransforms(animComp.m_animator->GetFinalBoneMatrices());
		}
	}
}
