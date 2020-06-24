#include "D3D12ShaderCompiler.h"

#include "Types.h"

#pragma comment(lib, "dxcompiler.lib")

/*
* Members
*/

D3D12ShaderCompiler::D3D12ShaderCompiler()
	: DxCompiler(nullptr)
	, DxLibrary(nullptr)
	, DxLinker(nullptr)
	, DxIncludeHandler(nullptr)
{
}

D3D12ShaderCompiler::~D3D12ShaderCompiler()
{
}

bool D3D12ShaderCompiler::Initialize()
{
	HRESULT hResult = ::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DxCompiler));
	if (SUCCEEDED(hResult))
	{
		hResult = ::DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&DxLibrary));
		if (SUCCEEDED(hResult))
		{
			hResult = DxLibrary->CreateIncludeHandler(&DxIncludeHandler);
			if (SUCCEEDED(hResult))
			{
				hResult = ::DxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(&DxLinker));
				if (SUCCEEDED(hResult))
				{
					return true;
				}
				else
				{
					::OutputDebugString("[D3D12ShaderCompiler]: Failed to create DxLinker\n");
					return false;
				}
			}
			else
			{
				::OutputDebugString("[D3D12ShaderCompiler]: Failed to create DxIncludeHandler\n");
				return false;
			}
		}
		else
		{
			::OutputDebugString("[D3D12ShaderCompiler]: Failed to create DxLibrary\n");
			return false;
		}
	}
	else
	{
		::OutputDebugString("[D3D12ShaderCompiler]: Failed to create DxCompiler\n");
		return false;
	}
}

IDxcBlob* D3D12ShaderCompiler::CompileFromFile(const std::string& InFilepath, const std::string& InEntrypoint, const std::string& InTargetProfile)
{
	using namespace Microsoft::WRL;

	// Convert to wide
	std::wstring WideFilePath		= ConvertToWide(InFilepath);
	std::wstring WideEntrypoint		= ConvertToWide(InEntrypoint);
	std::wstring WideTargetProfile	= ConvertToWide(InTargetProfile);

	// Create SourceBlob
	ComPtr<IDxcBlobEncoding> SourceBlob;
	HRESULT hResult = DxLibrary->CreateBlobFromFile(WideFilePath.c_str(), nullptr, &SourceBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12ShaderCompiler]: Failed to create Source Data\n");
		return nullptr;
	}

	return InternalCompileFromSource(SourceBlob.Get(), WideFilePath.c_str(), WideEntrypoint.c_str(), WideTargetProfile.c_str());
}

IDxcBlob* D3D12ShaderCompiler::CompileFromSource(const std::string& InSource, const std::string& InEntrypoint, const std::string& InTargetProfile)
{
	using namespace Microsoft::WRL;

	std::wstring WideEntrypoint		= ConvertToWide(InEntrypoint);
	std::wstring WideTargetProfile	= ConvertToWide(InTargetProfile);

	// Create SourceBlob
	ComPtr<IDxcBlobEncoding> SourceBlob;
	HRESULT hResult = DxLibrary->CreateBlobWithEncodingOnHeapCopy(InSource.c_str(), sizeof(Char) * static_cast<Uint32>(InSource.size()), CP_UTF8, &SourceBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12ShaderCompiler]: Failed to create Source Data\n");
		return nullptr;
	}

	return InternalCompileFromSource(SourceBlob.Get(), nullptr, WideEntrypoint.c_str(), WideTargetProfile.c_str());
}

IDxcBlob* D3D12ShaderCompiler::InternalCompileFromSource(IDxcBlob* InSourceBlob, LPCWSTR InFilePath, LPCWSTR InEntrypoint, LPCWSTR InTargetProfile)
{
	using namespace Microsoft::WRL;

	// Compile
	ComPtr<IDxcOperationResult> Result;
	HRESULT hResult = DxCompiler->Compile(InSourceBlob, InFilePath, InEntrypoint, InTargetProfile, nullptr, 0, nullptr, 0, DxIncludeHandler.Get(), &Result);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12ShaderCompiler]: Failed to Compile\n");
		return nullptr;
	}

	if (SUCCEEDED(Result->GetStatus(&hResult)))
	{
		if (SUCCEEDED(hResult))
		{
			IDxcBlob* CompiledBlob = nullptr;
			if (SUCCEEDED(Result->GetResult(&CompiledBlob)))
			{
				return CompiledBlob;
			}
			else
			{
				return nullptr;
			}
		}
		else
		{
			ComPtr<IDxcBlobEncoding> PrintBlob;
			if (SUCCEEDED(Result->GetErrorBuffer(&PrintBlob)))
			{
				ComPtr<IDxcBlobEncoding> PrintBlob8;
				DxLibrary->GetBlobAsUtf8(PrintBlob.Get(), &PrintBlob8);

				::OutputDebugString("[D3D12ShaderCompiler]: Failed to compile with the following error:\n");
				::OutputDebugString(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
			}

			return nullptr;
		}
	}
	else
	{
		return nullptr;
	}
}

/*
* Static
*/

std::unique_ptr<D3D12ShaderCompiler> D3D12ShaderCompiler::CompilerInstance = nullptr;

D3D12ShaderCompiler* D3D12ShaderCompiler::Create()
{
	CompilerInstance.reset(new D3D12ShaderCompiler());
	if (CompilerInstance->Initialize())
	{
		return CompilerInstance.get();
	}
	else
	{
		return nullptr;
	}	
}

D3D12ShaderCompiler* D3D12ShaderCompiler::Get()
{
	return CompilerInstance.get();
}
