#include "ShaderCompiler/FXC/FXCShaderTranslator.h"

// Matches D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS in Constants.hlsli
static const CHAR* const GShaderConstantsSpace = "space1";

static FShaderToken MakeToken(const FShaderToken& Location, const CHAR* Text, EShaderTokenType Type, bool bLeadingSpace)
{
    FShaderToken Token;
    Token.Text          = Text;
    Token.Type          = Type;
    Token.FileIndex     = Location.FileIndex;
    Token.Line          = Location.Line;
    Token.bLeadingSpace = bLeadingSpace;
    return Token;
}

static bool TranslationError(FShaderPreprocessorOutput& Source, const FShaderToken& Location, const String& Message)
{
    const bool bHasFile = Location.FileIndex >= 0 && Location.FileIndex < Source.Files.Size();
    Source.Errors += String::Printf("%s(%d): error: %s\n", bHasFile ? *Source.Files[Location.FileIndex] : "ShaderSource", Location.Line, *Message);
    return false;
}

static bool IsHexFloat(const FShaderToken& Token)
{
    const String& Text = Token.Text;
    if (Token.Type != EShaderTokenType::Number || Text.Length() < 3 || Text[0] != '0' || (Text[1] != 'x' && Text[1] != 'X'))
    {
        return false;
    }

    for (int32 Index = 2; Index < Text.Length(); ++Index)
    {
        if (Text[Index] == '.' || Text[Index] == 'p' || Text[Index] == 'P')
        {
            return true;
        }
    }

    return false;
}

static int32 GetHexDigit(CHAR Char)
{
    if (Char >= '0' && Char <= '9')
    {
        return Char - '0';
    }

    if (Char >= 'a' && Char <= 'f')
    {
        return Char - 'a' + 10;
    }

    if (Char >= 'A' && Char <= 'F')
    {
        return Char - 'A' + 10;
    }

    return -1;
}

static bool ConvertHexFloat(const String& Text, String& OutText)
{
    double Mantissa       = 0.0;
    int32  Exponent       = 0;
    bool   bFraction      = false;
    bool   bAnyDigits     = false;

    int32 Index = 2;
    for (; Index < Text.Length(); ++Index)
    {
        const CHAR Char = Text[Index];
        if (Char == '.')
        {
            if (bFraction)
            {
                return false;
            }

            bFraction = true;
            continue;
        }

        const int32 Digit = GetHexDigit(Char);
        if (Digit < 0)
        {
            break;
        }

        Mantissa = Mantissa * 16.0 + static_cast<double>(Digit);
        bAnyDigits = true;

        if (bFraction)
        {
            Exponent -= 4;
        }
    }

    if (!bAnyDigits)
    {
        return false;
    }

    if (Index < Text.Length() && (Text[Index] == 'p' || Text[Index] == 'P'))
    {
        ++Index;

        int32 Sign = 1;
        if (Index < Text.Length() && (Text[Index] == '+' || Text[Index] == '-'))
        {
            Sign = (Text[Index] == '-') ? -1 : 1;
            ++Index;
        }

        int32 BinaryExponent = 0;
        bool  bExponentDigits = false;
        for (; Index < Text.Length() && Text[Index] >= '0' && Text[Index] <= '9'; ++Index)
        {
            BinaryExponent = BinaryExponent * 10 + (Text[Index] - '0');
            bExponentDigits = true;
        }

        if (!bExponentDigits)
        {
            return false;
        }

        Exponent += Sign * BinaryExponent;
    }

    // Everything after the number is the suffix, f or h for example
    const String Suffix = Text.SubString(Index, Text.Length() - Index);
    for (int32 SuffixIndex = 0; SuffixIndex < Suffix.Length(); ++SuffixIndex)
    {
        const CHAR Char = Suffix[SuffixIndex];
        if (Char != 'f' && Char != 'F' && Char != 'h' && Char != 'H' && Char != 'l' && Char != 'L')
        {
            return false;
        }
    }

    double Value = Mantissa;
    for (; Exponent > 0; --Exponent)
    {
        Value *= 2.0;
    }

    for (; Exponent < 0; ++Exponent)
    {
        Value *= 0.5;
    }

    // 17 significant digits round-trip any double, so the literal keeps its exact value
    OutText = String::Printf("%.17g", Value);
    if (!OutText.Contains('.') && !OutText.Contains('e'))
    {
        OutText += ".0";
    }

    OutText += Suffix;
    return true;
}

// Returns the index of the first ']' of the closing ']]', or -1 when the brackets at Start are not an attribute
static int32 FindAttributeEnd(const TArray<FShaderToken>& Tokens, int32 Start)
{
    int32 Depth = 0;
    for (int32 Index = Start + 2; Index + 1 < Tokens.Size(); ++Index)
    {
        if (Tokens[Index].Is("["))
        {
            ++Depth;
        }
        else if (Tokens[Index].Is("]"))
        {
            if (Depth > 0)
            {
                --Depth;
            }
            else
            {
                return Tokens[Index + 1].Is("]") ? Index : -1;
            }
        }
        else if (Tokens[Index].Is("::") || Tokens[Index].Is(";") || Tokens[Index].Is("{"))
        {
            // Namespaced attributes such as [[vk::binding]] belong to other compilers
            return -1;
        }
    }

    return -1;
}

static bool TranslateConstantBuffer(FShaderPreprocessorOutput& Source, const TArray<FShaderToken>& Tokens, int32& InOutIndex, TArray<FShaderToken>& OutTokens)
{
    const FShaderToken& Keyword = Tokens[InOutIndex];

    // ConstantBuffer < Type... >
    int32 Index = InOutIndex + 2;
    int32 Depth = 1;

    TArray<FShaderToken> TypeTokens;
    for (; Index < Tokens.Size(); ++Index)
    {
        const FShaderToken& Token = Tokens[Index];
        if (Token.Is("<"))
        {
            ++Depth;
        }
        else if (Token.Is(">") && --Depth == 0)
        {
            break;
        }
        else if (Token.Is(">>"))
        {
            return TranslationError(Source, Token, "Nested template arguments are not supported in ConstantBuffer<T>, use a typedef");
        }
        else if (Token.Is(";"))
        {
            break;
        }

        TypeTokens.Add(Token);
    }

    if (Index >= Tokens.Size() || !Tokens[Index].Is(">") || TypeTokens.IsEmpty())
    {
        return TranslationError(Source, Keyword, "Expected 'ConstantBuffer<Type> Name'");
    }

    ++Index;
    if (Index >= Tokens.Size() || Tokens[Index].Type != EShaderTokenType::Identifier)
    {
        return TranslationError(Source, Keyword, "Expected a name after ConstantBuffer<Type>");
    }

    const FShaderToken& Name = Tokens[Index++];
    if (Index < Tokens.Size() && Tokens[Index].Is("["))
    {
        return TranslationError(Source, Name, String::Printf("'%s' is an array of constant buffers, which Shader Model 5.0 does not have", *Name.Text));
    }

    // : register ( bN [, spaceM] )
    String Register;
    if (Index < Tokens.Size() && Tokens[Index].Is(":"))
    {
        const bool bHasRegister = Index + 3 < Tokens.Size() && Tokens[Index + 1].Is("register") && Tokens[Index + 2].Is("(");
        if (!bHasRegister)
        {
            return TranslationError(Source, Name, String::Printf("Expected ': register(bN)' after '%s'", *Name.Text));
        }

        Index += 3;
        Register = Tokens[Index++].Text;

        String Space;
        if (Index < Tokens.Size() && Tokens[Index].Is(","))
        {
            ++Index;
            if (Index < Tokens.Size())
            {
                Space = Tokens[Index++].Text;
            }
        }

        if (Index >= Tokens.Size() || !Tokens[Index].Is(")"))
        {
            return TranslationError(Source, Name, String::Printf("Expected ')' after the register of '%s'", *Name.Text));
        }

        ++Index;

        if (!Register.StartsWith("b"))
        {
            return TranslationError(Source, Name, String::Printf("'%s' uses register '%s', constant buffers use b registers", *Name.Text, *Register));
        }

        if (Space.Equals(GShaderConstantsSpace))
        {
            Register = FXC_SHADER_CONSTANTS_REGISTER;
        }
        else if (!Space.IsEmpty() && !Space.Equals("space0"))
        {
            return TranslationError(Source, Name, String::Printf("'%s' is in register %s, Shader Model 5.0 has no register spaces", *Name.Text, *Space));
        }
    }

    if (Index >= Tokens.Size() || !Tokens[Index].Is(";"))
    {
        return TranslationError(Source, Name, String::Printf("Expected ';' after the declaration of '%s'", *Name.Text));
    }

    // cbuffer Name_CB : register(bN) { Type Name; }
    const String BufferName = Name.Text + "_CB";
    OutTokens.Add(MakeToken(Keyword, "cbuffer", EShaderTokenType::Identifier, Keyword.bLeadingSpace));
    OutTokens.Add(MakeToken(Keyword, *BufferName, EShaderTokenType::Identifier, true));

    if (!Register.IsEmpty())
    {
        OutTokens.Add(MakeToken(Keyword, ":", EShaderTokenType::Punctuator, true));
        OutTokens.Add(MakeToken(Keyword, "register", EShaderTokenType::Identifier, true));
        OutTokens.Add(MakeToken(Keyword, "(", EShaderTokenType::Punctuator, false));
        OutTokens.Add(MakeToken(Keyword, *Register, EShaderTokenType::Identifier, false));
        OutTokens.Add(MakeToken(Keyword, ")", EShaderTokenType::Punctuator, false));
    }

    OutTokens.Add(MakeToken(Keyword, "{", EShaderTokenType::Punctuator, true));

    for (int32 TypeIndex = 0; TypeIndex < TypeTokens.Size(); ++TypeIndex)
    {
        FShaderToken& TypeToken = OutTokens.Emplace(TypeTokens[TypeIndex]);
        TypeToken.bLeadingSpace = TypeIndex == 0 || TypeToken.bLeadingSpace;
    }

    OutTokens.Add(MakeToken(Keyword, *Name.Text, EShaderTokenType::Identifier, true));
    OutTokens.Add(MakeToken(Keyword, ";", EShaderTokenType::Punctuator, false));
    OutTokens.Add(MakeToken(Keyword, "}", EShaderTokenType::Punctuator, true));

    // The original ';' closes the cbuffer
    InOutIndex = Index;
    return true;
}

bool FFXCShaderTranslator::Translate(FShaderPreprocessorOutput& InOutSource)
{
    const TArray<FShaderToken>& Tokens = InOutSource.Tokens;

    TArray<FShaderToken> Result;
    Result.Reserve(Tokens.Size());

    for (int32 Index = 0; Index < Tokens.Size();)
    {
        const FShaderToken& Token = Tokens[Index];
        if (Token.Type == EShaderTokenType::Identifier && Token.Is("ConstantBuffer") && Index + 1 < Tokens.Size() && Tokens[Index + 1].Is("<"))
        {
            if (!TranslateConstantBuffer(InOutSource, Tokens, Index, Result))
            {
                return false;
            }

            continue;
        }

        if (Token.Is("[") && Index + 1 < Tokens.Size() && Tokens[Index + 1].Is("["))
        {
            const int32 End = FindAttributeEnd(Tokens, Index);
            if (End > 0)
            {
                Result.Add(Token);
                for (int32 Inner = Index + 2; Inner < End; ++Inner)
                {
                    Result.Add(Tokens[Inner]);
                }

                Result.Add(Tokens[End]);
                Index = End + 2;
                continue;
            }
        }

        if (IsHexFloat(Token))
        {
            String Decimal;
            if (!ConvertHexFloat(Token.Text, Decimal))
            {
                return TranslationError(InOutSource, Token, String::Printf("'%s' is not a valid hexadecimal float", *Token.Text));
            }

            FShaderToken& Converted = Result.Emplace(Token);
            Converted.Text = Decimal;
            ++Index;
            continue;
        }

        Result.Add(Token);
        ++Index;
    }

    InOutSource.Tokens = ::Move(Result);
    return true;
}
