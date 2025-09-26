/* Start Header ************************************************************************/
/*!
\file       ViewPortGUI.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       21/09/2025
\brief      This file contains the responsibility for rendering the viewport window

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "ImguiUIWindow.h"

class InspectorGUI;

namespace Ermine
{
	class ViewPortGUI : public ImGUIWindow
	{
		bool show;
		InspectorGUI* ref_Inspector;
	public:
		ViewPortGUI();
		ViewPortGUI(InspectorGUI* ref);

		void Update() override;
		void Render() override;
	};
}
