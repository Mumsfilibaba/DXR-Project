#pragma once
#if PLATFORM_WINDOWS
    #include <Unknwn.h>
#endif
#include <dxc/dxcapi.h>

#include "Core/Containers/ComPtr.h"
#include "ShaderCompiler/ShaderCompilerBackend.h"

class FDXCShaderCompiler final : public FShaderCompilerBackend
{
public:
    FDXCShaderCompiler();
    virtual ~FDXCShaderCompiler();

    // FShaderCompilerBackend Interface
    virtual bool Initialize() override final;

    virtual const CHAR* GetName() const override final { return "DXC"; }
    virtual bool SupportsOutputLanguage(EShaderOutputLanguage OutputLanguage) const override final;
    virtual FShaderCompilerIdentity GetIdentity() const override final;
    virtual bool Compile(const FShaderCompileRequest& Request, FShaderCompileResult& OutResult) override final;

private:
    void*                 DXCLib;
    DxcCreateInstanceProc DxcCreateInstanceFunc;
    uint32                VersionMajor;
    uint32                VersionMinor;
};
