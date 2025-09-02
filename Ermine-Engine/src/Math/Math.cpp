#include "PreCompile.h"
#include "Math.h"
#include <cmath>

namespace Ermine
{
    /**********************************Vector2D***************************************/
#pragma region Vector2D

#pragma region Operators
    /*!***********************************************************************
    \brief
     Addition assignment operator.
    \param[in] rhs
     The vector to add.
    \return
     A reference to the vector.
    *************************************************************************/
    Vector2D& Vector2D::operator+=(const Vector2D& rhs)
    {
        x += rhs.x;
        y += rhs.y;
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
    Vector2D& Vector2D::operator-=(const Vector2D& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
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
    Vector2D& Vector2D::operator*=(float rhs)
    {
        x *= rhs;
        y *= rhs;
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
    Vector2D& Vector2D::operator/=(float rhs)
    {
        if (rhs != 0.0f)
        {
            x /= rhs;
            y /= rhs;
        }
        return *this;
    }

    /*!***********************************************************************
    \brief
     Unary negation operator.
    \return
     The negated vector.
    *************************************************************************/
    Vector2D Vector2D::operator-() const
    {
        return Vector2D(-x, -y);
    }

    /*!***********************************************************************
    \brief
     Get the perpendicular vector. To get the normal of an edge.
    \return
     The perpendicular vector.
    *************************************************************************/
    Vector2D Vector2D::perpendicular() const
    {
        return Vector2D(y, -x);
    }
#pragma endregion Operators

#pragma region Binary operators
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
    Vector2D operator+(const Vector2D& lhs, const Vector2D& rhs)
    {
        return Vector2D(lhs.x + rhs.x, lhs.y + rhs.y);
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
    Vector2D operator-(const Vector2D& lhs, const Vector2D& rhs)
    {
        return Vector2D(lhs.x - rhs.x, lhs.y - rhs.y);
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
    Vector2D operator*(const Vector2D& lhs, float rhs)
    {
        return Vector2D(lhs.x * rhs, lhs.y * rhs);
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
    Vector2D operator*(float lhs, const Vector2D& rhs)
    {
        return Vector2D(lhs * rhs.x, lhs * rhs.y);
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
    Vector2D operator/(const Vector2D& lhs, float rhs)
    {
        if (rhs == 0.0f)
            return lhs;
        return Vector2D(lhs.x / rhs, lhs.y / rhs);
    }

    /*!***********************************************************************
    \brief
     Equality operator for two Vector2D instances.
    \param[in] lhs
     The first vector to compare.
    \param[in] rhs
     The second vector to compare.
    \return
     True if both x and y components of lhs and rhs are equal, false otherwise.
    *************************************************************************/
    bool operator==(const Vector2D& lhs, const Vector2D& rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    /*!***********************************************************************
    \brief
     Inequality operator for two Vector2D instances.
    \param[in] lhs
     The first vector to compare.
    \param[in] rhs
     The second vector to compare.
    \return
     True if any component of lhs and rhs differ, false if both are equal.
    *************************************************************************/
    bool operator!=(const Vector2D& lhs, const Vector2D& rhs)
    {
        return !(lhs == rhs);
    }

#pragma endregion Binary operators

#pragma region Utility functions
    /*!***********************************************************************
    \brief
     Normalize a vector.
    \param[out] pResult
     The normalized vector.
    \param[in] vec
     The input vector.
    *************************************************************************/
    void Vec2Normalize(Vector2D& pResult, const Vector2D& vec)
    {
        float length = Vec2Length(vec);
        if (length != 0.0f)
        {
            pResult.x = vec.x / length;
            pResult.y = vec.y / length;
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
    float Vec2Length(const Vector2D& vec)
    {
        return sqrt(vec.x * vec.x + vec.y * vec.y);
    }

    /*!***********************************************************************
    \brief
     Calculate the squared length of a vector.
    \param[in] vec
     The input vector.
    \return
     The squared length of the vector.
    *************************************************************************/
    float Vec2SquareLength(const Vector2D& vec)
    {
        return vec.x * vec.x + vec.y * vec.y;
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
    float Vec2Distance(const Vector2D& lhs, const Vector2D& rhs)
    {
        return Vec2Length(lhs - rhs);
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
    float Vec2SquareDistance(const Vector2D& lhs, const Vector2D& rhs)
    {
        return Vec2SquareLength(lhs - rhs);
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
    float Vec2DotProduct(const Vector2D& lhs, const Vector2D& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y;
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
    float Vec2CrossProductMagnitude(const Vector2D& lhs, const Vector2D& rhs)
    {
        return lhs.x * rhs.y - lhs.y * rhs.x;
    }

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
    Vector2D Vec2Rotate(const Vector2D& vec, const float angle)
    {
        // x' = x * cos(theta) - y * sin(theta)
        // y' = x * sin(theta) - y * cos(theta)
        return {
            vec.x * std::cos(angle) - vec.y * std::sin(angle),
            vec.x * std::sin(angle) + vec.y * std::cos(angle)
        };
    }

    /*!***********************************************************************
    \brief
     Linearly interpolates between two vectors.
    \param[in] a
     The starting vector.
    \param[in] b
     The ending vector.
    \param[in] t
     Interpolation factor (0.0 = a, 1.0 = b).
    \return
     The interpolated vector.
    *************************************************************************/
    inline Vector2D Vec2Lerp(const Vector2D& a, const Vector2D& b, float t)
    {
        return Vector2D(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t
        );
    }

    /*!***********************************************************************
    \brief
     Clamps each component of a vector to a given range.
    \param[in] v
     The input vector.
    \param[in] minVal
     Minimum value for each component.
    \param[in] maxVal
     Maximum value for each component.
    \return
     A vector with each component clamped between minVal and maxVal.
    *************************************************************************/
    inline Vector2D Vec2Clamp(const Vector2D& v, float minVal, float maxVal)
    {
        return Vector2D(
            (v.x < minVal) ? minVal : (v.x > maxVal ? maxVal : v.x),
            (v.y < minVal) ? minVal : (v.y > maxVal ? maxVal : v.y)
        );
    }

    /*!***********************************************************************
    \brief
     Calculates the angle (in radians) between two vectors.
    \param[in] a
     The first vector.
    \param[in] b
     The second vector.
    \return
     The angle in radians between vectors a and b.
    *************************************************************************/
    inline float Vec2Angle(const Vector2D& a, const Vector2D& b)
    {
        float dot = Vec2DotProduct(a, b);
        float len = Vec2Length(a) * Vec2Length(b);
        return (len == 0.0f) ? 0.0f : acos(dot / len);
    }
#pragma endregion Utility functions

#pragma endregion Vector2D

    /**********************************Vector2D***************************************/
}
