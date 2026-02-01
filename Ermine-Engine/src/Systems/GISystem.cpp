/* Start Header ************************************************************************/
/*!
\file       GISystem.cpp
\co-author  Ridhwan Afandi, mohamedridhwan.b, 2301367, mohamedridhwan.b\@digipen.edu
\date       02/01/2026
\brief      Implements GISystem for light probe iteration.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "GISystem.h"
#include "Renderer.h"
#include "Entity.h"

namespace Ermine
{
	void GISystem::Update()
	{
		AssignProbeIndices();
	}

	void GISystem::BakeAllProbes()
	{
		auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
		if (!renderer)
			return;

		AssignProbeIndices();
		renderer->BakeAllProbes();
	}

	void GISystem::RebuildAllProbeVolumes()
	{
		// No-op: one probe per volume (user-placed)
	}

	int GISystem::AssignProbeIndices()
	{
		auto& ecs = ECS::GetInstance();
		int index = 0;

		for (EntityID entity : m_Entities)
		{
			if (!ecs.HasComponent<LightProbeVolumeComponent>(entity))
				continue;

			auto& volume = ecs.GetComponent<LightProbeVolumeComponent>(entity);
			if (!volume.isActive)
			{
				volume.probeIndex = -1;
				continue;
			}

			if (index >= kMaxProbeIndices)
			{
				volume.probeIndex = -1;
				continue;
			}

			volume.probeIndex = index;
			++index;
		}

		return index;
	}
}
