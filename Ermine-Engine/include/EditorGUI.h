/* Start Header ************************************************************************/
/*!
\file       EditorGUI.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu (95%)
\co-authors LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu (5%)
\date       27/03/2025
\brief      This file contains the declaration of the EditorGUI class.
            Function just like a wrapper for the ImGUI library.
            Each window for teh editor should be encapsulated into a function in this class.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "imgui.h"
#include "GLFW/glfw3.h"
#include "ImGuiUIWindow.h"
#include <type_traits> // for std::is_base_of
#include <utility> // for std::forward

namespace Ermine::editor
{
    /**
	 * @brief The EditorGUI class, function just like a wrapper for the ImGUI library
	 */
    class EE_API EditorGUI
    {
        // Keeps track of all registered ImGui windows
        static std::vector<std::unique_ptr<ImGUIWindow>> m_Windows;

        static bool isPlaying;

		/**
		 * @brief Top menu bar for the editor
		 */
        static void TopMenuBar(GLFWwindow* windowContext);

		/**
		 * @brief Profiling window for the editor
		 */
		static void ProfilingWindow();

		/**
		 * @brief Show the viewport for the editor
		 * @param show 
		 */
		static void ViewPortWindow(bool& show);
    public:
        /**
         * @brief Initialize the ImGUI context
         * @param window The window to initialize the ImGUI context
         */
        static void Init(GLFWwindow* window);

		/**
		 * @brief Check if the ImGUI context is initialized
		 */
        static bool IsInit();

		/**
		 * @brief Dock the ImGUI window
		 */
		static void DockingWindow();

		/**
        * @brief Update the ImGUI context (Render)
        */
        static void Update(GLFWwindow* windowContext);

		/**
         * @brief Render the ImGUI context
         */
        static void Render();

        /**
         * @brief Shut down the ImGUI context
         */
        static void ShutDown();

        /*!***********************************************************************
        \brief
         Create and register an ImGUIWindow
        \param[in/out] T Window type
         Must derive from ImGUIWindow
        \param[in/out] Args
         Constructor arguments
        \return
         Pointer to the created window
        *************************************************************************/
        template<typename T, typename... Args>
        static T* CreateImGUIWindow(Args&&... args) {
            static_assert(std::is_base_of<ImGUIWindow, T>::value, "T must derive from ImGUIWindow");
            auto window = std::make_unique<T>(std::forward<Args>(args)...);
            T* ptr = window.get();
            m_Windows.emplace_back(std::move(window));
            return ptr;
        }
    };
}
