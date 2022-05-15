#pragma once
//#define __EMULATE_UUID (1)
#include <dxc/dxcapi.h>

#include "RHIModule.h"
#include "RHIShader.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShaderModel

enum class EShaderModel
{
    Unknown = 0,
    SM_5_0 = 1,
    SM_5_1 = 2,
    SM_6_0 = 3,
    SM_6_1 = 4,
    SM_6_2 = 5,
    SM_6_3 = 6,
    SM_6_4 = 7,
    SM_6_5 = 8,
    SM_6_6 = 9,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SShaderDefine

struct SShaderDefine
{
    FORCEINLINE SShaderDefine(const String& InDefine)
        : Define(InDefine)
        , Value()
    { }

    FORCEINLINE SShaderDefine(const String& InDefine, const String& InValue)
        : Define(InDefine)
        , Value(InValue)
    { }

    String Define;
    String Value;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShaderCompileRequest

class CShaderCompileRequest
{
public:
    
    CShaderCompileRequest()
        : EntryPoint()
        , ShaderModel(EShaderModel::Unknown)
        , ShaderStage(EShaderStage::Unknown)
        , Defines()
    { }
    
    CShaderCompileRequest( const String& InEntryPoint
                                 , EShaderModel InShaderModel
                                 , EShaderStage InShaderStage
                                 , const TArrayView<SShaderDefine>& InDefines)
        : EntryPoint(InEntryPoint)
        , ShaderModel(InShaderModel)
        , ShaderStage(InShaderStage)
        , Defines(InDefines)
    { }
    
    String                    EntryPoint;
    
    EShaderModel              ShaderModel;
    EShaderStage              ShaderStage;
    
    TArrayView<SShaderDefine> Defines;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShaderFromFileCompileRequest

class CShaderFromFileCompileRequest
{
public:
    
    CShaderFromFileCompileRequest()
        : Filename()
        , EntryPoint()
        , ShaderModel(EShaderModel::Unknown)
        , ShaderStage(EShaderStage::Unknown)
        , Defines()
    { }
    
    CShaderFromFileCompileRequest( const String& InFilename
                                 , const String& InEntryPoint
                                 , EShaderModel InShaderModel
                                 , EShaderStage InShaderStage
                                 , const TArrayView<SShaderDefine>& InDefines)
        : Filename(InFilename)
        , EntryPoint(InEntryPoint)
        , ShaderModel(InShaderModel)
        , ShaderStage(InShaderStage)
        , Defines(InDefines)
    { }
    
    String                    Filename;
    String                    EntryPoint;
    
    EShaderModel              ShaderModel;
    EShaderStage              ShaderStage;
    
    TArrayView<SShaderDefine> Defines;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShaderCompiler

class RHI_API CShaderCompiler
{
private:
    CShaderCompiler();
    ~CShaderCompiler();
    
public:
    
    static bool Initialize();
    
    static void Release();
    
    static bool CompileFromFile(const CShaderFromFileCompileRequest& CompileRequest, TArray<uint8>& OutByteCode);
    
    static bool Compile();
    
private:
    struct IDxcCompiler* Compiler;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* DEPRECATED */

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHIShaderCompiler

class IRHIShaderCompiler
{
public:
    virtual ~IRHIShaderCompiler() = default;

    virtual bool CompileFromFile(const String& FilePath, const String& EntryPoint, const TArray<SShaderDefine>* Defines, EShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code) = 0;
    virtual bool CompileShader(const String& ShaderSource, const String& EntryPoint, const TArray<SShaderDefine>* Defines, EShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code) = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderCompiler

class CRHIShaderCompiler
{
public:
    static FORCEINLINE bool CompileFromFile(const String& FilePath, const String& EntryPoint, const TArray<SShaderDefine>* Defines, EShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code)
    {
        return GShaderCompiler->CompileFromFile(FilePath, EntryPoint, Defines, ShaderStage, ShaderModel, Code);
    }

    static FORCEINLINE bool CompileShader(const String& ShaderSource, const String& EntryPoint, const TArray<SShaderDefine>* Defines, EShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code)
    {
        return GShaderCompiler->CompileShader(ShaderSource, EntryPoint, Defines, ShaderStage, ShaderModel, Code);
    }
};

