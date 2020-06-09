#include "D3D12ShaderCompiler.h"

#include "Types.h"

#include <codecvt>
#include <locale>

#pragma comment(lib, "dxcompiler.lib")

/*
* Members
*/

D3D12ShaderCompiler::D3D12ShaderCompiler()
{
}

D3D12ShaderCompiler::~D3D12ShaderCompiler()
{
}

bool D3D12ShaderCompiler::Init()
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

IDxcBlob* D3D12ShaderCompiler::CompileFromFile(const std::string& Filepath, const std::string& Entrypoint, const std::string& TargetProfile)
{
	using namespace Microsoft::WRL;

	// Convert to wide
	std::wstring WideFilePath		= std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(Filepath.c_str());
	std::wstring WideEntrypoint		= std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(Entrypoint.c_str());
	std::wstring WideTargetProfile	= std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(TargetProfile.c_str());

	// Create SourceBlob
	ComPtr<IDxcBlobEncoding> SourceBlob;
	HRESULT hResult = DxLibrary->CreateBlobFromFile(WideFilePath.c_str(), nullptr, &SourceBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12ShaderCompiler]: Failed to create Source Data\n");
		return nullptr;
	}

	// Compile
	ComPtr<IDxcOperationResult> Result;
	hResult = DxCompiler->Compile(SourceBlob.Get(), WideFilePath.c_str(), WideEntrypoint.c_str(), WideTargetProfile.c_str(), nullptr, 0, nullptr, 0, DxIncludeHandler.Get(), &Result);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12ShaderCompiler]: Failed to Compile\n");
		return nullptr;
	}

	HRESULT hCompileResult;
	if (SUCCEEDED(Result->GetStatus(&hCompileResult)))
	{
		if (SUCCEEDED(hCompileResult))
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
	if (CompilerInstance->Init())
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
