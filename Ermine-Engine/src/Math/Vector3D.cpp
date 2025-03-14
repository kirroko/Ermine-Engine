/* Start Header ************************************************************************/
/*!
\file       Vector3D.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       Sept 08, 2024
\brief      This file contains the definition of the Vector3D structure.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Vector3D.h" // for forward declaration
#include <cmath>      // for sqrt

namespace Ermine
{
    /*!***********************************************************************
    \brief
     Addition assignment operator.
    \param[in] rhs
     The vector to add.
    \return
     A reference to the vector.
    *************************************************************************/
    Vector3D& Vector3D::operator+=(const Vector3D& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    /*!***********************************************************************
    \brief
     Subtraction assignment operator.
    \param[in] rhs
     The vector to subtract.
    \return
     A reference to the vector.
    *************************************************************************/
    Vector3D& Vector3D::operator-=(const Vector3D& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    /*!***********************************************************************
    \brief
     Multiplication assignment operator.
    \param[in] rhs
     The scalar to multiply.
    \return
     A reference to the vector.
    *************************************************************************/
    Vector3D& Vector3D::operator*=(float rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }

    /*!***********************************************************************
    \brief
     Division assignment operator.
    \param[in] rhs
     The scalar to divide.
    \return
     A reference to the vector.
    *************************************************************************/
    Vector3D& Vector3D::operator/=(float rhs)
    {
        if (rhs != 0.0f)
        {
            x /= rhs;
            y /= rhs;
            z /= rhs;
        }
        return *this;
    }

    /*!***********************************************************************
    \brief
     Unary negation operator.
    \return
     The negated vector.
    *************************************************************************/
    Vector3D Vector3D::operator-() const
    {
        return Vector3D(-x, -y, -z);
    }

    /*!***********************************************************************
    \brief
     Binary addition operator for two vectors.
    \param[in] lhs
     The first vector to add.
    \param[in] rhs
     The second vector to add.
    \return
     The vector of the sum of the two vectors.
    *************************************************************************/
    Vector3D operator+(const Vector3D& lhs, const Vector3D& rhs)
    {
        return Vector3D(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
    }

    /*!***********************************************************************
    \brief
     Binary subtraction operator for two vectors.
    \param[in] lhs
     The first vector to subtract.
    \param[in] rhs
     The second vector to subtract.
    \return
     The vector of the difference of the two vectors.
    *************************************************************************/
    Vector3D operator-(const Vector3D& lhs, const Vector3D& rhs)
    {
        return Vector3D(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
    }

    /*!***********************************************************************
    \brief
     Scalar multiplication operator for a vector and a scalar.
    \param[in] lhs
     The vector to multiply.
    \param[in] rhs
     The scalar to multiply.
    \return
     The vector of the product of the vector and scalar.
    *************************************************************************/
    Vector3D operator*(const Vector3D& lhs, float rhs)
    {
        return Vector3D(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
    }

    /*!***********************************************************************
    \brief
     Scalar multiplication operator for a scalar and a vector.
    \param[in] lhs
     The scalar to multiply.
    \param[in] rhs
     The vector to multiply.
    \return
     The vector of the product of the scalar and vector.
    *************************************************************************/
    Vector3D operator*(float lhs, const Vector3D& rhs)
    {
        return Vector3D(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z);
    }

    /*!***********************************************************************
    \brief
     Scalar division operator for a vector and a scalar.
    \param[in] lhs
     The vector to divide.
    \param[in] rhs
     The scalar to divide.
    \return
     The vector of the division of the vector and scalar.
    *************************************************************************/
    Vector3D operator/(const Vector3D& lhs, float rhs)
    {
        if (rhs == 0.0f)
            return lhs;
        return Vector3D(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
    }

    /*!***********************************************************************
    \brief
     Normalize a vector.
    \param[out] pResult
     The normalized vector.
    \param[in] vec
     The input vector.
    *************************************************************************/
    void Vec3Normalize(Vector3D& pResult, const Vector3D& vec)
    {
        float length = Vec3Length(vec);
        if (length != 0.0f)
        {
            pResult.x = vec.x / length;
            pResult.y = vec.y / length;
            pResult.z = vec.z / length;
        }
    }

    /*!***********************************************************************
    \brief
     Calculate the length of a vector.
    \param[in] vec
     The input vector.
    \return
     The length of the vector.
    *************************************************************************/
    float Vec3Length(const Vector3D& vec)
    {
        return sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    }

    /*!***********************************************************************
    \brief
     Calculate the squared length of a vector.
    \param[in] vec
     The input vector.
    \return
     The squared length of the vector.
    *************************************************************************/
    float Vec3SquareLength(const Vector3D& vec)
    {
        return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
    }

    /*!***********************************************************************
    \brief
     Calculate the distance of two vectors.
    \param[in] lhs
     The first vector.
    \param[in] rhs
     The second vector.
    \return
     The distance of the two vectors.
    *************************************************************************/
    float Vec3Distance(const Vector3D& lhs, const Vector3D& rhs)
    {
        return Vec3Length(lhs - rhs);
    }

    /*!***********************************************************************
    \brief
     Calculate the squared distance of two vectors.
    \param[in] lhs
     The first vector.
    \param[in] rhs
     The second vector.
    \return
     The squared distance of the two vectors.
    *************************************************************************/
    float Vec3SquareDistance(const Vector3D& lhs, const Vector3D& rhs)
    {
        return Vec3SquareLength(lhs - rhs);
    }

    /*!***********************************************************************
    \brief
     Calculate the dot product of two vectors.
    \param[in] lhs
     The first vector.
    \param[in] rhs
     The second vector.
    \return
     The dot product of the two vectors.
    *************************************************************************/
    float Vec3DotProduct(const Vector3D& lhs, const Vector3D& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    /*!***********************************************************************
    \brief
     Calculate the magnitude of the cross product of two vectors.
    \param[in] lhs
     The first vector.
    \param[in] rhs
     The second vector.
    \return
     The magnitude of the cross product of the two vectors.
    *************************************************************************/
    float Vec3CrossProductMagnitude(const Vector3D& lhs, const Vector3D& rhs)
    {
        Vector3D cross_product = Vector3D(
            lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x);
        return Vec3Length(cross_product);
    }

    /*!***********************************************************************
    \brief
     Calculate the cross product of two vectors.
    \param[out] pResult
     The cross product of the two vectors.
    \param[in] lhs
     The first vector.
    \param[in] rhs
     The second vector.
    *************************************************************************/
    void Vec3CrossProduct(Vector3D& pResult, const Vector3D& lhs, const Vector3D& rhs)
    {
     pResult.x = lhs.y * rhs.z - lhs.z * rhs.y;
     pResult.y = lhs.z * rhs.x - lhs.x * rhs.z;
     pResult.z = lhs.x * rhs.y - lhs.y * rhs.x;
    }
}
