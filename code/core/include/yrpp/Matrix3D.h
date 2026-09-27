#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/YRPPCore.h"
#include "yrpp/GeneralStructures.h"

#include "yrpp/Helpers/CompileTime.h"

template <typename T>
class Vector4D
{
public:
    static const Vector4D Empty;

    // no constructor, so this class stays aggregate and can be initialized using the curly braces {}
    T X, Y, Z, W;

    // TODO add Vector4 methods
};

template <typename T>
const Vector4D<T> Vector4D<T>::Empty = { T(), T(), T(), T() };

class Quaternion
{
public:
    constexpr Quaternion() :X { 0.0f }, Y { 0.0f }, Z { 0.0f }, W { 1.0f } { }
    constexpr Quaternion(float x, float y, float z, float w) : X { x }, Y { y }, Z { z }, W { w } { }
    constexpr Quaternion(const noinit_t&) { }

    constexpr void Normalize()
    {
        float mag = X * X + Y * Y + Z * Z + W * W;
        if (mag != 0.0)
        {
            X /= mag;
            Y /= mag;
            Z /= mag;
            W /= mag;
        }
    }

    constexpr void Scale(float scale)
    {
        X *= scale;
        Y *= scale;
        Z *= scale;
        W *= scale;
    }

    constexpr float& operator[](int idx)
    { return (&X)[idx]; }

    constexpr float operator[](int idx) const
    { return (&X)[idx]; }

    constexpr Quaternion& operator=(Quaternion& another)
    {
        X = another.X;
        Y = another.Y;
        Z = another.Z;
        W = another.W;
        return *this;
    }

    Quaternion Inverse(const Quaternion& value)
    { return Quaternion { -X,-Y,-Z,-W }; }

    Quaternion Conjugate(const Quaternion& value)
    { return Quaternion { -X,-Y,-Z,W }; }

    /// VA: 0x00646160.
    static Quaternion* YRPP_FASTCALL Trackball(Quaternion* ret, float x0, float y0, float x1, float y1, float radius)
    { JMP_STD(0x646160); }

    /// VA: 0x00646480.
    static Quaternion* YRPP_FASTCALL FromAxis(Quaternion* ret, const Vector3D<float>& src, float phi)
#if defined(RA2_YRPP_GAME)
    { JMP_STD(0x646480); }
#else
    noexcept;
#endif

    /// VA: 0x00646590.
    static Quaternion* YRPP_FASTCALL Slerp(Quaternion* ret, const Quaternion& a, const Quaternion& b, float alpha)
#if defined(RA2_YRPP_GAME)
    { JMP_STD(0x646590); }
#else
    noexcept;
#endif

    /// Global VA: 0x00B43188.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(Quaternion, [21], VoxelRampQuaternion, 0xB43188)
#else
    static Quaternion (&VoxelRampQuaternion)[21];
#endif

    float X;
    float Y;
    float Z;
    float W;
};

class Matrix3D
{
public:

    // Constructor
    explicit constexpr Matrix3D() :Data {} { }

    constexpr Matrix3D(const noinit_t&) { }

    // plain floats ctor
    constexpr Matrix3D(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23)
    {
        row[0][0] = m00; row[0][1] = m01; row[0][2] = m02; row[0][3] = m03;
        row[1][0] = m10; row[1][1] = m11; row[1][2] = m12; row[1][3] = m13;
        row[2][0] = m20; row[2][1] = m21; row[2][2] = m22; row[2][3] = m23;
        // JMP_THIS(0x5AE630);
    }

    // column vector ctor
    Matrix3D(
        Vector3D<float> const& x,
        Vector3D<float> const& y,
        Vector3D<float> const& z,
        Vector3D<float> const& pos)
    {
        row[0][0] = x.X; row[0][1] = y.X; row[0][2] = z.X; row[0][3] = pos.X;
        row[1][0] = x.Y; row[1][1] = y.Y; row[1][2] = z.Y; row[1][3] = pos.Y;
        row[2][0] = x.Z; row[2][1] = y.Z; row[2][2] = z.Z; row[2][3] = pos.Z;
        // JMP_THIS(0x5AE690);
    }

    // Terrain ramp
    /// VA: 0x005AE6F0
#if defined(RA2_YRPP_GAME)
    Matrix3D(float rotate_z, float rotate_x) { JMP_THIS(0x5AE6F0); }
#else
    Matrix3D(float rotate_z, float rotate_x) noexcept;
#endif

    // rotation along axis
    /// VA: 0x005AE750.
    Matrix3D(Vector3D<float>* axis, float angle) { JMP_THIS(0x5AE750); }

    // copy ctor// JMP_THIS(0x5AE610);
    constexpr Matrix3D(const Matrix3D& another) = default;
        
    constexpr Matrix3D(Matrix3D&& another) = default;

    constexpr Matrix3D& operator=(const Matrix3D& another) = default;

    constexpr Matrix3D& operator=(Matrix3D&& another) = default;

    // operators

    /// VA: 0x005AF980
    static constexpr Matrix3D* YRPP_FASTCALL MatrixMultiply(Matrix3D* ret, const Matrix3D* A, const Matrix3D* B)// { JMP_STD(0x5AF980); }
    {

        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
                ret->row[i][j] =
                A->row[i][0] * B->row[0][j] +
                A->row[i][1] * B->row[1][j] +
                A->row[i][2] * B->row[2][j];

            ret->row[i][3] =
                A->row[i][0] * B->row[0][3] +
                A->row[i][1] * B->row[1][3] +
                A->row[i][2] * B->row[2][3] +
                A->row[i][3];
        }

        return ret;
    }

    constexpr Matrix3D operator*(const Matrix3D& B) const
    {
        Matrix3D ret { noinit_t() };
        MatrixMultiply(&ret, this, &B);
        return ret;
    }

    constexpr void operator*=(const Matrix3D& another)
    {
        *this = *this * another;
    }

//	static Vector3D<float>* YRPP_FASTCALL MatrixApply(Vector3D<float>* ret, const Matrix3D* mat, const Vector3D<float>* vec) { JMP_STD(0x5AFB80); }
    constexpr Vector3D<float> operator*(const Vector3D<float>& point) const
    {
        return RotateVector(point) + GetTranslation();
    }

    /// VA: 0x005AE860
    constexpr void MakeIdentity()// { JMP_THIS(0x5AE860); } // 1-matrix
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 4; j++)
                row[i][j] = i == j ? 1.f : 0.f;
    }

    /// VA: 0x005AE890
    constexpr void Translate(float x, float y, float z) //{ JMP_THIS(0x5AE890); }
    {
        TranslateX(x);
        TranslateY(y);
        TranslateZ(z);
    }

    /// VA: 0x005AE8F0
    template<typename U>
    constexpr void Translate(Vector3D<U> const& vec) //{ JMP_THIS(0x5AE8F0); }
    {
        Translate(float(vec.X), float(vec.Y), float(vec.Z));
    }

    /// VA: 0x005AE980
    constexpr void TranslateX(float x) //{ JMP_THIS(0x5AE980); }
    {
        for (int i = 0; i < 3; i++)
            row[i][3] += x * row[i][0];
    }

    /// VA: 0x005AE9B0
    constexpr void TranslateY(float y) //{ JMP_THIS(0x5AE9B0); }
    {
        for (int i = 0; i < 3; i++)
            row[i][3] += y * row[i][1];
    }

    /// VA: 0x005AE9E0
    constexpr void TranslateZ(float z) //{ JMP_THIS(0x5AE9E0); }
    {
        for (int i = 0; i < 3; i++)
            row[i][3] += z * row[i][2];
    }

    constexpr void Scale(float factor) //{ JMP_THIS(0x5AEA10); }
    {
        for (float& i : Data)
            i *= factor;
    }

    constexpr void Scale(float x, float y, float z) //{ JMP_THIS(0x5AEA70); }
    {
        ScaleX(x);
        ScaleY(y);
        ScaleZ(z);
    }

    constexpr void ScaleX(float factor) //{ JMP_THIS(0x5AEAD0); }
    {
        for (int i = 0; i < 3; i++)
            row[i][0] *= factor;
    }

    constexpr void ScaleY(float factor) //{ JMP_THIS(0x5AEAF0); }
    {
        for (int i = 0; i < 3; i++)
            row[i][1] *= factor;
    }

    constexpr void ScaleZ(float factor) //{ JMP_THIS(0x5AEB20); }
    {
        for (int i = 0; i < 3; i++)
            row[i][2] *= factor;
    }

    /// VA: 0x005AEB50.
    void ShearYZ(float y, float z) { JMP_THIS(0x5AEB50); }
    /// VA: 0x005AEBA0.
    void ShearXY(float x, float y) { JMP_THIS(0x5AEBA0); }
    /// VA: 0x005AEBF0.
    void ShearXZ(float x, float z) { JMP_THIS(0x5AEBF0); }
    /// VA: 0x005AEC40.
    void PreRotateX(float theta) { JMP_THIS(0x5AEC40); }
    /// VA: 0x005AED50.
    void PreRotateY(float theta) { JMP_THIS(0x5AED50); }
    /// VA: 0x005AEE50.
    void PreRotateZ(float theta) { JMP_THIS(0x5AEE50); }
    /// VA: 0x005AEF60
#if defined(RA2_YRPP_GAME)
    void RotateX(float theta) { JMP_THIS(0x5AEF60); }
#else
    void RotateX(float theta) noexcept;
#endif
    /// VA: 0x005AF000.
    void RotateX(float Sin, float Cos) { JMP_THIS(0x5AF000); }
    /// VA: 0x005AF080.
#if defined(RA2_YRPP_GAME)
    void RotateY(float theta) { JMP_THIS(0x5AF080); }
#else
    void RotateY(float theta) noexcept;
#endif
    /// VA: 0x005AF120.
    void RotateY(float Sin, float Cos) { JMP_THIS(0x5AF120); }
    /// VA: 0x005AF1A0.
#if defined(RA2_YRPP_GAME)
    void RotateZ(float theta) { JMP_THIS(0x5AF1A0); }
#else
    void RotateZ(float theta);
#endif
    /// VA: 0x005AF240.
    void RotateZ(float Sin, float Cos) { JMP_THIS(0x5AF240); }

    constexpr float GetXVal() const //{ JMP_THIS(0x5AF2C0); }
    {
        return row[0][3];
    }

    constexpr float GetYVal() const //{ JMP_THIS(0x5AF310); }
    {
        return row[1][3];
    }

    constexpr float GetZVal() const //{ JMP_THIS(0x5AF360); }
    {
        return row[2][3];
    }

    constexpr Vector3D<float> GetTranslation() const
    {
        return { row[0][3], row[1][3], row[2][3] };
    }

    /// VA: 0x005AF3B0.
    float GetXRotation() const { JMP_THIS(0x5AF3B0); }
    /// VA: 0x005AF410.
    float GetYRotation() const { JMP_THIS(0x5AF410); }
    /// VA: 0x005AF470.
    float GetZRotation() const { JMP_THIS(0x5AF470); }

    /// VA: 0x005AF4D0
    Vector3D<float>* __RotateVector(Vector3D<float>* ret, const Vector3D<float>* rotate) const
#if defined(RA2_YRPP_GAME)
    { JMP_THIS(0x5AF4D0); }
#else
    noexcept;
#endif
    constexpr Vector3D<float> RotateVector(Vector3D<float> const& rotate) const
    {
        if(!std::is_constant_evaluated()) {
            Vector3D<float> result;__RotateVector(&result,&rotate);return result;
        }
        return {
            row[0][0] * rotate.X + row[0][1] * rotate.Y + row[0][2] * rotate.Z,
            row[1][0] * rotate.X + row[1][1] * rotate.Y + row[1][2] * rotate.Z,
            row[2][0] * rotate.X + row[2][1] * rotate.Y + row[2][2] * rotate.Z,
        };
    }

    /// VA: 0x005AF550.
    void LookAt1(Vector3D<float>& p, Vector3D<float>& t, float roll) { JMP_THIS(0x5AF550); }
    /// VA: 0x005AF710.
    void LookAt2(Vector3D<float>& p, Vector3D<float>& t, float roll) { JMP_THIS(0x5AF710); }

    /// VA: 0x005AFC20.
    static Matrix3D* YRPP_FASTCALL TransposeMatrix(Matrix3D* buffer, const Matrix3D* mat)
#if defined(RA2_YRPP_GAME)
    { JMP_STD(0x5AFC20); }
#else
    noexcept;
#endif
    static Matrix3D TransposeMatrix(const Matrix3D& mat)
    {
        Matrix3D buffer;
        TransposeMatrix(&buffer, &mat);
        return buffer;
    }
    void Transpose()
    {
        *this = TransposeMatrix(*this);
    }

    /// VA: 0x00646980.
    static Matrix3D* YRPP_FASTCALL FromQuaternion(Matrix3D* mat, const Quaternion* q)
#if defined(RA2_YRPP_GAME)
    { JMP_STD(0x646980); }
#else
    noexcept;
#endif
    static Matrix3D FromQuaternion(const Quaternion& q)
    {
        Matrix3D buffer { noinit_t() };
        FromQuaternion(&buffer, &q);
        return buffer;
    }

    /// VA: 0x00646730.
    static Quaternion* YRPP_FASTCALL ToQuaternion(Quaternion* ret, const Matrix3D* mat)
    { JMP_STD(0x646730); }

    Quaternion ToQuaternion() const
    {
        Quaternion ret { noinit_t() };
        ToQuaternion(&ret, this);
        return ret;
    }

    void ApplyQuaternion(const Quaternion& q)
    {
        *this = FromQuaternion(q) * *this;
    }

    /// Global VA: 0x00B44318.
    DEFINE_REFERENCE(Matrix3D, VoxelDefaultMatrix, 0xB44318)
    /// Global VA: 0x00B45188.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(Matrix3D, [21], VoxelRampMatrix, 0xB45188)
#else
    static Matrix3D (&VoxelRampMatrix)[21];
#endif

    static constexpr Matrix3D GetIdentity()
    {
        return Matrix3D { 1,0,0,0,0,1,0,0,0,0,1,0 };
    }

    // Properties
public:
    union
    {
        Vector4D<float> Row[3];
        float row[3][4];
        float Data[12];
    };
};
