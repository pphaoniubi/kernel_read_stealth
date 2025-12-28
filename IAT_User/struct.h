#pragma once
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "unreal.h"

constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;
constexpr float RAD2DEG = 180.0f / 3.14159265358979323846f;
constexpr float M_PI = 3.14159265358979323846f;

struct Vec3
{
    float x, y, z;

    Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator*(float f) const { return { x * f, y * f, z * f }; }
};

inline float Dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline Vec3 Normalize(const Vec3& v)
{
    float len = std::sqrt(Dot(v, v));
    return { v.x / len, v.y / len, v.z / len };
}

struct Matrix4x4
{
    float m[4][4];

    Matrix4x4 operator*(const Matrix4x4& other) const
    {
        Matrix4x4 result{};

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                result.m[row][col] =
                    m[row][0] * other.m[0][col] +
                    m[row][1] * other.m[1][col] +
                    m[row][2] * other.m[2][col] +
                    m[row][3] * other.m[3][col];
            }
        }

        return result;
    }

};

float NormalizePitch(float& pitchDeg)
{
    // Bring into [0, 360)
    pitchDeg = std::fmod(pitchDeg, 360.0f);
    if (pitchDeg < 0.0f)
        pitchDeg += 360.0f;

    // Convert 270–360 range to negative angles
    if (pitchDeg > 180.0f)
        pitchDeg -= 360.0f;

    // Clamp to safe range
    return std::clamp(pitchDeg, -89.0f, 89.0f);
}

void PrintMatrix(const Matrix4x4& mat)
{
    std::cout << std::fixed << std::setprecision(3);

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            std::cout << std::setw(10) << mat.m[row][col] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

struct Vec2
{
    float x, y;

};


// Assumption based on you are the host
const std::vector<std::uintptr_t>& enemy_pos_offset =        { 0x30, 0xB8, 0x120, 0x238, 0x8, 0x280, 0x130 ,0x11C };

const std::vector<std::uintptr_t>& local_pitch_yaw_offset =  { 0x30, 0xB8, 0x180, 0x38, 0x0, 0x30, 0x2B8, 0xE8C };

const std::vector<std::uintptr_t>& local_camera_pos_offset = { 0x30, 0xB8, 0x180, 0x38, 0x0, 0x30, 0x2B8, 0xE80 };

const std::vector<std::uintptr_t>& local_fov_offset =        { 0x30, 0xB8, 0x180, 0x38, 0x0, 0x30, 0x2B8, 0xE98 };


FMatrix ToMatrixB(FVector Rotation, FVector Origin = FVector{ 0.f,0.f,0.f })
{
    float Pitch = (Rotation.X * float(M_PI) / 180.f);
    float Yaw = (Rotation.Y * float(M_PI) / 180.f);
    float Roll = (Rotation.Z * float(M_PI) / 180.f);

    float SP = sinf(Pitch);
    float CP = cosf(Pitch);
    float SY = sinf(Yaw);
    float CY = cosf(Yaw);
    float SR = sinf(Roll);
    float CR = cosf(Roll);

    FMatrix Matrix;

    // 第一行 (XPlane)
    Matrix.XPlane.X = CP * CY;
    Matrix.XPlane.Y = CP * SY;
    Matrix.XPlane.Z = SP;
    Matrix.XPlane.W = 0.f;

    // 第二行 (YPlane)
    Matrix.YPlane.X = SR * SP * CY - CR * SY;
    Matrix.YPlane.Y = SR * SP * SY + CR * CY;
    Matrix.YPlane.Z = -SR * CP;
    Matrix.YPlane.W = 0.f;

    // 第三行 (ZPlane)
    Matrix.ZPlane.X = -(CR * SP * CY + SR * SY);
    Matrix.ZPlane.Y = CY * SR - CR * SP * SY;
    Matrix.ZPlane.Z = CR * CP;
    Matrix.ZPlane.W = 0.f;

    // 第四行 (WPlane) - 平移分量
    Matrix.WPlane.X = Origin.X;
    Matrix.WPlane.Y = Origin.Y;
    Matrix.WPlane.Z = Origin.Z;
    Matrix.WPlane.W = 1.f;

    return Matrix;
}

bool WorldToScreen(FVector Location, OUT FVector2D& Screen, FVector CameraLocation, FRotator CameraRotation, float FOV, float ScreenCenterX, float ScreenCenterY) {
    FVector Rotation{};
    Rotation.X = CameraRotation.Pitch;
    Rotation.Y = CameraRotation.Yaw;
    Rotation.Z = CameraRotation.Roll;

    FMatrix tempMatrix = ToMatrixB(Rotation);


    FVector vAxisX(tempMatrix.XPlane.X, tempMatrix.XPlane.Y, tempMatrix.XPlane.Z);
    FVector vAxisY(tempMatrix.YPlane.X, tempMatrix.YPlane.Y, tempMatrix.YPlane.Z);
    FVector vAxisZ(tempMatrix.ZPlane.X, tempMatrix.ZPlane.Y, tempMatrix.ZPlane.Z);

    FVector vDelta = Location - CameraLocation;
    FVector vTransformed = FVector(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

    if (vTransformed.Z < .1f)
    {
        vTransformed.Z = .1f;
    }

    if (vTransformed.Z <= 0)
        return false;

    Screen.X = ScreenCenterX + vTransformed.X * (ScreenCenterX / tanf(FOV * (float)M_PI / 360.f)) / vTransformed.Z;
    Screen.Y = ScreenCenterY - vTransformed.Y * (ScreenCenterX / tanf(FOV * (float)M_PI / 360.f)) / vTransformed.Z;

    if (Screen.X < 0 || Screen.X > ScreenCenterX * 2 ||
        Screen.Y < 0 || Screen.Y > ScreenCenterY * 2)
        return false;

    if (isnan(Screen.X) || isnan(Screen.Y))
        return false;

    return true;
}