/* Start Header ************************************************************************/
/*!
\file       GeometryFactory.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       15/03/2025
\brief      This reflects the brief

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "Components.h"
#include "Math.h"

namespace Ermine::graphics
{
    /**
     * @biref a static class that generate geometry
     */
    class GeometryFactory
    {
        struct Vertex
        {
            Vec3 pos;
            Vec3 norms;
            Vec2 tex;
        };

        GeometryFactory() = default;
        ~GeometryFactory() = default;
    public:
        static GeometryFactory& GetInstance()
        {
            static GeometryFactory instance;
            return instance;
        }

        /**
         * @brief Create a cube
         * 
         * @param width The width of the cube
         * @param height The height of the cube
         * @param depth The depth of the cube
         * @return Mesh The cube mesh
         */
        static Mesh CreateCube(float width = 1.0f, float height = 1.0f, float depth = 1.0f);

        /**
         * @brief Create a quad
         * 
         * @param width The width of the quad
         * @param height The height of the quad
         * @return Mesh The quad mesh
         */
        static Mesh CreateQuad(float width = 1.0f, float height = 1.0f);

        /**
         * @brief Create a sphere
         * 
         * @param radius The radius of the sphere
         * @param sectors The number of sectors
         * @param stacks The number of stacks
         * @return Mesh The sphere mesh
         */
        static Mesh CreateSphere(float radius = 1.0f, unsigned int sectors = 36, unsigned int stacks = 18);

        /**
         * @brief Structure for Axis-Aligned Bounding Box
         */
        struct AABB
        {
            Vec3 min;  // Minimum corner
            Vec3 max;  // Maximum corner
            
            // Get center of the AABB
            Vec3 GetCenter() const { return Vec3((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f); }
            
            // Get size/extents of the AABB
            Vec3 GetSize() const { return Vec3(max.x - min.x, max.y - min.y, max.z - min.z); }
        };

        /**
         * @brief Calculate AABB for a mesh
         * 
         * @param mesh The mesh to calculate AABB for
         * @return AABB The calculated axis-aligned bounding box
         */
        static AABB CalculateAABB(const Mesh& mesh);

        /**
         * @brief Calculate AABB for a primitive type
         * 
         * @param type The type of primitive ("Cube", "Quad", "Sphere", etc.)
         * @param size The size/dimensions of the primitive
         * @return AABB The calculated axis-aligned bounding box
         */
        static AABB CalculatePrimitiveAABB(const std::string& type, const Vec3& size);
    };
}
