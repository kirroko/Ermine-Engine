/* Start Header ************************************************************************/
/*!
\file       Vector4D.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       19/03/2025
\brief      This file contains the definition of the Vector4D structure.
Copyright (C) 2025 TwoJumpingRabbits
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
     Represents a 4D vector with x, y, z and w components, or as an array.
    *************************************************************************/
    typedef union Vector4D
    {
     
        struct
        {
            float x,y,z,w;
        };

        float m[4];
        
        /*!***********************************************************************
        \brief
         Default constructor.
        *************************************************************************/
        Vector4D() : x(0), y(0), z(0), w(0) {}

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
        Vector4D(float x, float y, float z, float w) : x(x), y(y), z(z), w(w), m{ x, y, z, w }
        {
        }

        /*!***********************************************************************
        \brief
         Copy constructor.
        \param[in] rhs
         The vector to copy.
        *************************************************************************/
        Vector4D(const Vector4D& rhs) = default;

        /*!***********************************************************************
        \brief
         Copy assignment operator.
        \param[in] rhs
         The vector to copy.
        \return
         A reference to the vector.
        *************************************************************************/
        Vector4D& operator=(const Vector4D& rhs) = default;

     /*!***********************************************************************
        \brief
         Addition assignment operator.
        \param[in] rhs
         The vector to add.
        \return
         A reference to the vector.
        *************************************************************************/
     Vector4D& operator+=(const Vector4D& rhs);

     /*!***********************************************************************
     \brief
      Subtraction assignment operator.
     \param[in] rhs
      The vector to subtract.
     \return
      A reference to the vector.
     *************************************************************************/
     Vector4D& operator-=(const Vector4D& rhs);

     /*!***********************************************************************
     \brief
      Multiplication assignment operator.
     \param[in] rhs
      The scalar to multiply.
     \return
      A reference to the vector.
     *************************************************************************/
     Vector4D& operator*=(float rhs);

     /*!***********************************************************************
     \brief
      Division assignment operator.
     \param[in] rhs
      The scalar to divide.
     \return
      A reference to the vector.
     *************************************************************************/
     Vector4D& operator/=(float rhs);

     /*!***********************************************************************
     \brief
      Unary negation operator.
     \return
      The negated vector.
     *************************************************************************/
     Vector4D operator-() const;
    } Vector4D, Vec4;

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
    Vector4D operator+(const Vector4D& lhs, const Vector4D& rhs);

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
    Vector4D operator-(const Vector4D& lhs, const Vector4D& rhs);

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
    Vector4D operator*(const Vector4D& lhs, float rhs);

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
    Vector4D operator*(float lhs, const Vector4D& rhs);

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
    Vector4D operator/(const Vector4D& lhs, float rhs);
}
