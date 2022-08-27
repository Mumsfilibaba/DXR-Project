## Code Standard

#### Contents
- [Code Standard](#code-standard)
    - [Contents](#contents)
  - [If Statements](#if-statements)
  - [Loops](#loops)
  - [Templates](#templates)
  - [Classes](#classes)
  - [Structs](#structs)
  - [Interfaces](#interfaces)
  - [Enums](#enums)
  - [Union](#union)
  - [Virtual](#virtual)
  - [Naming Conventions](#naming-conventions)
  - [Platform Specific Code](#platform-specific-code)

### If Statements
* If statements should be written using the following style:

```
if (condition)
{
  // Statements
}
else
{
  // Statements
}
```
* Avoid single-line if-statements. This is to minimize the amount of bugs in the application.

* Conditional (ternary) operator are allowed
```
condition ? true : false;
```

### Loops
* Loops (for, while, do while) should be written using the following style:

```
while (condition)
{
  // Statements
}
```
```
for (int32 Index = 0; Index < SomeNumber; ++Index)
{
  // Statements
}
```
```
do
{
  // Statements
} while (condition)
```

* Avoid single-line loops. This is to minimize the amount of bugs in the application.

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
class FMyClass
{
  // Private constructor and destructor
  FMyClass();
  ~FMyClass();

public:
  // Public functions and variable here ...

protected:
  // Protected functions and variable here ...

private:
  // Protected functions and variable here ...
};
```
* Classes use 'F' as prefix

* Note the order of the access modifers

* Note that the constructor is kept on top even though the private accessor is on the bottom. This is to increase readability. Try to keep constructors in the top. 

* Header files should be included in compilation units (.cpp). Only reason to include in the header is when a class/struct is using the type more than just a pointer or reference, such as calling a function, accessing a variable or declaring the type directly. This means that forward declarations has to and should be used.

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

### Structs
* Structs should be used when all members are public

* Structs use 'F' as prefix, for example

```
struct FMyStruct
{
  int32 x;
  int32 y;
};
```

### Interfaces
* Interfaces do not contain any state (i.e no variables) and does not provide any function definition. All functions should be pure virtual.

* Interfaces should use the capital letter 'I' as prefix.

* Interfaces should use a virtual destructor if the interface will be deleted as the instance-type.

* Prefer structs over classes. Since everything most likeley will be public anyway.

```
struct IMyInterface
{
  virtual ~IMyInterface() = default;
  
  virtual void Func() = 0;
}
```

### Enums
* Enums should use the capital letter 'E' as prefix

* Enum class should be preferred, unless they mostly will be used as integers.

* Enum classes with flags can use the 'ENUM_CLASS_OPERATORS' macro to make operations such as 'and', 'or' etc to work.

* Example can be seen below:
```
enum class EMyEnum
{
  Car   = 0,
  Apple = 1,
};
ENUM_CLASS_OPERATORS(EMyEnum);
```

* Enums **not** using the class keyword should prefix all enumerators with the name of the enum:
```
enum EMyEnum
{
  MyEnum_Something0 = 0,
  MyEnum_Something1 = 1,
};
```

### Union
* Unions use 'F' as prefix, for example

```
union FMyUnion
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
class FMyClass
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
struct FMyMathStruct
{
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};
```

* Functions use pascalcase. Same with parameters and local variables.
```
void FunctionsLookLikeThis(int FirstAParameter)
{
  int ALocalVariable = FirstAParameter;
}
```

* Memberfunctions also use pascalcase. Parameters with same name as a member variable should use the 'In' or 'New' prefix:
```
class FMyClass
{
public:
  void MyMemberFunction(int InVariable)
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
Application/Platform/
Application/Mac/
Application/Windows/
```
* Classes specific to platform should be prefixed with platform-name

* Prefer static classes
```
class FGenericApplication
{
public:
  static void Func()
  {
  }
};

class FMacApplication : public FGenericApplication
{
public:
  static void Func()
  {
  }
};

class FWindowsApplication : public FGenericApplication
{
public:
  static void Func()
  {
  }
};
```

* Platform classes should be accompanied with a Platform-header like this:
```
// FPlatformApplication.h

#pragma once
#if PLATFORM_WINDOWS
  #include "Application/Windows/WindowsApplication.h"
  typedef FWindowsApplication FPlatformApplication;
#elif PLATFORM_MACOS
  #include "Application/Mac/MacApplication.h"
  typedef FMacApplication FPlatformApplication;
#else
  #error No platform defined
#endif
```
