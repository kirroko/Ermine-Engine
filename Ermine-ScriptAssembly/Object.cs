/* Start Header ************************************************************************/
/*!
\file       Object.cs
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       19/08/2025
\brief      This file contains the base class for all objects in the Ermine Engine.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

using System.Dynamic;
using System.Runtime.CompilerServices;
using System.Threading;

namespace ErmineEngine
{
    public class Object
    {
        #region Properties
        public string name { get; set; }

        private long EntityID;
        #endregion

        #region Public Methods

        public long GetInstanceID() => EntityID;

        public string ToString() => name ?? base.ToString();

        public static bool operator ==(Object lhs, Object rhs)
        {
            if (ReferenceEquals(lhs, rhs)) return true;
            if (lhs is null || rhs is null) return false;
            return lhs.EntityID == rhs.EntityID;
        }

        public static bool operator !=(Object lhs, Object rhs) => !(lhs == rhs);
        #endregion
    }
}
