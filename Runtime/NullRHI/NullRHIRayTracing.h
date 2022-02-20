#pragma once
#include "RHI/RHIRayTracing.h"

#include "NullRHIResourceView.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRayTracingGeometry

class CNullRHIRayTracingGeometry : public CRHIRayTracingGeometry
{
public:
    CNullRHIRayTracingGeometry(uint32 InFlags)
        : CRHIRayTracingGeometry(InFlags)
    {
    }

    ~CNullRHIRayTracingGeometry() = default;

    virtual void SetName(const String& InName) override
    {
        CRHIObject::SetName(InName);
    }

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRayTracingScene

class CNullRHIRayTracingScene : public CRHIRayTracingScene
{
public:
    CNullRHIRayTracingScene(uint32 InFlags)
        : CRHIRayTracingScene(InFlags)
        , View(dbg_new CNullRHIShaderResourceView())
    {
    }

    ~CNullRHIRayTracingScene() = default;

    virtual void SetName(const String& InName) override final
    {
        CRHIObject::SetName(InName);
    }

    virtual bool IsValid() const override final
    {
        return true;
    }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final
    {
        return View.Get();
    }

private:
    TSharedRef<CNullRHIShaderResourceView> View;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
