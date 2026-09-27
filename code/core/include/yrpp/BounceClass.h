/*
    Ballistic trajectory data used by Animations and Voxel Animations.
*/

#pragma once

#include "yrpp/GeneralStructures.h"
#include "yrpp/YRMathVector.h"
#include "yrpp/Matrix3D.h"

class BounceClass
{
public:
    enum class Status : int {
        None = 0,
        Bounce = 1,
        Impact = 2
    };

    // constructors
    BounceClass() = default;

    BounceClass(const CoordStruct& coords, double elasticity, double gravity,
        double maxVelocity, const Vector3D<float>& velocity, double angularVelocity)
    {
        this->Initialize(coords, elasticity, gravity, maxVelocity, velocity, angularVelocity);
    }

    /// VA: 0x004397E0
    void Initialize(const CoordStruct& coords, double elasticity, double gravity,
            double maxVelocity, const Vector3D<float>& velocity, double angularVelocity)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x4397E0); }
#else
        ;
#endif

    /// VA: 0x004399A0.
    CoordStruct* GetCoords(CoordStruct* pBuffer) const
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x4399A0); }
#else
        ;
#endif

    CoordStruct GetCoords() const {
        CoordStruct buffer;
        this->GetCoords(&buffer);
        return buffer;
    }

    /// VA: 0x004399E0.
    Matrix3D* GetDrawingMatrix(Matrix3D* pBuffer) const
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x4399E0); }
#else
        ;
#endif

    Matrix3D GetDrawingMatrix() const {
        Matrix3D buffer;
        this->GetDrawingMatrix(&buffer);
        return buffer;
    }

    /// VA: 0x00439B00.
    Status Update()
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x439B00); }
#else
        ;
#endif

    double Elasticity{ 0.0 }; // speed multiplier when bouncing off the ground
    double Gravity{ 0.0 }; // subtracted from the Z coords every frame
    double MaxVelocity{ 0.0 }; // 0.0 disables check
    Vector3D<float> Coords; // position with precision
    Vector3D<float> Velocity; // speed components
    Quaternion CurrentAngle; // quaternion for drawing
    Quaternion AngularVelocity; // second quaternion as per-frame delta
};
