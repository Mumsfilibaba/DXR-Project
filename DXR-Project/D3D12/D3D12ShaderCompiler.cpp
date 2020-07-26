#include "D3D12ShaderCompiler.h"

#include "Types.h"

struct CompilerData
{
	HMODULE DxCompilerDLL = 0;
};

static CompilerData GlobalCompilerData;

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
	if (!GlobalCompilerData.DxCompilerDLL)
	{
		GlobalCompilerData.DxCompilerDLL = LoadLibrary("dxcompiler.dll");
		if (!GlobalCompilerData.DxCompilerDLL)
		{
			::MessageBox(0, "FAILED to load dxcompiler.dll", "ERROR", MB_OK);
			return false;
		}
	}

	DxcCreateInstanceProc DxcCreateInstance_ = reinterpret_cast<DxcCreateInstanceProc>(::GetProcAddress(GlobalCompilerData.DxCompilerDLL, "DxcCreateInstance"));
	if (!DxcCreateInstance_)
	{
		::OutputDebugString("[D3D12ShaderCompiler]: FAILED to load DxcCreateInstance\n");
		return false;
	}
	
	HRESULT hResult = DxcCreateInstance_(CLSID_DxcCompiler, IID_PPV_ARGS(&DxCompiler));
	if (SUCCEEDED(hResult))
	{
		hResult = DxcCreateInstance_(CLSID_DxcLibrary, IID_PPV_ARGS(&DxLibrary));
		if (SUCCEEDED(hResult))
		{
			hResult = DxLibrary->CreateIncludeHandler(&DxIncludeHandler);
			if (SUCCEEDED(hResult))
			{
				hResult = DxcCreateInstance_(CLSID_DxcLinker, IID_PPV_ARGS(&DxLinker));
				if (SUCCEEDED(hResult))
				{
					return true;
				}
				else
				{
					::OutputDebugString("[D3D12ShaderCompiler]: FAILED to create DxLinker\n");
					return false;
				}
			}
			else
			{
				::OutputDebugString("[D3D12ShaderCompiler]: FAILED to create DxIncludeHandler\n");
				return false;
			}
		}
		else
		{
			::OutputDebugString("[D3D12ShaderCompiler]: FAILED to create DxLibrary\n");
			return false;
		}
	}
	else
	{
		::OutputDebugString("[D3D12ShaderCompiler]: FAILED to create DxCompiler\n");
		return false;
	}
}

IDxcBlob* D3D12ShaderCompiler::CompileFromFile(const std::string& Filepath, const std::string& Entrypoint, const std::string& TargetProfile)
{
	using namespace Microsoft::WRL;

	// Convert to wide
	std::wstring WideFilePath		= ConvertToWide(Filepath);
	std::wstring WideEntrypoint		= ConvertToWide(Entrypoint);
	std::wstring WideTargetProfile	= ConvertToWide(TargetProfile);

	// Create SourceBlob
	ComPtr<IDxcBlobEncoding> SourceBlob;
	HRESULT hResult = DxLibrary->CreateBlobFromFile(WideFilePath.c_str(), nullptr, &SourceBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12ShaderCompiler]: FAILED to create Source Data\n");
		return nullptr;
	}

	return InternalCompileFromSource(SourceBlob.Get(), WideFilePath.c_str(), WideEntrypoint.c_str(), WideTargetProfile.c_str());
}

IDxcBlob* D3D12ShaderCompiler::CompileFromSource(const std::string& Source, const std::string& Entrypoint, const std::string& TargetProfile)
{
	using namespace Microsoft::WRL;

	std::wstring WideEntrypoint		= ConvertToWide(Entrypoint);
	std::wstring WideTargetProfile	= ConvertToWide(TargetProfile);

	// Create SourceBlob
	ComPtr<IDxcBlobEncoding> SourceBlob;
	HRESULT hResult = DxLibrary->CreateBlobWithEncodingOnHeapCopy(Source.c_str(), sizeof(Char) * static_cast<Uint32>(Source.size()), CP_UTF8, &SourceBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12ShaderCompiler]: FAILED to create Source Data\n");
		return nullptr;
	}

	return InternalCompileFromSource(SourceBlob.Get(), nullptr, WideEntrypoint.c_str(), WideTargetProfile.c_str());
}

IDxcBlob* D3D12ShaderCompiler::InternalCompileFromSource(IDxcBlob* SourceBlob, LPCWSTR Filepath, LPCWSTR Entrypoint, LPCWSTR TargetProfile)
{
	using namespace Microsoft::WRL;

	// Compile
	ComPtr<IDxcOperationResult> Result;
	HRESULT hResult = DxCompiler->Compile(SourceBlob, Filepath, Entrypoint, TargetProfile, nullptr, 0, nullptr, 0, DxIncludeHandler.Get(), &Result);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12ShaderCompiler]: FAILED to Compile\n");
		return nullptr;
	}

	if (SUCCEEDED(Result->GetStatus(&hResult)))
	{
		ComPtr<IDxcBlobEncoding> PrintBlob;
		ComPtr<IDxcBlobEncoding> PrintBlob8;
		if (SUCCEEDED(Result->GetErrorBuffer(&PrintBlob)))
		{
			DxLibrary->GetBlobAsUtf8(PrintBlob.Get(), &PrintBlob8);
		}

		if (SUCCEEDED(hResult))
		{
			::OutputDebugString("[D3D12ShaderCompiler]: Compile with the following output:\n");
			::OutputDebugString(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));

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
			::OutputDebugString("[D3D12ShaderCompiler]: FAILED to compile with the following error:\n");
			::OutputDebugString(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));

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

D3D12ShaderCompiler* D3D12ShaderCompiler::Make()
{
	CompilerInstance = std::unique_ptr<D3D12ShaderCompiler>(new D3D12ShaderCompiler());
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
