#pragma once
#include "Vector3.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Axis-Aligned Bounding Box

struct SAABB
{
    SAABB()
        : Top()
        , Bottom()
    { }

    SAABB(const CVector3& InTop, const CVector3& InBottom)
        : Top(InTop)
        , Bottom(InBottom)
    { }

   /**
    * Returns the center position of the bounding box
    * 
    * @return: Returns the center position of the box
    */
    FORCEINLINE CVector3 GetCenter() const
    {
        return (Bottom + Top) * 0.5f;
    }

    /**
    * Return the width of the bounding box
    * 
    * @return: Return the width of the bounding box
    */
    FORCEINLINE float GetWidth() const
    {
        return Top.x - Bottom.x;
    }

    /**
    * Return the height of the bounding box
    *
    * @return: Return the height of the bounding box
    */
    FORCEINLINE float GetHeight() const
    {
        return Top.y - Bottom.y;
    }

    /**
    * Return the depth of the bounding box
    *
    * @return: Return the depth of the bounding box
    */
    FORCEINLINE float GetDepth() const
    {
        return Top.z - Bottom.z;
    }

    CVector3 Top;
    CVector3 Bottom;
};