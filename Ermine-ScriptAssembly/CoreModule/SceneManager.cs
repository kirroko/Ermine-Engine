/* Start Header ************************************************************************/
/*!
\file       SceneManager.cs
\author     Claude Code
\date       11/2025
\brief      SceneManager class for loading scenes from C# scripts.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    public static class SceneManager
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void LoadSceneInternal(string scenePath);

        /// <summary>
        /// Loads a scene by file path.
        /// </summary>
        /// <param name="scenePath">Path to the scene file (e.g., "../Resources/Scenes/level.scene")</param>
        public static void LoadScene(string scenePath)
        {
            if (string.IsNullOrEmpty(scenePath))
            {
                Debug.LogError("SceneManager.LoadScene: Scene path is null or empty!");
                return;
            }

            Debug.Log($"SceneManager: Loading scene '{scenePath}'");
            LoadSceneInternal(scenePath);
        }
    }
}
