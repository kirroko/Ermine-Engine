/* Start Header ************************************************************************/
/*!
\file       Vector3D.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       Sept 08, 2024
\brief      This file contains the declaration of the Vector3D structure.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

namespace Ermine
{
#ifdef _MSC_VER
    // Supress warning: nonstandard extension used : nameless struct/union
#pragma warning( disable : 4201 )
#endif

    /*!***********************************************************************
    \brief
     Represents a 3D vector with x, y and z components, or as an array.
    *************************************************************************/
    typedef union Vector3D
    {
        struct
        {
            float x, y, z;
        };

        float m[3];

        /*!***********************************************************************
        \brief
         Default constructor.
        *************************************************************************/
        Vector3D() : x(0.0f), y(0.0f), z(0.0f), m{ 0.0f } {}

        /*!***********************************************************************
        \brief
         Constructor with x and y parameters.
        \param[in] x
         The x coordinate.
        \param[in] y
         The y coordinate.
        \param[in] z
         The z coordinate.
        *************************************************************************/
        Vector3D(float x, float y, float z) : x(x), y(y), z(z), m{ x, y, z } {}

        /*!***********************************************************************
        \brief
         Copy constructor.
        \param[in] rhs
         The vector to copy.
        *************************************************************************/
        Vector3D(const Vector3D& rhs) = default;

        /*!***********************************************************************
        \brief
         Copy assignment operator.
        \param[in] rhs
         The vector to copy.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector3D& operator=(const Vector3D& rhs) = default;

        /*!***********************************************************************
        \brief
         Addition assignment operator.
        \param[in] rhs
         The vector to add.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector3D& operator+=(const Vector3D& rhs);

        /*!***********************************************************************
        \brief
         Subtraction assignment operator.
        \param[in] rhs
         The vector to subtract.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector3D& operator-=(const Vector3D& rhs);

        /*!***********************************************************************
        \brief
         Multiplication assignment operator.
        \param[in] rhs
         The scalar to multiply.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector3D& operator*=(float rhs);

        /*!***********************************************************************
        \brief
         Division assignment operator.
        \param[in] rhs
         The scalar to divide.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector3D& operator/=(float rhs);

        /*!***********************************************************************
        \brief
         Unary negation operator.
        \return
         The negated vector.
        *************************************************************************/
        Vector3D operator-() const;

    } Vector3D, Vec3;

#ifdef _MSC_VER
    // Supress warning: nonstandard extension used : nameless struct/union
#pragma warning( default : 4201 )
#endif

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
    Vector3D operator+(const Vector3D& lhs, const Vector3D& rhs);

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
    Vector3D operator-(const Vector3D& lhs, const Vector3D& rhs);

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
    Vector3D operator*(const Vector3D& lhs, float rhs);

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
    Vector3D operator*(float lhs, const Vector3D& rhs);

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
    Vector3D operator/(const Vector3D& lhs, float rhs);

    /*!***********************************************************************
    \brief
     Normalize a vector.
    \param[out] pResult
     The normalized vector.
    \param[in] vec
     The input vector.
    *************************************************************************/
    void Vec3Normalize(Vector3D& pResult, const Vector3D& vec);

    /*!***********************************************************************
    \brief
     Calculate the length of a vector.
    \param[in] vec
     The input vector.
    \return
     The length of the vector.
    *************************************************************************/
    float Vec3Length(const Vector3D& vec);

    /*!***********************************************************************
    \brief
     Calculate the squared length of a vector.
    \param[in] vec
     The input vector.
    \return
     The squared length of the vector.
    *************************************************************************/
    float Vec3SquareLength(const Vector3D& vec);

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
    float Vec3Distance(const Vector3D& lhs, const Vector3D& rhs);

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
    float Vec3SquareDistance(const Vector3D& lhs, const Vector3D& rhs);

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
    float Vec3DotProduct(const Vector3D& lhs, const Vector3D& rhs);

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
    float Vec3CrossProductMagnitude(const Vector3D& lhs, const Vector3D& rhs);

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
  void Vec3CrossProduct(Vector3D& pResult, const Vector3D& lhs, const Vector3D& rhs);

}