#include "DirectionalLight.h"

#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12DescriptorHeap.h"

#include "Rendering/Renderer.h"

DirectionalLight::DirectionalLight()
	: Light()
	, Direction(0.0f, -1.0f, 0.0f)
{
	CORE_OBJECT_INIT();
}

DirectionalLight::~DirectionalLight()
{
}

bool DirectionalLight::Initialize(D3D12Device* Device)
{
	// Create descriptortable
	DescriptorTable = new D3D12DescriptorTable(Device, 1);

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
		LightBuffer->SetConstantBufferView(MakeShared<D3D12ConstantBufferView>(Device, LightBuffer->GetResource(), &CBVDesc));

		TSharedPtr<D3D12ImmediateCommandList> CommandList = Renderer::Get()->GetImmediateCommandList();
		CommandList->TransitionBarrier(LightBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		CommandList->Flush();

		BuildBuffer(CommandList.Get());
		CommandList->Flush();

		// Copy descriptors
		DescriptorTable->SetConstantBufferView(LightBuffer->GetConstantBufferView().Get(), 0);
		DescriptorTable->CopyDescriptors();

		return true;
	}
	else
	{
		return false;
	}
}

void DirectionalLight::BuildBuffer(D3D12CommandList* CommandList)
{
	DirectionalLightProperties Properties;
	Properties.Color		= XMFLOAT3(Color.x * Intensity, Color.y * Intensity, Color.z * Intensity);
	Properties.Direction	= Direction;
	CommandList->UploadBufferData(LightBuffer, 0, &Properties, sizeof(DirectionalLightProperties));

	LightBufferIsDirty = false;
}

void DirectionalLight::SetDirection(const XMFLOAT3& InDirection)
{
	XMVECTOR XmDir = XMVectorSet(InDirection.x, InDirection.y, InDirection.z, 0.0f);
	XmDir = XMVector3Normalize(XmDir);
	XMStoreFloat3(&Direction, XmDir);

	LightBufferIsDirty = true;
}

void DirectionalLight::SetDirection(Float32 X, Float32 Y, Float32 Z)
{
	XMVECTOR XmDir = XMVectorSet(X, Y, Z, 0.0f);
	XmDir = XMVector3Normalize(XmDir);
	XMStoreFloat3(&Direction, XmDir);

	LightBufferIsDirty = true;
}
