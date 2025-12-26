#pragma once
struct FVector
{
    float X;
    float Y;
    float Z;

    FVector() : X(0), Y(0), Z(0) {}
    FVector(float x, float y, float z) : X(x), Y(y), Z(z) {}

    FVector operator-(const FVector& Other) const
    {
        return FVector(X - Other.X, Y - Other.Y, Z - Other.Z);
    }

    float Dot(const FVector& Other) const
    {
        return X * Other.X + Y * Other.Y + Z * Other.Z;
    }
};


struct FPlane
{
    float X;
    float Y;
    float Z;
    float W;

    FPlane() : X(0), Y(0), Z(0), W(0) {}
    FPlane(float x, float y, float z, float w)
        : X(x), Y(y), Z(z), W(w) {
    }
};


struct FMatrix
{
    FPlane XPlane; // row 0
    FPlane YPlane; // row 1
    FPlane ZPlane; // row 2
    FPlane WPlane; // row 3

    FMatrix()
        : XPlane(1, 0, 0, 0),
        YPlane(0, 1, 0, 0),
        ZPlane(0, 0, 1, 0),
        WPlane(0, 0, 0, 1)
    {
    }
};


struct FVector2D
{
    float X;
    float Y;

    FVector2D() : X(0.f), Y(0.f) {}
    FVector2D(float x, float y) : X(x), Y(y) {}
};

struct FRotator
{
    float Pitch; // X axis (look up / down)
    float Yaw;   // Z axis (look left / right)
    float Roll;  // Y axis (tilt)

    FRotator() : Pitch(0.f), Yaw(0.f), Roll(0.f) {}
    FRotator(float pitch, float yaw, float roll)
        : Pitch(pitch), Yaw(yaw), Roll(roll) {
    }
};