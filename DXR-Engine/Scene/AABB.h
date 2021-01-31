#pragma once

struct AABB
{
    XMFLOAT3 GetCenter() const
    {
        return XMFLOAT3((Bottom.x + Top.x) * 0.5f, (Bottom.y + Top.y) * 0.5f, (Bottom.z + Top.z) * 0.5f);
    }

    Float GetWidth() const
    {
        return Top.x - Bottom.x;
    }

    Float GetHeight() const
    {
        return Top.y - Bottom.y;
    }

    Float GetDepth() const
    {
        return Top.z - Bottom.z;
    }

    XMFLOAT3 Top;
    XMFLOAT3 Bottom;
};