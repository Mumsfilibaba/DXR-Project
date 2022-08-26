#pragma once
#include "ModuleInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FModuleManager

typedef void* PlatformModule;

class CORE_API FModuleManager
    : public FModuleInterface
{
    struct FModuleData
    {
        FModuleData() = default;

        FModuleData(const FString& InName, IModule* InInterface)
            : Interface(InInterface)
            , Handle(0)
            , Name(InName)
        { }

        IModule*       Interface;
        PlatformModule Handle;
        FString        Name;
    };

public:
    FModuleManager()  = default;
    ~FModuleManager() = default;

    virtual IModule* LoadModule(const CHAR* ModuleName) override final;
    virtual IModule* GetModule(const CHAR* ModuleName)  override final;

    virtual PlatformModule GetModuleHandle(const CHAR* ModuleName) override final;

    virtual void RegisterStaticModule(const CHAR* ModuleName, FInitializeStaticModuleDelegate InitDelegate) override final;

    virtual bool IsModuleLoaded(const CHAR* ModuleName) override final;

    virtual void UnloadModule(const CHAR* ModuleName) override final;

    virtual uint32 GetLoadedModuleCount() override final;

    void ReleaseAllModules();

private:
    FInitializeStaticModuleDelegate* GetStaticModuleDelegate(const CHAR* ModuleName);

    int32 GetModuleIndexUnlocked(const CHAR* ModuleName);

    FORCEINLINE uint32 GetLoadedModuleCountUnlocked()
    {
        return static_cast<uint32>(Modules.GetSize());
    }

private:
    typedef TPair<FString, FInitializeStaticModuleDelegate> FStaticModulePair;
    TArray<FStaticModulePair> StaticModuleDelegates;
    FCriticalSection          StaticModuleDelegatesCS;

    TArray<FModuleData>           Modules;
    FCriticalSection          ModulesCS;
};
