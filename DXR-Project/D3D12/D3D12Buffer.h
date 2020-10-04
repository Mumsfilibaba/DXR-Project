#pragma once
#include "RenderingCore/Buffer.h"

#include "D3D12Resource.h"
#include "D3D12Views.h"

/*
* D3D12Buffer
*/

class D3D12Buffer : public Buffer, public D3D12Resource
{
public:
	D3D12Buffer(D3D12Device* InDevice);
	~D3D12Buffer();

	// Inherited from Buffer
	virtual bool Initialize(const BufferInitializer& InInitializer) override;

	virtual void* Map() override;
	virtual void Unmap() override;

	virtual void SetName(const std::string& InName) override;

	virtual VoidPtr GetNativeResource() const
	{
		return reinterpret_cast<VoidPtr>(NativeResource.Get());
	}

	FORCEINLINE void SetConstantBufferView(TSharedPtr<D3D12ConstantBufferView> InConstantBufferView)
	{
		ConstantBufferView = InConstantBufferView;
	}

	FORCEINLINE TSharedPtr<D3D12ConstantBufferView> GetConstantBufferView() const
	{
		return ConstantBufferView;
	}

private:
	TSharedPtr<D3D12ConstantBufferView> ConstantBufferView;
};