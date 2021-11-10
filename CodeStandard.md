## Code Standard

#### Contents
- [Code Standard](#code-standard)
    - [Contents](#contents)
  - [If Statements](#if-statements)
  - [Loops](#loops)
  - [Templates](#templates)
  - [Classes](#classes)
  - [Interfaces](#interfaces)
  - [Enums](#enums)
  - [Structs](#structs)
  - [Union](#union)
  - [Virtual](#virtual)
  - [Naming Conventions](#naming-conventions)
  - [Platform Specific Code](#platform-specific-code)
  - [FORCEINLINE macro](#forceinline-macro)

### If Statements
* If statements should be written using the following style:

```
if (condition)
{
  //Statements
}
else
{
  //Statements
}
```
* Note that single if-statements should be avoided. This is to minimize the amount of bugs in the application.

* Conditional (ternary) operator are allowed
```
condition ? true : false;
```

### Loops
* Loops (for, while, do while) should be written using the following style:

```
while (condition)
{
  //Statements
}
```

* Note that single line loops should be avoided. This is to minimize the amount of bugs in the application.

### Templates
* Templates should use the following style:
```
template<typename T>
class TMyClass
{
};

template<typename T>
void SomeFunc()
{
}
```
* Note that the classname is using capital letter 'T' as prefix
* Note the use of typename instead of class, typename is preferred over class

* When forward declaring a template the following style should be used:

```
// Correct
template<typename InvokableType>
class TFunction;

// Wrong
template<typename T> class TFunction;
```

### Classes
* Classes should be written on using the following style:
```
class CMyClass
{
public:
  // Public functions and variable here ...

protected:
  // Protected functions and variable here ...

private:
  // Protected functions and variable here ...
};
```
* Classes use 'C' as prefix

* Note the order of the access modifers

* Header files should be included in compilation units (.cpp). Only reason to include in the header is when a class/struct is 
using the type more than just a pointer or reference, such as calling a function, accessing a variable or declaring the type directly. This means that forward declarations has to and should be used.

* Classes that are meant to be used outside the engine- project, and are not header-only should use the export macro before the class-name
```
class CORE_API MyClass
{
};
```

* The export macro depend on the engine module it belongs to for example
```
// Core engine module
#define CORE_API ...

// RHI layer
#define RHI_API ...

// etc.
```

### Interfaces
* Interfaces do not contain any state (i.e no variables) and does not provide any function definition. All functions should be pure virtual.

* Interfaces should use the capital letter 'I' as prefix.

* Interfaces should use a virtual destructor if the interface will be deleted as the instance-type

```
class IMyInterface
{
public:
  virtual ~IMyInterface() = default;
  
  virtual void Func() = 0;
}
```

### Enums
* Enums should use the capital letter 'E' as prefix

* Enum class should be preferred, unless they mostly will be used as integers

* Example can be seen below:
```
enum class EMyEnum
{
  Car   = 0,
  Apple = 1,
};
```

* Enums **not** using the class keyword should prefix all enumerators with the name of the enum:
```
enum EMyEnum
{
  MyEnum_Something0 = 0,
  MyEnum_Something1 = 1,
};
```

### Structs
* Structs should primarily be used for data only

* Structs use 'S' as prefix, for example

```
struct SMyStruct
{
  int32 x;
  int32 y;
};
```

### Union
* Unions use 'U' as prefix, for example

```
union UMyUnion
{
  int32 x;
  float y;
};
```

### Virtual
* Should be used when runtime polymorphism is needed

* Prefeer functions that can be determined at compile-time due to performance

### Naming Conventions
* Variables normaly no not use prefixes and should use the following style
```
// Correct
int* MyPointer  = nullptr;

// Wrong
int* pMyPointer = nullptr;
```

* This applies to classes and structues as well
```
class CMyClass
{
private:
  int  MyInteger = 0;
  int* Pointer   = nullptr;
 };
```

* Global variables should have 'G' as a prefix:
```
bool GMyGlobal;
```
* Mathematical components such as x, y, z etc. should **NOT** be capitalized.
```
struct SMyMathStruct
{
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};
```

* Functions use pascalcase. Same with parameters and local variables.
```
void FunctionsLookLikeThis( int FirstAParameter )
{
  int ALocalVariable = FirstAParameter;
}
```

* Memberfunctions also use pascalcase. Parameters with same name as a member variable should use the 'In' or 'New' prefix:
```
class CMyClass
{
public:
  void MyMemberFunction( int InVariable )
  {
    Variable = InVariable;
  }

private:
  int Variable = 0;
};
```

### Platform Specific Code
* Platform specific code should be kept in seperate directories with the platform name. Generic should contain platform-independent code only
```
Application/Generic/
Application/Mac/
Application/Windows/
```
* Classes specific to platform should be prefixed with platform-name

* Prefer static classes
```
class CGenericApplication
{
public:
  static void Func()
  {
  }
};

class CMacApplication : public CGenericApplication
{
public:
  static void Func()
  {
  }
};

typedef MacApplication PlatformApplication;

class CWindowsApplication : public CGenericApplication
{
public:
  static void Func()
  {
  }
};

typedef CWindowsApplication PlatformApplication;
```

* Platform classes should be accompanied with a Platform-header like this:
```
//PlatformApplication.h

#pragma once
#if PLATFORM_WINDOWS
  #include "Application/Windows/WindowsApplication.h"
#elif PLATFORM_MACOS
  #include "Application/Mac/MacApplication.h"
#else
  #error No platform defined
#endif
```
### FORCEINLINE macro
* The FORCEINLINE- macro should be used on all one-line functions (for example setters and getters). The only time this does not apply is when the class or function is exported with the LAMBDA_API macro. When you are certain that the function will stay the same between versions, that is when the code in the .exe and .dll will be the same no matter what, it is ok to still use FORCEINLINE. However, it is better practise to make sure that a function is exported, e.i. define the function in the compilation unit (.cpp). 


