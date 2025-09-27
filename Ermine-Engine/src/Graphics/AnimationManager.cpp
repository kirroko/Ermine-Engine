/* Start Header ************************************************************************/
/*!
\file       AnimationManager.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       26/09/2025
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
			if (ecs.HasComponent<AnimationComponent>(entity) && ecs.HasComponent<ModelComponent>(entity))
			{
				auto& animComp = ecs.GetComponent<AnimationComponent>(entity);
				auto& modelComp = ecs.GetComponent<ModelComponent>(entity);

				if (animComp.m_animator && animComp.m_animator->GetCurrentClip())
				{
					animComp.m_animator->Update(deltaTime);
					modelComp.m_model->SetBoneTransforms(animComp.m_animator->GetFinalBoneMatrices());
				}
			}
		}
	}
}
