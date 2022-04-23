#pragma once
#include "RHI/RHIResources.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIVertexBuffer

class CNullRHIVertexBuffer final : public CRHIVertexBuffer
{
public:

    explicit CNullRHIVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
        : CRHIVertexBuffer(Initializer)
    { }

    virtual void* GetRHIHandle()     const override final { return nullptr; }
    virtual void* GetRHIBaseBuffer() const override final { return (void*)this; }

    virtual void   SetName(const String& InName) override final { }
    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIIndexBuffer

class CNullRHIIndexBuffer final : public CRHIIndexBuffer
{
public:

    explicit CNullRHIIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
        : CRHIIndexBuffer(Initializer)
    { }

    virtual void* GetRHIHandle()     const override final { return nullptr; }
    virtual void* GetRHIBaseBuffer() const override final { return (void*)this; }

    virtual void   SetName(const String& InName) override final { }
    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIGenericBuffer

class CNullRHIGenericBuffer final : public CRHIGenericBuffer
{
public:

    explicit CNullRHIGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
        : CRHIGenericBuffer(Initializer)
    { }

    virtual void* GetRHIHandle()     const override final { return nullptr; }
    virtual void* GetRHIBaseBuffer() const override final { return (void*)this; }

    virtual void   SetName(const String& InName) override final { }
    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIConstantBuffer

class CNullRHIConstantBuffer final: public CRHIConstantBuffer
{
public:

    explicit CNullRHIConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
        : CRHIConstantBuffer(Initializer)
    { }

    virtual void* GetRHIHandle()     const override final { return nullptr; }
    virtual void* GetRHIBaseBuffer() const override final { return (void*)this; }

    virtual void   SetName(const String& InName) override final { }
    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHISamplerState

class CNullRHISamplerState final : public CRHISamplerState
{
public:

    explicit CNullRHISamplerState(const CRHISamplerStateInitializer& Initializer)
        : CRHISamplerState(Initializer)
    { }

    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRayTracingGeometry

class CNullRHIRayTracingGeometry final : public CRHIRayTracingGeometry
{
public:

    explicit CNullRHIRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer)
        : CRHIRayTracingGeometry(Initializer)
    { }

    virtual void* GetRHIHandle() const override final { return nullptr; }

    virtual void   SetName(const String& InName) override final { }
    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRayTracingScene

class CNullRHIRayTracingScene final : public CRHIRayTracingScene
{
public:

    explicit CNullRHIRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer)
        : CRHIRayTracingScene(Initializer)
        , ShaderResourceView(dbg_new CNullRHIShaderResourceView())
    { }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return ShaderResourceView.Get(); }
    virtual CRHIDescriptorHandle    GetBindlessHandle()     const override final { return CRHIDescriptorHandle(); }

    virtual void* GetRHIHandle() const override final { return nullptr; }

    virtual void   SetName(const String& InName) override final { }
    virtual String GetName() const override final { return ""; }

private:
    TSharedRef<CNullRHIShaderResourceView> ShaderResourceView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHITimeQuery

class CNullRHITimeQuery final : public CRHITimeQuery
{
public:

    CNullRHITimeQuery()
        : CRHITimeQuery()
    { }

    virtual uint32 GetNumTimestamps() const override final { }

    virtual void GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const override final { }

    virtual uint64 GetFrequency() const override final { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIViewport

class CNullRHIViewport final : public CRHIViewport
{
public:

    explicit CNullRHIViewport(const CRHIViewportInitializer& Initializer)
        : CRHIViewport(Initializer)
        , BackBuffer()
    { }

    virtual bool Resize(uint32 Width, uint32 Height) override final { }

    virtual CRHITexture2D* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    TSharedRef<CNullRHITexture2D> BackBuffer;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif