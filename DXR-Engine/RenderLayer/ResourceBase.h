#pragma once
#include "RenderingCore.h"

#include "Core/RefCounted.h"

class Resource : public CRefCounted
{
public:
    virtual void* GetNativeResource() const
    {
        return nullptr;
    }

    virtual bool IsValid() const
    {
        return false;
    }

    virtual void SetName( const std::string& InName )
    {
        Name = InName;
    }

    const std::string& GetName() const
    {
        return Name;
    }

private:
    std::string Name;
};