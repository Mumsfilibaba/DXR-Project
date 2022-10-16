#pragma once
#include "Vector3.h"

struct FAABB
{
    FAABB()
        : Top()
        , Bottom()
    { }

    FAABB(const FVector3& InTop, const FVector3& InBottom)
        : Top(InTop)
        , Bottom(InBottom)
    { }

   /**
    * @brief  - Returns the center position of the bounding box
    * @return - Returns the center position of the box
    */
    FORCEINLINE FVector3 GetCenter() const
    {
        return (Bottom + Top) * 0.5f;
    }

    /**
     * @brief  - Return the width of the bounding box
     * @return - Return the width of the bounding box
     */
    FORCEINLINE float GetWidth() const
    {
        return Top.x - Bottom.x;
    }

    /**
     * @brief  - Return the height of the bounding box
     * @return - Return the height of the bounding box
     */
    FORCEINLINE float GetHeight() const
    {
        return Top.y - Bottom.y;
    }

    /**
     * @brief  - Return the depth of the bounding box
     * @return - Return the depth of the bounding box
     */
    FORCEINLINE float GetDepth() const
    {
        return Top.z - Bottom.z;
    }

    FVector3 Top;
    FVector3 Bottom;
};

MARK_AS_REALLOCATABLE(FAABB);