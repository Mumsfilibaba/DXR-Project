#pragma once
#include "Vector3.h"

struct AABB
{
    CVector3 GetCenter() const
    {
        return (Bottom + Top) * 0.5f;
    }

    float GetWidth() const
    {
        return Top.x - Bottom.x;
    }

    float GetHeight() const
    {
        return Top.y - Bottom.y;
    }

    float GetDepth() const
    {
        return Top.z - Bottom.z;
    }

    CVector3 Top;
    CVector3 Bottom;
};