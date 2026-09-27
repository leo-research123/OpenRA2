// pd's Vector classes (math sense)
#pragma once

#include <math.h>
#include <limits>

/*==========================================
============ 2D Vector =====================
==========================================*/

template <typename T> class Vector2D
{
public:
    static const Vector2D Empty;

    // no constructor, so this class stays aggregate and can be initialized using the curly braces {}
    T X,Y;

    // operator overloads
    // addition
    constexpr Vector2D operator+(const Vector2D& a) const
    {
        return Vector2D{ X + a.X, Y + a.Y };
    }
    // addition
    constexpr Vector2D& operator+=(const Vector2D& a)
    {
        X += a.X;
        Y += a.Y;
        return *this;
    }
    // substraction
    constexpr Vector2D operator-(const Vector2D& a) const
    {
        return Vector2D{ X - a.X, Y - a.Y };
    }
    // substraction
    constexpr Vector2D& operator-=(const Vector2D& a)
    {
        X -= a.X;
        Y -= a.Y;
        return *this;
    }
    // negation
    constexpr Vector2D operator-() const
    {
        return Vector2D{ -X, -Y };
    }
    // equality
    constexpr bool operator==(const Vector2D& a) const
    {
        return (X == a.X && Y == a.Y);
    }
    // unequality
    constexpr bool operator!=(const Vector2D& a) const
    {
        return (X != a.X || Y != a.Y);
    }
    // scalar multiplication
    constexpr Vector2D operator*(double r) const
    {
        return Vector2D{ static_cast<T>(X * r), static_cast<T>(Y * r) };
    }
    // scalar multiplication
    constexpr Vector2D& operator*=(double r)
    {
        X = static_cast<T>(X * r);
        Y = static_cast<T>(Y * r);
        return *this;
    }
    // vector multiplication
    constexpr double operator*(const Vector2D& a) const
    {
        return static_cast<double>(X) * a.X + static_cast<double>(Y) * a.Y;
    }
    // magnitude
    double Magnitude() const
    {
        return sqrt(MagnitudeSquared());
    }
    // magnitude squared
    constexpr double MagnitudeSquared() const
    {
        return *this * *this;
    }
    // distance from another vector
    double DistanceFrom(const Vector2D& a) const
    {
        return (a - *this).Magnitude();
    }
    // distance from another vector, squared
    constexpr double DistanceFromSquared(const Vector2D& a) const
    {
        return (a - *this).MagnitudeSquared();
    }
    // collinearity
    constexpr bool IsCollinearTo(const Vector2D& a) const
    {
        auto const c = static_cast<double>(X) * a.Y;
        return c == static_cast<double>(Y) * a.X;
    }
    // find scalar
    constexpr double FindScalar(const Vector2D& a) const
    {
        double r = static_cast<double>(a.X) / static_cast<double>(X);
        if (static_cast<T>(r * Y) == a.Y)
            return r;
        else
            return std::numeric_limits<double>::quiet_NaN();
    }
    // calculate angle sin
    constexpr double AngleSinTo(const Vector2D& a) const
    {
        return (X * a.Y - Y * a.X) / (Magnitude() * a.Magnitude());
    }
    // calculate angle cos
    constexpr double AngleCosTo(const Vector2D& a) const
    {
        return (*this * a) / (Magnitude() * a.Magnitude());
    }
};

template <typename T>
const Vector2D<T> Vector2D<T>::Empty = {T(), T()};

/*==========================================
============ 3D Vector =====================
==========================================*/

template <typename T> class Vector3D
{
public:
    static const Vector3D Empty;

    // no constructor, so this class stays aggregate and can be initialized using the curly braces {}
    T X,Y,Z;

    // operator overloads
    // addition
    constexpr Vector3D operator+(const Vector3D& a) const
    {
        return Vector3D{ X + a.X, Y + a.Y, Z + a.Z };
    }
    // addition
    constexpr Vector3D& operator+=(const Vector3D& a)
    {
        X += a.X;
        Y += a.Y;
        Z += a.Z;
        return *this;
    }
    // substraction
    constexpr Vector3D operator-(const Vector3D& a) const
    {
        return Vector3D{ X - a.X, Y - a.Y, Z - a.Z };
    }
    // substraction
    constexpr Vector3D& operator-=(const Vector3D& a)
    {
        X -= a.X;
        Y -= a.Y;
        Z -= a.Z;
        return *this;
    }
    // negation
    constexpr Vector3D operator-() const
    {
        return Vector3D{ -X, -Y, -Z };
    }
    // equality
    constexpr bool operator==(const Vector3D& a) const
    {
        return (X == a.X && Y == a.Y && Z == a.Z);
    }
    // scalar multiplication
    constexpr Vector3D operator*(double r) const
    {
        return Vector3D{
            static_cast<T>(X * r),
            static_cast<T>(Y * r),
            static_cast<T>(Z * r) };
    }
    // scalar multiplication
    constexpr Vector3D& operator*=(double r)
    {
        X = static_cast<T>(X * r);
        Y = static_cast<T>(Y * r);
        Z = static_cast<T>(Z * r);
        return *this;
    }
    // scalar division
    constexpr Vector3D operator/(double r) const
    {
        return Vector3D {
            static_cast<T>(X / r),
            static_cast<T>(Y / r),
            static_cast<T>(Z / r) };
    }
    // scalar division
    constexpr Vector3D& operator/=(double r)
    {
        X /= r;
        Y /= r;
        Z /= r;
        return *this;
    }
    // vector multiplication
    constexpr double operator*(const Vector3D& a) const
    {
        return static_cast<double>(X) * a.X
            + static_cast<double>(Y) * a.Y
            + static_cast<double>(Z) * a.Z;
    }
    // magnitude
    double Magnitude() const
    {
        return sqrt(MagnitudeSquared());
    }
    // magnitude squared
    constexpr double MagnitudeSquared() const
    {
        return *this * *this;
    }
    // distance from another vector
    double DistanceFrom(const Vector3D& a) const
    {
        return (a - *this).Magnitude();
    }
    // distance from another vector, squared
    constexpr double DistanceFromSquared(const Vector3D& a) const
    {
        return (a - *this).MagnitudeSquared();
    }
    // collinearity
    constexpr bool IsCollinearTo(const Vector3D& a) const
    {
        return CrossProduct(a).MagnitudeSquared() == 0;
    }
    // find scalar
    constexpr double FindScalar(const Vector3D& a) const
    {
        double r = static_cast<double>(a.X) / static_cast<double>(X);
        if ((static_cast<T>(r * Y) == a.Y) && (static_cast<T>(r * Z) == a.Z))
            return r;
        else
            return std::numeric_limits<double>::quiet_NaN();
    }
    // cross product
    constexpr Vector3D CrossProduct(const Vector3D& a) const
    {
        return Vector3D{
            Y * a.Z - Z * a.Y,
            Z * a.X - X * a.Z,
            X * a.Y - Y * a.X };
    }
    // normalize
    Vector3D Normalized() const
    {
        double magnitude = this->Magnitude();
        return magnitude > 0.0 ? *this / magnitude : Vector3D::Empty;
    }
};

template <typename T>
const Vector3D<T> Vector3D<T>::Empty = {T(), T(), T()};
