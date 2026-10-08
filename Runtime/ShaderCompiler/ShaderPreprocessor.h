#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"

enum class EShaderTokenType : uint8
{
    Identifier,
    Number,
    String,
    Punctuator,
    Other,

    /** A directive that is passed through to the compiler, #pragma for example. Always rendered on its own line. */
    Directive,
};

struct FShaderToken
{
    NODISCARD bool Is(const CHAR* InText) const
    {
        return Text.Equals(InText);
    }

    String           Text;
    EShaderTokenType Type          = EShaderTokenType::Other;
    int32            FileIndex     = 0;
    int32            Line          = 0;
    bool             bLeadingSpace = false;
    bool             bStartOfLine  = false;

    /** The macro this identifier names was being expanded when it was reached, so it is never expanded again */
    bool             bNoExpand      = false;
    bool             bPasteOperator = false;
};

struct SHADERCOMPILER_API FShaderPreprocessorOutput
{
    /** @return Returns the tokens as source text, with #line markers that map every line back to the file it came from */
    NODISCARD String Render() const;

    TArray<FShaderToken> Tokens;

    /** Indexed by FShaderToken::FileIndex */
    TArray<String>       Files;

    /** Every file that was included, the root file is not part of the list */
    TArray<String>       Dependencies;
    String               Errors;
};

class SHADERCOMPILER_API FShaderPreprocessor
{
    struct FMacro
    {
        TArray<FShaderToken> Body;
        TArray<String>       Parameters;
        bool                 bFunctionLike = false;

        /** Set while the expansion of the macro is being rescanned */
        bool                 bDisabled = false;
    };

    struct FExpansionFrame
    {
        TArray<FShaderToken> Tokens;
        int32                Position = 0;
        FMacro*              Macro    = nullptr;
    };

    struct FConditional
    {
        bool bParentActive = false;
        bool bActive       = false;
        bool bTaken        = false;
        bool bSeenElse     = false;
    };

public:
    /** Changes whenever the output for the same input changes, so compiled shaders can be invalidated */
    static constexpr uint32 Version = 1;

    /** @brief Splits source text into preprocessing tokens. Line continuations are removed and comments become whitespace. */
    static void Tokenize(StringView Source, int32 FileIndex, TArray<FShaderToken>& OutTokens);

    FShaderPreprocessor(const String& InIncludeDir);
    ~FShaderPreprocessor();

    /** @brief Defines a macro the same way '#define Name Value' would */
    void AddDefine(const String& Name, const String& Value);

    /**
     * @brief Preprocesses a shader and every file it includes
     * @param FilePath The path of the source, used to resolve relative includes. Empty when compiling from source.
     * @return Returns false and fills OutOutput.Errors when the source cannot be preprocessed
     */
    bool Preprocess(const String& FilePath, StringView Source, FShaderPreprocessorOutput& OutOutput);

private:
    bool ProcessFile(const String& FilePath, StringView Source, int32 IncludeDepth);
    bool HandleDirective(TArrayView<const FShaderToken> Tokens, const FShaderToken& HashToken, const String& FilePath, int32 IncludeDepth);
    bool HandleDefine(TArrayView<const FShaderToken> Tokens, const FShaderToken& Location);
    bool HandleInclude(TArrayView<const FShaderToken> Tokens, const FShaderToken& Location, const String& FilePath, int32 IncludeDepth);
    bool EvaluateCondition(TArrayView<const FShaderToken> Tokens, const FShaderToken& Location, bool& OutValue);

    bool ExpandTokens(TArray<FShaderToken>&& Input, TArray<FShaderToken>& OutTokens);
    bool Substitute(const FMacro& Macro, const TArray<TArray<FShaderToken>>& Arguments, const FShaderToken& NameToken, TArray<FShaderToken>& OutTokens);

    NODISCARD bool IsActive() const;
    bool Error(const FShaderToken& Location, const String& Message);

    String                     IncludeDir;
    TMap<String, FMacro>       Macros;
    TSet<String>               OnceFiles;
    TArray<FConditional>       Conditionals;
    FShaderPreprocessorOutput* Output;
};
