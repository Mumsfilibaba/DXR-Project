#include "D3D12ShaderCompiler.h"

#include "Types.h"

Microsoft::WRL::ComPtr<IDxcCompiler>		D3D12ShaderCompiler::DxCompiler;
Microsoft::WRL::ComPtr<IDxcLibrary>			D3D12ShaderCompiler::DxLibrary;
Microsoft::WRL::ComPtr<IDxcLinker>			D3D12ShaderCompiler::DxLinker;
Microsoft::WRL::ComPtr<IDxcIncludeHandler>	D3D12ShaderCompiler::DxIncludeHandler;
HMODULE										D3D12ShaderCompiler::DxCompilerDLL = 0;

bool D3D12ShaderCompiler::Initialize()
{
	if (!DxCompilerDLL)
	{
		DxCompilerDLL = ::LoadLibrary("dxcompiler.dll");
		if (!DxCompilerDLL)
		{
			::MessageBox(0, "FAILED to load dxcompiler.dll", "ERROR", MB_OK);
			return false;
		}
	}

	DxcCreateInstanceProc DxcCreateInstance_ = reinterpret_cast<DxcCreateInstanceProc>(::GetProcAddress(DxCompilerDLL, "DxcCreateInstance"));
	if (!DxcCreateInstance_)
	{
		LOG_ERROR("[D3D12ShaderCompiler]: FAILED to load DxcCreateInstance");
		return false;
	}
	
	HRESULT hr = DxcCreateInstance_(CLSID_DxcCompiler, IID_PPV_ARGS(&DxCompiler));
	if (SUCCEEDED(hr))
	{
		hr = DxcCreateInstance_(CLSID_DxcLibrary, IID_PPV_ARGS(&DxLibrary));
		if (SUCCEEDED(hr))
		{
			hr = DxLibrary->CreateIncludeHandler(&DxIncludeHandler);
			if (SUCCEEDED(hr))
			{
				hr = DxcCreateInstance_(CLSID_DxcLinker, IID_PPV_ARGS(&DxLinker));
				if (SUCCEEDED(hr))
				{
					return true;
				}
				else
				{
					LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxLinker");
					return false;
				}
			}
			else
			{
				LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxIncludeHandler");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxLibrary");
			return false;
		}
	}
	else
	{
		LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create DxCompiler");
		return false;
	}
}

IDxcBlob* D3D12ShaderCompiler::CompileFromFile(const std::string& Filepath, const std::string& Entrypoint, const std::string& TargetProfile)
{
	using namespace Microsoft::WRL;

	if (DxCompilerDLL == 0)
	{
		if (!Initialize())
		{
			return nullptr;
		}
	}

	// Convert to wide
	std::wstring WideFilePath		= ConvertToWide(Filepath);
	std::wstring WideEntrypoint		= ConvertToWide(Entrypoint);
	std::wstring WideTargetProfile	= ConvertToWide(TargetProfile);

	// Create SourceBlob
	ComPtr<IDxcBlobEncoding> SourceBlob;
	HRESULT hResult = DxLibrary->CreateBlobFromFile(WideFilePath.c_str(), nullptr, &SourceBlob);
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create Source Data");
		return nullptr;
	}

	return InternalCompileFromSource(SourceBlob.Get(), WideFilePath.c_str(), WideEntrypoint.c_str(), WideTargetProfile.c_str());
}

IDxcBlob* D3D12ShaderCompiler::CompileFromSource(const std::string& Source, const std::string& Entrypoint, const std::string& TargetProfile)
{
	using namespace Microsoft::WRL;

	if (DxCompilerDLL == 0)
	{
		if (!Initialize())
		{
			return nullptr;
		}
	}

	std::wstring WideEntrypoint		= ConvertToWide(Entrypoint);
	std::wstring WideTargetProfile	= ConvertToWide(TargetProfile);

	// Create SourceBlob
	ComPtr<IDxcBlobEncoding> SourceBlob;
	HRESULT hResult = DxLibrary->CreateBlobWithEncodingOnHeapCopy(Source.c_str(), sizeof(Char) * static_cast<Uint32>(Source.size()), CP_UTF8, &SourceBlob);
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create Source Data");
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
		LOG_ERROR("[D3D12ShaderCompiler]: FAILED to Compile");
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
			if (PrintBlob8->GetBufferSize() > 0)
			{
				LOG_INFO("[D3D12ShaderCompiler]: Compiled with the following output:");
				LOG_INFO(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));
			}

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
			LOG_ERROR("[D3D12ShaderCompiler]: FAILED to compile with the following error:");
			LOG_ERROR(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));

			return nullptr;
		}
	}
	else
	{
		return nullptr;
	}
}
