#pragma once
#include "ConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"

#include <cstdlib>
#include <sstream>

class CConsoleVariable : public CConsoleObject
{
public:
    virtual CConsoleVariable* AsVariable() override
    {
        return this;
    }

    virtual void SetInt( int32 InValue ) = 0;
    virtual void SetFloat( float InValue ) = 0;
    virtual void SetBool( bool InValue ) = 0;
    virtual void SetString( const CString& InValue ) = 0;

    virtual int32 GetInt() const = 0;
    virtual float GetFloat() const = 0;
    virtual bool GetBool() const = 0;
    virtual CString GetString() const = 0;

    virtual bool IsInt() const = 0;
    virtual bool IsFloat() const = 0;
    virtual bool IsBool() const = 0;
    virtual bool IsString() const = 0;

    TMulticastDelegate<CConsoleVariable*> OnChangedDelegate;
};

template<typename T>
class TConsoleVariable : public CConsoleVariable
{
public:
    FORCEINLINE TConsoleVariable()
        : CConsoleVariable()
        , Value()
    {
    }

    FORCEINLINE TConsoleVariable( T StartValue )
        : CConsoleVariable()
        , Value( StartValue )
    {
    }

    virtual void SetString( const CString& InValue ) override;

    virtual CString GetString() const override;

    virtual void SetInt( int32 InValue ) override
    {
        Value = (T)InValue;
        OnChanged();
    }

    virtual void SetFloat( float InValue ) override
    {
        Value = (T)InValue;
        OnChanged();
    }

    virtual void SetBool( bool InValue ) override
    {
        Value = (T)InValue;
        OnChanged();
    }

    virtual int32 GetInt() const override
    {
        return (int32)Value;
    }

    virtual float GetFloat() const override
    {
        return (float)Value;
    }

    virtual bool GetBool() const override
    {
        return (bool)Value;
    }

    virtual bool IsInt() const override
    {
        return false;
    }

    virtual bool IsFloat() const override
    {
        return false;
    }

    virtual bool IsBool() const override
    {
        return false;
    }

    virtual bool IsString() const override
    {
        return false;
    }

private:
    FORCEINLINE void OnChanged()
    {
        if ( OnChangedDelegate.IsBound() )
        {
            OnChangedDelegate.Broadcast( this );
        }
    }

    T Value;
};

// int32
template<>
inline void TConsoleVariable<int32>::SetString( const CString& InValue )
{
    Value = atoi( InValue.CStr() );
    OnChanged();
}

template<>
inline bool TConsoleVariable<int32>::IsInt() const
{
    return true;
}

template<>
inline CString TConsoleVariable<int32>::GetString() const
{
    CString String;
    String.Format( "%d", Value );
    return String;
}

// float
template<>
inline void TConsoleVariable<float>::SetString( const CString& InValue )
{
    Value = (float)atof( InValue.CStr() );
    OnChanged();
}

template<>
inline bool TConsoleVariable<float>::IsFloat() const
{
    return true;
}

template<>
inline CString TConsoleVariable<float>::GetString() const
{
    CString String;
    String.Format( "%.4f", Value );
    return String;
}

// bool
template<>
inline void TConsoleVariable<bool>::SetString( const CString& InValue )
{
    CString Lower = InValue.ToLower();

    int32 Number = 0;
    std::istringstream Stream( Lower.CStr() );
    Stream >> Number;

    if ( Stream.fail() )
    {
        Stream.clear();
        Stream >> std::boolalpha >> Value;
    }
    else
    {
        Value = static_cast<bool>(Number);
    }

    if ( !Stream.fail() )
    {
        OnChanged();
    }
}

template<>
inline CString TConsoleVariable<bool>::GetString() const
{
    return Value ? "true" : "false";
}

template<>
inline bool TConsoleVariable<bool>::IsBool() const
{
    return true;
}

// String
template<>
inline void TConsoleVariable<CString>::SetInt( int32 InValue )
{
    Value.Format( "%d", InValue );
}

template<>
inline void TConsoleVariable<CString>::SetFloat( float InValue )
{
    Value.Format( "%.4f", InValue );
}

template<>
inline void TConsoleVariable<CString>::SetBool( bool InValue )
{
    std::stringstream Stream;
    Stream << std::boolalpha << InValue;
    Value = Stream.str().c_str();
}

template<>
inline void TConsoleVariable<CString>::SetString( const CString& InValue )
{
    Value = InValue;
}

template<>
inline bool TConsoleVariable<CString>::IsString() const
{
    return true;
}

template<>
inline int32 TConsoleVariable<CString>::GetInt() const
{
    return 0;
}

template<>
inline float TConsoleVariable<CString>::GetFloat() const
{
    return 0.0f;
}

template<>
inline bool TConsoleVariable<CString>::GetBool() const
{
    return false;
}

template<>
inline CString TConsoleVariable<CString>::GetString() const
{
    return Value;
}
