#include "D3D12Buffer.h"

/*
* D3D12VertexBuffer
*/

D3D12VertexBuffer::D3D12VertexBuffer(D3D12Device* InDevice, Uint32 SizeInBytes, Uint32 Stride, Uint32 Usage)
	: VertexBuffer(SizeInBytes, Stride, Usage)
	, D3D12Resource(InDevice)
	, VertexBufferView()
{
}

/*
* D3D12IndexBuffer
*/

D3D12IndexBuffer::D3D12IndexBuffer(D3D12Device* InDevice, Uint32 SizeInBytes, EIndexFormat IndexFormat, Uint32 Usage)
	: IndexBuffer(SizeInBytes, IndexFormat, Usage)
	, D3D12Resource(InDevice)
	, IndexBufferView()
{
}

/*
* D3D12ConstantBuffer
*/

D3D12ConstantBuffer::D3D12ConstantBuffer(D3D12Device* InDevice, Uint32 SizeInBytes, Uint32 Usage)
	: ConstantBuffer(SizeInBytes, Usage)
	, D3D12Resource(InDevice)
	, View(nullptr)
{
}

/*
* D3D12StructuredBuffer
*/

D3D12StructuredBuffer::D3D12StructuredBuffer(D3D12Device* InDevice, Uint32 SizeInBytes, Uint32 Stride, Uint32 Usage)
	: StructuredBuffer(SizeInBytes, Stride, Usage)
	, D3D12Resource(InDevice)
{
}
