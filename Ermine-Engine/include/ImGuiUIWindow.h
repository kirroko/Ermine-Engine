/* Start Header ************************************************************************/
/*!
\file       ImguiUIWindow.h
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       02/09/2025
\brief      This file contains declarations for interfaces of ImguiUIWindow.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"

namespace Ermine {

	class ImGUIWindow {
	public:
		/*!***********************************************************************
		\brief
		 <function brief>
		\param[in/out] <param name>
		 <parameter description>
		\param[in/out] <param 2 name>
		 <parameter 2 description>
		\return
		 <return description if any>
		*************************************************************************/
		ImGUIWindow(std::string _name = "") : m_name{_name} {}
		virtual ~ImGUIWindow() {};

		virtual void Update() {}
		virtual void Render() = 0;

		std::string Name() { return m_name; }
		void Name(std::string _name) { m_name = _name; }

	private:
		std::string m_name;
	};
}