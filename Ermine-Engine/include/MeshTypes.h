/* Start Header ************************************************************************/
/*!
\file       MeshTypes.h
\author     Ridhwan Afandi, mohamedridhwan.b, 2301367, mohamedridhwan.b\@digipen.edu
\date       27/09/2025
\brief      This file contains the declaration of Mesh types and structures.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "glm/glm.hpp"
#include "cstdint"
#include "string"

namespace Ermine::graphics {

    // Vertex structure matching GPU std430 layout
    // std430: vec3 has base alignment of 16 bytes (aligned like vec4)
    struct Vertex {
        glm::vec3 position;     // offset 0, size 12 bytes
        float _pad0;            // offset 12, size 4 bytes (padding to reach 16)
        glm::vec3 normal;       // offset 16, size 12 bytes
        float _pad1;            // offset 28, size 4 bytes (padding to reach 32)
        glm::vec2 texCoord;     // offset 32, size 8 bytes
        float _pad2[2];         // offset 40, size 8 bytes (padding to reach 48)
        glm::vec3 tangent;      // offset 48, size 12 bytes
        float _pad3;            // offset 60, size 4 bytes (padding to reach 64)
        // Total: 64 bytes matching std430 layout
    };
	static_assert(sizeof(Vertex) == 64, "Vertex size mismatch - expected 64 bytes for std430 layout");

    // SkinnedVertex structure matching GPU std430 layout
    struct SkinnedVertex {
        glm::vec3 position;     // offset 0, size 12 bytes
        float _pad0;            // offset 12, size 4 bytes (padding to reach 16)
        glm::vec3 normal;       // offset 16, size 12 bytes
        float _pad1;            // offset 28, size 4 bytes (padding to reach 32)
        glm::vec2 texCoord;     // offset 32, size 8 bytes
        float _pad2[2];         // offset 40, size 8 bytes (padding to reach 48)
        glm::vec3 tangent;      // offset 48, size 12 bytes
        float _pad3;            // offset 60, size 4 bytes (padding to reach 64)
        glm::ivec4 boneIDs;     // offset 64, size 16 bytes
        glm::vec4 boneWeights;  // offset 80, size 16 bytes
        // Total: 96 bytes matching std430 layout
    };
    static_assert(sizeof(SkinnedVertex) == 96, "SkinnedVertex size mismatch - expected 96 bytes for std430 layout");

    struct MeshSubset  {
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t baseVertex;
        std::string meshID;
    };

    struct MeshHandle {
        uint32_t index = UINT32_MAX;
        bool isValid() const { return index != UINT32_MAX; }
    };

}  //namespace graphics