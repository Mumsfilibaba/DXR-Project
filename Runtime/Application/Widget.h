#pragma once
#include "Core/RefCounted.h"

struct FWidget 
    : public FRefCounted
{
    virtual ~FWidget() = default;

    /** 
     * @brief - Update the window 
     */
    virtual void Tick() = 0;

    /**
     * @brief  - Check if the window should be updated this frame
     * @return - Returns true if the window should be updated
     */
    virtual bool ShouldTick() = 0;
};