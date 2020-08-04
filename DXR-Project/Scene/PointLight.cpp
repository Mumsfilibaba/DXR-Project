#include "PointLight.h"

#include "D3D12/D3D12Buffer.h"

#include "Rendering/Renderer.h"

PointLight::PointLight()
	: Position(0.0f, 0.0f, 0.0f)
{
}

PointLight::~PointLight()
{
}

bool PointLight::Initialize(D3D12Device* Device)
{
	// Create materialbuffer
	BufferProperties MaterialBufferProps = { };
	MaterialBufferProps.Name		= "LightBuffer";
	MaterialBufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
	MaterialBufferProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;
	MaterialBufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;
	MaterialBufferProps.SizeInBytes	= 256; // Must atleast be a multiple of 256

	LightBuffer = new D3D12Buffer(Device);
	if (LightBuffer->Initialize(MaterialBufferProps))
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc = { };
		CBVDesc.BufferLocation	= LightBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes		= LightBuffer->GetSizeInBytes();
		LightBuffer->SetConstantBufferView(std::make_shared<D3D12ConstantBufferView>(Device, LightBuffer->GetResource(), &CBVDesc));

		std::shared_ptr<D3D12ImmediateCommandList> CommandList = Renderer::Get()->GetImmediateCommandList();
		CommandList->TransitionBarrier(LightBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		CommandList->Flush();

		//BuildBuffer(CommandList.get());
		//CommandList->Flush();
		
		return true;
	}
	else
	{
		return false;
	}
}

void PointLight::SetPosition(const XMFLOAT3& InPosition)
{
	Position = InPosition;
}

void PointLight::SetPosition(Float32 X, Float32 Y, Float32 Z)
{
	Position = XMFLOAT3(X, Y, Z);
}
