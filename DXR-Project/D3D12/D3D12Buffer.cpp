#include "D3D12Buffer.h"

/*
* D3D12VertexBuffer
*/

D3D12VertexBuffer::D3D12VertexBuffer(D3D12Device* InDevice, Uint32 InSizeInBytes, Uint32 InStride, Uint32 InUsage)
	: VertexBuffer(InSizeInBytes, InStride, InUsage)
	, D3D12Resource(InDevice)
	, VertexBufferView()
{
}

/*
* D3D12IndexBuffer
*/

D3D12IndexBuffer::D3D12IndexBuffer(D3D12Device* InDevice, Uint32 InSizeInBytes, EIndexFormat InIndexFormat, Uint32 InUsage)
	: IndexBuffer(InSizeInBytes, InIndexFormat, InUsage)
	, D3D12Resource(InDevice)
	, IndexBufferView()
{
}

/*
* D3D12ConstantBuffer
*/

D3D12ConstantBuffer::D3D12ConstantBuffer(D3D12Device* InDevice, Uint32 InSizeInBytes, Uint32 InUsage)
	: ConstantBuffer(InSizeInBytes, InUsage)
	, D3D12Resource(InDevice)
	, View(nullptr)
{
}

/*
* D3D12StructuredBuffer
*/

D3D12StructuredBuffer::D3D12StructuredBuffer(D3D12Device* InDevice, Uint32 InSizeInBytes, Uint32 InStride, Uint32 InUsage)
	: StructuredBuffer(InSizeInBytes, InStride, InUsage)
	, D3D12Resource(InDevice)
{
}
