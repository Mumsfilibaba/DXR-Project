#pragma once
#include "RenderingCore.h"

#include "Core/RefCountedObject.h"

class Resource : public RefCountedObject
{
public:
    Resource() = default;
    ~Resource() = default;

    virtual void SetName(const std::string& InName)
    {
        Name = InName;
    }

    virtual void* GetNativeResource() const { return nullptr; }

    virtual Bool IsValid() const { return false; }

    const std::string& GetName() const { return Name; }

private:
    std::string Name;
};