#pragma once
#include "Vector3.h"

struct SAABB
{
    FORCEINLINE CVector3 GetCenter() const
    {
        return (Bottom + Top) * 0.5f;
    }

    FORCEINLINE float GetWidth() const
    {
        return Top.x - Bottom.x;
    }

    FORCEINLINE float GetHeight() const
    {
        return Top.y - Bottom.y;
    }

    FORCEINLINE float GetDepth() const
    {
        return Top.z - Bottom.z;
    }

    CVector3 Top;
    CVector3 Bottom;
};