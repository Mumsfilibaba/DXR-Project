#include "D3D12ShaderCompiler.h"

#include "Types.h"

#include "Windows/Windows.h"
#include "Windows/Windows.inl"

/*
* GetTargetProfile
*/

static LPCWSTR GetTargetProfile(EShaderStage ShaderStage, EShaderModel ShaderModel)
{
	if (ShaderStage == EShaderStage::ShaderStage_Vertex)
	{
		if (ShaderModel == EShaderModel::ShaderModel_5_0)
		{
			return L"vs_5_0";
		}
		else if (ShaderModel == EShaderModel::ShaderModel_5_1)
		{
			return L"vs_5_1";
		}
		else if (ShaderModel == EShaderModel::ShaderModel_6_0)
		{
			return L"vs_6_0";
		}
	}
	else if (ShaderStage == EShaderStage::ShaderStage_Pixel)
	{
		if (ShaderModel == EShaderModel::ShaderModel_5_0)
		{
			return L"ps_5_0";
		}
		else if (ShaderModel == EShaderModel::ShaderModel_5_1)
		{
			return L"ps_5_1";
		}
		else if (ShaderModel == EShaderModel::ShaderModel_6_0)
		{
			return L"ps_6_0";
		}
	}
	else if (ShaderStage == EShaderStage::ShaderStage_Compute)
	{
		if (ShaderModel == EShaderModel::ShaderModel_5_0)
		{
			return L"cs_5_0";
		}
		else if (ShaderModel == EShaderModel::ShaderModel_5_1)
		{
			return L"cs_5_1";
		}
		else if (ShaderModel == EShaderModel::ShaderModel_6_0)
		{
			return L"cs_6_0";
		}
	}

	return L"";
}

/*
* D3D12ShaderCompiler
*/

D3D12ShaderCompiler::D3D12ShaderCompiler()
	: IShaderCompiler()
	, DxCompiler(nullptr)
	, DxLibrary(nullptr)
	, DxLinker(nullptr)
	, DxIncludeHandler(nullptr)
	, DxCompilerDLL()
{
}

D3D12ShaderCompiler::~D3D12ShaderCompiler()
{
	::CloseHandle(DxCompilerDLL);
}

bool D3D12ShaderCompiler::CompileFromFile(
	const std::string& FilePath, 
	const std::string& EntryPoint, 
	const TArray<ShaderDefine>& Defines, 
	EShaderStage ShaderStage, 
	EShaderModel ShaderModel, 
	TArray<Uint8>& Code) const
{
	using namespace Microsoft::WRL;

	// Convert to wide
	std::wstring WideFilePath	= ConvertToWide(FilePath);
	std::wstring WideEntrypoint	= ConvertToWide(EntryPoint);
	LPCWSTR TargetProfile		= GetTargetProfile(ShaderStage, ShaderModel);

	// Create SourceBlob
	ComPtr<IDxcBlobEncoding> SourceBlob;
	HRESULT hResult = DxLibrary->CreateBlobFromFile(WideFilePath.c_str(), nullptr, &SourceBlob);
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create Source Data");
		Debug::DebugBreak();

		return nullptr;
	}

	return InternalCompileFromSource(
		SourceBlob.Get(), 
		WideFilePath.c_str(),
		WideEntrypoint.c_str(), 
		TargetProfile, 
		Defines,
		Code);
}

bool D3D12ShaderCompiler::CompileShader(
	const std::string& ShaderSource, 
	const std::string& EntryPoint, 
	const TArray<ShaderDefine>& Defines, 
	EShaderStage ShaderStage, 
	EShaderModel ShaderModel, 
	TArray<Uint8>& Code) const
{
	using namespace Microsoft::WRL;

	std::wstring WideEntrypoint	= ConvertToWide(EntryPoint);
	LPCWSTR TargetProfile		= GetTargetProfile(ShaderStage, ShaderModel);

	// Create SourceBlob
	ComPtr<IDxcBlobEncoding> SourceBlob;
	HRESULT hResult = DxLibrary->CreateBlobWithEncodingOnHeapCopy(ShaderSource.c_str(), sizeof(Char) * static_cast<Uint32>(ShaderSource.size()), CP_UTF8, &SourceBlob);
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12ShaderCompiler]: FAILED to create Source Data");
		Debug::DebugBreak();

		return nullptr;
	}

	return InternalCompileFromSource(
		SourceBlob.Get(), 
		nullptr, 
		WideEntrypoint.c_str(), 
		TargetProfile,
		Defines, 
		Code);
}

bool D3D12ShaderCompiler::Initialize()
{
	DxCompilerDLL = ::LoadLibrary("dxcompiler.dll");
	if (!DxCompilerDLL)
	{
		::MessageBox(0, "FAILED to load dxcompiler.dll", "ERROR", MB_OK);
		return false;
	}

	DxcCreateInstanceProc DxcCreateInstance_ = GetTypedProcAddress<DxcCreateInstanceProc>(DxCompilerDLL, "DxcCreateInstance");
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

bool D3D12ShaderCompiler::InternalCompileFromSource(
	IDxcBlob* SourceBlob, 
	LPCWSTR FilePath, 
	LPCWSTR Entrypoint, 
	LPCWSTR TargetProfile, 
	const TArray<ShaderDefine>& Defines, 
	TArray<Uint8>& Code) const
{
	using namespace Microsoft::WRL;

	LPCWSTR Args[] =
	{
		L"-O3",	// Optimization level 3
	};

	// Convert defines
	TArray<DxcDefine>		DxDefines(Defines.Size());
	TArray<std::wstring>	StrBuff(Defines.Size() * 2);
	for (const ShaderDefine& Define : Defines)
	{
		const std::wstring& WideDefine	= StrBuff.EmplaceBack(ConvertToWide(Define.Define));
		const std::wstring& WideValue	= StrBuff.EmplaceBack(ConvertToWide(Define.Define));
		DxDefines.PushBack({ WideDefine.c_str(), WideValue.c_str() });
	}

	// Compile
	ComPtr<IDxcOperationResult> Result;
	HRESULT hResult = DxCompiler->Compile(
		SourceBlob, 
		FilePath, 
		Entrypoint, 
		TargetProfile, 
		Args, 1, 
		DxDefines.Data(),
		DxDefines.Size(),
		DxIncludeHandler.Get(), 
		&Result);
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12ShaderCompiler]: FAILED to Compile");
		Debug::DebugBreak();

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
				// Copy data to resulting bytecode
				const Uint32 BlobSize = CompiledBlob->GetBufferSize();
				Code.Resize(BlobSize);
				Memory::Memcpy(Code.Data(), CompiledBlob->GetBufferPointer(), BlobSize);

				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			LOG_ERROR("[D3D12ShaderCompiler]: FAILED to compile with the following error:");
			LOG_ERROR(reinterpret_cast<LPCSTR>(PrintBlob8->GetBufferPointer()));

			return false;
		}
	}
	else
	{
		return false;
	}
}