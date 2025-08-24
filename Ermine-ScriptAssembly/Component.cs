/* Start Header ************************************************************************/
/*!
\file       Component.cs
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       22/08/2025
\brief      This file acts as base for all attachable data/logic units in the Ermine Engine.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    public abstract class Component : Object
    {
        public string tag
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
            [MethodImpl(MethodImplOptions.InternalCall)]
            set;
        }
    }
}
