/* Start Header ************************************************************************/
/*!
\file       ImguiUIWindow.h
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu (95%)
\co-author Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu (5%)
\date       26/01/2026
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
		 Constructor for ImGUIWindow, initializes the window with a name.
		\param[in] _name
		 Optional name for the ImGui window. Defaults to an empty string.
		*************************************************************************/
		ImGUIWindow(std::string _name = "") : m_name{ _name } {}
		/*!***********************************************************************
		\brief
		 Virtual destructor for ImGUIWindow.
		*************************************************************************/
		virtual ~ImGUIWindow() {};
		/*!***********************************************************************
		\brief
		 Update logic for the ImGui window.
		 Intended to be overridden by derived classes.
		*************************************************************************/
		virtual void Update() {}
		/*!***********************************************************************
		\brief
		 Pure virtual function to render the ImGui window.
		 Must be implemented by derived classes.
		*************************************************************************/
		virtual void Render() = 0;
		/*!***********************************************************************
		\brief
		 Getter for the window name.
		\return
		 The name of the ImGui window as a std::string.
		*************************************************************************/
		std::string Name() { return m_name; }
		/*!***********************************************************************
		\brief
		 Setter for the window name.
		\param[in] _name
		 New name to assign to the ImGui window.
		*************************************************************************/
		void Name(std::string _name) { m_name = _name; }

		/*!***********************************************************************
		\brief
		 Check if the window is open.
		\return
		 True if the window is open, false otherwise.
		*************************************************************************/
		bool IsOpen() const { return m_isOpen; }

		/*!***********************************************************************
		\brief
		 Set the open state of the window.
		\param[in] open
		 True to open the window, false to close it.
		*************************************************************************/
		void SetOpen(bool open) { m_isOpen = open; }

	protected:
		std::string m_name;   // Name of the ImGui window
		bool m_isOpen = true; // Flag indicating if the window is open
	};
}