/* Start Header ************************************************************************/
/*!
\file       Vector2D.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       Nov 06, 2024
\brief      This file contains the declaration of the Vector2D structure.

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
     Represents a 2D vector with x and y components, or as an array.
    *************************************************************************/
    typedef union Vector2D
    {
        struct
        {
            float x, y;
        };

        float m[2];

        /*!***********************************************************************
        \brief
         Default constructor.
        *************************************************************************/
        Vector2D() : x(0.0f), y(0.0f), m{ 0.0f } {}

        /*!***********************************************************************
        \brief
         Constructor with x and y parameters.
        \param[in] x
         The x coordinate.
        \param[in] y
         The y coordinate.
        *************************************************************************/
        Vector2D(float x, float y) : x(x), y(y), m{ x, y } {}

        /*!***********************************************************************
        \brief
         Copy constructor.
        \param[in] rhs
         The vector to copy.
        *************************************************************************/
        Vector2D(const Vector2D& rhs) = default;

        /*!***********************************************************************
        \brief
         Copy assignment operator.
        \param[in] rhs
         The vector to copy.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector2D& operator=(const Vector2D& rhs) = default;

        /*!***********************************************************************
        \brief
         Addition assignment operator.
        \param[in] rhs
         The vector to add.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector2D& operator+=(const Vector2D& rhs);

        /*!***********************************************************************
        \brief
         Subtraction assignment operator.
        \param[in] rhs
         The vector to subtract.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector2D& operator-=(const Vector2D& rhs);

        /*!***********************************************************************
        \brief
         Multiplication assignment operator.
        \param[in] rhs
         The scalar to multiply.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector2D& operator*=(float rhs);

        /*!***********************************************************************
        \brief
         Division assignment operator.
        \param[in] rhs
         The scalar to divide.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector2D& operator/=(float rhs);

        /*!***********************************************************************
        \brief
         Unary negation operator.
        \return
         The negated vector.
        *************************************************************************/
        Vector2D operator-() const;

        /*!***********************************************************************
        \brief
         Get the perpendicular vector. To get the normal of an edge.
        \return
         The perpendicular vector.
        *************************************************************************/
        Vector2D perpendicular() const;

    } Vector2D, Vec2;

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
    Vector2D operator+(const Vector2D& lhs, const Vector2D& rhs);

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
    Vector2D operator-(const Vector2D& lhs, const Vector2D& rhs);

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
    Vector2D operator*(const Vector2D& lhs, float rhs);

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
    Vector2D operator*(float lhs, const Vector2D& rhs);

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
    Vector2D operator/(const Vector2D& lhs, float rhs);

    /*!***********************************************************************
    \brief
     Normalize a vector.
    \param[out] pResult
     The normalized vector.
    \param[in] vec
     The input vector.
    *************************************************************************/
    void Vec2Normalize(Vector2D& pResult, const Vector2D& vec);

    /*!***********************************************************************
    \brief
     Calculate the length of a vector.
    \param[in] vec
     The input vector.
    \return
     The length of the vector.
    *************************************************************************/
    float Vec2Length(const Vector2D& vec);

    /*!***********************************************************************
    \brief
     Calculate the squared length of a vector.
    \param[in] vec
     The input vector.
    \return
     The squared length of the vector.
    *************************************************************************/
    float Vec2SquareLength(const Vector2D& vec);

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
    float Vec2Distance(const Vector2D& lhs, const Vector2D& rhs);

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
    float Vec2SquareDistance(const Vector2D& lhs, const Vector2D& rhs);

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
    float Vec2DotProduct(const Vector2D& lhs, const Vector2D& rhs);

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
    float Vec2CrossProductMagnitude(const Vector2D& lhs, const Vector2D& rhs);

    /*!***********************************************************************
    \brief
     Rotate a vector by the given angle.
    \param[in] vec
     The vector to rotate.
    \param[in] angle
     The given angle to rotate by.
    \return
     The rotated vector.
    *************************************************************************/
    Vector2D Vec2Rotate(const Vector2D& vec, const float angle);
}