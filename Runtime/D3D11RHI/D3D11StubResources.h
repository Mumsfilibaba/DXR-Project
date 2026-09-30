#pragma once
#include "RHI/RHI.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"
#include "D3D11RHI/D3D11Core.h"

// ------------------------------------------------------------------------------------------------
// Placeholder objects that let the engine boot on D3D11RHI before the real resource types exist.
// Each one is removed once its D3D11 implementation lands.
// ------------------------------------------------------------------------------------------------

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FD3D11StubQueryRHI : public FRHIQuery
{
    FD3D11StubQueryRHI(EQueryType InQueryType)
        : FRHIQuery(InQueryType)
    {
    }

    virtual ~FD3D11StubQueryRHI();
};

class FD3D11StubFenceRHI : public FRHIFence
{
public:
    virtual void* GetRHINativeFence() const override final { return nullptr; }
    virtual bool  IsSignaled() const override final { return true; }
    virtual bool  Wait(uint64 TimeoutNs) const override final { return true; }

    virtual void SetDebugName(const String& InName) override final { DebugName = InName; }
    virtual void GetDebugName(String& OutDebugName) const override final { OutDebugName = DebugName; }

private:
    String DebugName;
};

class FD3D11StubInputLayoutRHI : public FRHIInputLayout
{
public:
    FD3D11StubInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
        : FRHIInputLayout()
        , InputElements(InInputElements)
    {
    }

    virtual ~FD3D11StubInputLayoutRHI();

    virtual void* GetRHINativeState() const override final { return nullptr; }

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const override final
    {
        return (Index < static_cast<uint32>(InputElements.Size())) ? &InputElements[Index] : nullptr;
    }

    virtual uint32 GetNumInputElementDescs() const override final { return InputElements.Size(); }

private:
    TArray<FRHIInputElementDesc> InputElements;
};

class FD3D11StubDepthStencilStateRHI : public FRHIDepthStencilState
{
public:
    FD3D11StubDepthStencilStateRHI(const FRHIDepthStencilStateDesc& InDesc)
        : FRHIDepthStencilState(InDesc)
    {
    }

    virtual ~FD3D11StubDepthStencilStateRHI();

    virtual void* GetRHINativeState() const override final { return nullptr; }
};

class FD3D11StubRasterizerStateRHI : public FRHIRasterizerState
{
public:
    FD3D11StubRasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc)
        : FRHIRasterizerState(InDesc)
    {
    }

    virtual ~FD3D11StubRasterizerStateRHI();

    virtual void* GetRHINativeState() const override final { return nullptr; }
};

class FD3D11StubBlendStateRHI : public FRHIBlendState
{
public:
    FD3D11StubBlendStateRHI(const FRHIBlendStateDesc& InDesc)
        : FRHIBlendState(InDesc)
    {
    }

    virtual ~FD3D11StubBlendStateRHI();

    virtual void* GetRHINativeState() const override final { return nullptr; }
};

template<typename BasePipelineType>
class TD3D11StubPipelineStateRHI final : public BasePipelineType
{
public:
    virtual void* GetRHINativeState() const override final { return nullptr; }

    virtual void SetDebugName(const String& InDebugName) override final { DebugName = InDebugName; }
    virtual void GetDebugName(String& OutDebugName) const override final { OutDebugName = DebugName; }

private:
    String DebugName;
};

template<typename BaseShaderType>
class TD3D11StubShaderRHI final : public BaseShaderType
{
public:
    virtual void* GetRHINativeHandle()  override final { return nullptr; }
    virtual void* GetRHIBaseInterface() override final { return this; }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
