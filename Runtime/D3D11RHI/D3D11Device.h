#pragma once
#include "Core/Containers/String.h"
#include "RHI/RHITypes.h"
#include "D3D11RHI/D3D11Core.h"

class FD3D11Adapter
{
public:
    FD3D11Adapter();
    ~FD3D11Adapter();

    bool Initialize();

    bool IsDebugLayerEnabled() const { return bEnableDebugLayer; }
    bool IsTearingSupported()  const { return bAllowTearing; }

    String GetDescription() const { return WideToChar(WStringView(AdapterDesc.Description)); }

    FORCEINLINE uint32 GetAdapterIndex() const
    {
        return AdapterIndex;
    }

    FORCEINLINE IDXGIAdapter1* GetDXGIAdapter() const
    {
        return Adapter.Get();
    }

    FORCEINLINE IDXGIAdapter3* GetDXGIAdapter3() const
    {
        return Adapter3.Get();
    }

    FORCEINLINE IDXGIFactory2* GetDXGIFactory() const
    {
        return Factory.Get();
    }

    FORCEINLINE IDXGIFactory5* GetDXGIFactory5() const { return Factory5.Get(); }
    FORCEINLINE IDXGIFactory6* GetDXGIFactory6() const { return Factory6.Get(); }

private:
    TComPtr<IDXGIAdapter1> Adapter;
    TComPtr<IDXGIAdapter3> Adapter3;
    TComPtr<IDXGIFactory2> Factory;
    TComPtr<IDXGIFactory5> Factory5;
    TComPtr<IDXGIFactory6> Factory6;

    DXGI_ADAPTER_DESC1 AdapterDesc;
    uint32             AdapterIndex;
    bool               bAllowTearing     : 1;
    bool               bEnableDebugLayer : 1;
};

class FD3D11Device
{
public:
    FD3D11Device(FD3D11Adapter* InAdapter);
    ~FD3D11Device();

    bool Initialize();

    bool SupportsSwapChainFormat(DXGI_FORMAT DXGIFormat, ESwapChainUsageFlags Usage) const;
    bool QueryMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount, uint32& OutQuality) const;

    void FlushDebugMessages();

    void CheckDeviceRemoved(HRESULT Result, const CHAR* Operation) const;

    FORCEINLINE FD3D11Adapter*        GetAdapter()       const { return Adapter; }
    FORCEINLINE ID3D11Device*         GetD3D11Device()   const { return D3D11Device.Get(); }
    FORCEINLINE ID3D11Device1*        GetD3D11Device1()  const { return D3D11Device1.Get(); }
    FORCEINLINE ID3D11Device5*        GetD3D11Device5()  const { return D3D11Device5.Get(); }
    FORCEINLINE ID3D11DeviceContext*  GetD3D11Context()  const { return D3D11Context.Get(); }
    FORCEINLINE ID3D11DeviceContext1* GetD3D11Context1() const { return D3D11Context1.Get(); }
    FORCEINLINE ID3D11DeviceContext4* GetD3D11Context4() const { return D3D11Context4.Get(); }
    FORCEINLINE D3D_FEATURE_LEVEL     GetFeatureLevel()  const { return FeatureLevel; }

private:
    bool CreateDevice();
    void SetupDebugMessages();

    FD3D11Adapter* const          Adapter;
    TComPtr<ID3D11Device>         D3D11Device;
    TComPtr<ID3D11Device1>        D3D11Device1;
    TComPtr<ID3D11Device5>        D3D11Device5;
    TComPtr<ID3D11DeviceContext>  D3D11Context;
    TComPtr<ID3D11DeviceContext1> D3D11Context1;
    TComPtr<ID3D11DeviceContext4> D3D11Context4;
    TComPtr<ID3D11InfoQueue>      InfoQueue;
    D3D_FEATURE_LEVEL             FeatureLevel;
};
