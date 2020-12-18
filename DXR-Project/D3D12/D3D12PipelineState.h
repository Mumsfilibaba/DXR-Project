#pragma once
#include "RenderingCore/PipelineState.h"

#include "D3D12DeviceChild.h"
#include "D3D12Helpers.h"

/*
* D3D12InputLayoutState
*/

class D3D12InputLayoutState : public InputLayoutState, public D3D12DeviceChild
{
public:
	inline D3D12InputLayoutState(D3D12Device* InDevice, const InputLayoutStateCreateInfo& CreateInfo)
		: InputLayoutState()
		, D3D12DeviceChild(InDevice)
		, SemanticNames()
		, ElementDesc()
		, Desc()
	{
		SemanticNames.Reserve(CreateInfo.Elements.Size());
		for (const InputElement& Element : CreateInfo.Elements)
		{
			D3D12_INPUT_ELEMENT_DESC DxElement;
			DxElement.SemanticName			= SemanticNames.EmplaceBack(Element.Semantic).c_str();
			DxElement.SemanticIndex			= Element.SemanticIndex;
			DxElement.Format				= ConvertFormat(Element.Format);
			DxElement.InputSlot				= Element.InputSlot;
			DxElement.AlignedByteOffset		= Element.ByteOffset;
			DxElement.InputSlotClass		= ConvertInputClassification(Element.InputClassification);
			DxElement.InstanceDataStepRate	= Element.InstanceStepRate;
			ElementDesc.EmplaceBack(DxElement);
		}

		Desc.NumElements		= GetElementCount();
		Desc.pInputElementDescs	= GetElementData();
	}

	~D3D12InputLayoutState() = default;

	FORCEINLINE const D3D12_INPUT_ELEMENT_DESC* GetElementData() const
	{
		return ElementDesc.Data();
	}

	FORCEINLINE UInt32 GetElementCount() const
	{
		return ElementDesc.Size();
	}

	FORCEINLINE const D3D12_INPUT_LAYOUT_DESC& GetDesc() const
	{
		return Desc;
	}

private:
	TArray<std::string> SemanticNames;
	TArray<D3D12_INPUT_ELEMENT_DESC> ElementDesc;
	D3D12_INPUT_LAYOUT_DESC Desc;
};

/*
* D3D12DepthStencilState
*/

class D3D12DepthStencilState : public DepthStencilState, public D3D12DeviceChild
{
public:
	inline D3D12DepthStencilState(D3D12Device* InDevice, const D3D12_DEPTH_STENCIL_DESC& InDesc)
		: DepthStencilState()
		, D3D12DeviceChild(InDevice)
		, Desc(InDesc)
	{
	}

	~D3D12DepthStencilState() = default;

	FORCEINLINE const D3D12_DEPTH_STENCIL_DESC& GetDesc() const
	{
		return Desc;
	}

private:
	D3D12_DEPTH_STENCIL_DESC Desc;
};

/*
* D3D12RasterizerState
*/

class D3D12RasterizerState : public RasterizerState, public D3D12DeviceChild
{
public:
	inline D3D12RasterizerState(D3D12Device* InDevice, const D3D12_RASTERIZER_DESC& InDesc)
		: RasterizerState()
		, D3D12DeviceChild(InDevice)
		, Desc(InDesc)
	{
	}

	~D3D12RasterizerState() = default;

	FORCEINLINE const D3D12_RASTERIZER_DESC& GetDesc() const
	{
		return Desc;
	}

private:
	D3D12_RASTERIZER_DESC Desc;
};

/*
* D3D12BlendState
*/

class D3D12BlendState : public BlendState, public D3D12DeviceChild
{
public:
	inline D3D12BlendState(D3D12Device* InDevice, const D3D12_BLEND_DESC& InDesc)
		: BlendState()
		, D3D12DeviceChild(InDevice)
		, Desc(InDesc)
	{
	}

	~D3D12BlendState() = default;

	FORCEINLINE const D3D12_BLEND_DESC& GetDesc() const
	{
		return Desc;
	}

private:
	D3D12_BLEND_DESC Desc;
};

/*
* D3D12GraphicsPipelineState
*/

class D3D12GraphicsPipelineState : public GraphicsPipelineState, public D3D12DeviceChild
{
	friend class D3D12RenderingAPI;

public:
	inline D3D12GraphicsPipelineState(D3D12Device* InDevice)
		: D3D12DeviceChild(InDevice)
		, PipelineState(nullptr)
	{
	}

	~D3D12GraphicsPipelineState() = default;

	FORCEINLINE virtual void SetName(const std::string& Name) override final
	{
		std::wstring WideName = ConvertToWide(Name);
		PipelineState->SetName(WideName.c_str());
	}

	FORCEINLINE ID3D12PipelineState* GetPipeline() const
	{
		return PipelineState.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
};

/*
* D3D12ComputePipelineState
*/

class D3D12ComputePipelineState : public ComputePipelineState, public D3D12DeviceChild
{
	friend class D3D12RenderingAPI;

public:
	inline D3D12ComputePipelineState(D3D12Device* InDevice)
		: D3D12DeviceChild(InDevice)
		, PipelineState(nullptr)
	{
	}

	~D3D12ComputePipelineState() = default;

	FORCEINLINE virtual void SetName(const std::string& Name) override final
	{
		std::wstring WideName = ConvertToWide(Name);
		PipelineState->SetName(WideName.c_str());
	}

	FORCEINLINE ID3D12PipelineState* GetPipeline() const
	{
		return PipelineState.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
};