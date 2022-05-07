#pragma once
#include "D3D12Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ID3D12RefCounted

class ID3D12RefCounted
{
protected:

    ID3D12RefCounted() = default;
    virtual ~ID3D12RefCounted() = default;

public:

    virtual int32 AddRef() = 0;

    virtual int32 Release() = 0;
};