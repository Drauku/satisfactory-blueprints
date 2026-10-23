#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Modules/ModuleManager.h"

class FBlueprintWorldBridgeModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues Values);
    void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    bool ResolveStoredWorld(const FString& StoredWorld, FString& OutWorld);
    void CheckStockWorldName(const FString& WorldName, const FString& PackageName) const;

    FDelegateHandle PostWorldInitializationHandle;
    FDelegateHandle WorldCleanupHandle;
    FDelegateHandle FindBlueprintWorldHook;

    // Blueprint loading is not guaranteed to happen on the game thread, so the cached world
    // is guarded.
    mutable FCriticalSection CurrentWorldLock;
    TWeakObjectPtr<UWorld> CurrentWorld;
    FString CurrentWorldName;
    FString CurrentPackageName;
    bool bCheckedStockWorld = false;

    // Each stored name is logged once per world rather than once per blueprint load.
    TSet<FString> LoggedStoredWorlds;
};
