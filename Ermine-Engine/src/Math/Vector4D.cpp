/* Start Header ************************************************************************/
/*!
\file       Vector4D.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       19/03/2025
\brief      This file contains the definition of the Vector4D structure.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Vector4D.h"

using namespace Ermine;

/*!***********************************************************************
    \brief
     Addition assignment operator.
    \param[in] rhs
     The vector to add.
    \return
     A reference to the vector.
    *************************************************************************/
    Vector4D& Vector4D::operator+=(const Vector4D& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
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
    Vector4D& Vector4D::operator-=(const Vector4D& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
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
    Vector4D& Vector4D::operator*=(float rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        w *= rhs;
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
    Vector4D& Vector4D::operator/=(float rhs)
    {
        if (rhs != 0.0f)
        {
            x /= rhs;
            y /= rhs;
            z /= rhs;
            w /= rhs;
        }
        return *this;
    }

    /*!***********************************************************************
    \brief
     Unary negation operator.
    \return
     The negated vector.
    *************************************************************************/
    Vector4D Vector4D::operator-() const
    {
        return {-x, -y, -z, -w};
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
    Vector4D operator+(const Vector4D& lhs, const Vector4D& rhs)
    {
        return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
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
    Vector4D operator-(const Vector4D& lhs, const Vector4D& rhs)
    {
        return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
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
    Vector4D operator*(const Vector4D& lhs, float rhs)
    {
        return {lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs};
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
    Vector4D operator*(float lhs, const Vector4D& rhs)
    {
        return {lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w};
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
    Vector4D operator/(const Vector4D& lhs, float rhs)
    {
        if (rhs == 0.0f)
            return lhs;
        return {lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs};
    }